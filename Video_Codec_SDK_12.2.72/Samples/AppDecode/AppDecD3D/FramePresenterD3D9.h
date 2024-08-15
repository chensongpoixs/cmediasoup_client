/*
* Copyright 2017-2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/
#pragma once

#include <iostream>
#include <mutex>
#include <thread>
#include <d3d9.h>
#include <cuda.h>
#include <cudaD3D9.h>
#include "FramePresenterD3D.h"
#include "../Utils/NvCodecUtils.h"

/**
* @brief D3D9 presenter class derived from FramePresenterD3D
*/
class FramePresenterD3D9 : public FramePresenterD3D
{
public:
    /**
    *   @brief  FramePresenterD3D9 constructor. This will launch a rendering thread which will be fed with decoded frames
    *   @param  cuContext - CUDA context handle
    *   @param  nWidth - Width of D3D surface
    *   @param  nHeight - Height of D3D surface
    */
    FramePresenterD3D9(CUcontext cuContext, int nWidth, int nHeight) : 
        FramePresenterD3D(cuContext, nWidth, nHeight) 
    {
        pthMsgLoop = new std::thread(ThreadProc, this);
        while (!bReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        hTimerQueue = CreateTimerQueue();
        hPresentEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    /**
    *   @brief  FramePresenterD3D9 destructor.
    */
    ~FramePresenterD3D9() {
        if (hTimerQueue)
        {
            DeleteTimerQueue(hTimerQueue);
        }
        if (hPresentEvent)
        {
            CloseHandle(hPresentEvent);
        }
        bQuit = true;
        pthMsgLoop->join();
        delete pthMsgLoop;
    }

    /**
    *   @brief  Presents a frame present in host memory. More specifically, it copies the host surface
    *           data to a d3d offscreenplain surface and then copies it to the swap chain backbuffer for presentation
    *   @param  pData - pointer to host surface data
    *   @param  nBytes - number of bytes to copy
    *   @return true on success
    *   @return false when the windowing thread is not ready to be served
    */
    bool PresentHostFrame(BYTE *pData, int nBytes) {
        mtx.lock();
        if (!bReady) {
            mtx.unlock();
            return false;
        }

        D3DLOCKED_RECT lockedRect;
        ck(pSurface->LockRect(&lockedRect, NULL, 0));
        for (int y = 0; y < nHeight; y++) {
            memcpy((uint8_t *)lockedRect.pBits + y * lockedRect.Pitch, pData + y * nWidth * 4, nWidth * 4);
        }
        ck(pSurface->UnlockRect());
        pDevice->UpdateSurface(pSurface, NULL, pBackBuffer, NULL);
        mtx.unlock();
        return true;
    }

    bool PresentDeviceFrame(unsigned char *dpBgra, int nPitch, int64_t delay) {
        mtx.lock();
        if (!bReady) {
            mtx.unlock();
            return false;
        }
        CopyDeviceFrame(dpBgra, nPitch);
        if (!CreateTimerQueueTimer(&hTimer, hTimerQueue,
            (WAITORTIMERCALLBACK)PresentRoutine, this, (DWORD)delay, 0, 0))
        {
            std::cout << "Problem in createtimer" << std::endl;
        }
        while (WaitForSingleObject(hPresentEvent, 0) != WAIT_OBJECT_0)
        {
        }
        if (hTimer)
        {
            DeleteTimerQueueTimer(hTimerQueue, hTimer, nullptr);
        }
        mtx.unlock();
        return true;
    }

private:
    /**
    *   @brief  Launches the windowing functionality
    *   @param  This - pointer to FramePresenterD3D9 object
    */
    static void ThreadProc(FramePresenterD3D9 *This) {
        This->Run();
    }
    /**
    *   @brief  Callback called by D3D runtime. This callback is registered during call to
    *           CreateTimerQueueTimer in PresentDeviceFrame. In CreateTimerQueueTimer we also
    *           set a timer. When this timer expires this callback is called. This functionality
    *           is present to facilitate timestamp based presentation.
    *   @param  lpParam - void pointer to client data
    *   @param  TimerOrWaitFired - TRUE for this callback as this is a Timer based callback (Refer:https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/ms687066(v=vs.85))
    */
    static VOID CALLBACK PresentRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired)
    {
        if (!lpParam) return;
        FramePresenterD3D9* presenter = (FramePresenterD3D9 *)lpParam;
        ck(presenter->pDevice->Present(NULL, NULL, NULL, NULL));
        SetEvent(presenter->hPresentEvent);
    }

    /**
    *   @brief This function is on a thread spawned during FramePresenterD3D9 construction.
    *          It creates the D3D window and monitors window messages in a loop. This function
    *          also creates swap chain for presentation and also registers the swap chain backbuffer
    *          with cuda.
    */
    void Run() {
        HWND hwndMain = CreateAndShowWindow(nWidth, nHeight);

        IDirect3D9Ex* pD3D = NULL;
        ck(Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D));
        D3DPRESENT_PARAMETERS d3dpp = { 0 };
        d3dpp.BackBufferWidth = nWidth;
        d3dpp.BackBufferHeight = nHeight;
        d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
        d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        d3dpp.EnableAutoDepthStencil = FALSE;
        d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
        d3dpp.Windowed = TRUE;
        d3dpp.hDeviceWindow = hwndMain;
        ck(pD3D->CreateDevice(GetAdapterByContext(cuContext), D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pDevice));
        pD3D->Release();

        ck(pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer));
        ck(pDevice->CreateOffscreenPlainSurface(nWidth, nHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurface, NULL));

        ck(cuCtxPushCurrent(cuContext));
        ck(cuGraphicsD3D9RegisterResource(&cuResource, pBackBuffer, CU_GRAPHICS_REGISTER_FLAGS_NONE));
        ck(cuGraphicsResourceSetMapFlags(cuResource, CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD));
        ck(cuCtxPopCurrent(NULL));

        bReady = true;
        MSG msg = { 0 };
        while (!bQuit && msg.message != WM_QUIT) {
            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        mtx.lock();
        bReady = false;
        ck(cuCtxPushCurrent(cuContext));
        ck(cuGraphicsUnregisterResource(cuResource));
        ck(cuCtxPopCurrent(NULL));
        pSurface->Release();
        pBackBuffer->Release();
        pDevice->Release();
        DestroyWindow(hwndMain);
        mtx.unlock();
    }

    /**
    *   @brief  Gets the D3D9 adapter ID on which the given cuda context is current
    *   @param   CUcontext - handle to cuda context
    *   @return  pAdapter - pointer to DXGI adapter
    *   @return  NULL - In case there is no adapter corresponding to the supplied cuda context
    */
    static UINT GetAdapterByContext(CUcontext cuContext) {
        CUdevice cuDeviceTarget;
        ck(cuCtxPushCurrent(cuContext));
        ck(cuCtxGetDevice(&cuDeviceTarget));
        ck(cuCtxPopCurrent(NULL));

        IDirect3D9Ex* pD3D = NULL;
        ck(Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D));
        for (unsigned i = 0; i < pD3D->GetAdapterCount(); i++) {
            D3DADAPTER_IDENTIFIER9 identifier;
            pD3D->GetAdapterIdentifier(i, 0, &identifier);
            CUdevice cuDevice;
            ck(cuD3D9GetDevice(&cuDevice, identifier.DeviceName));
            if (cuDevice == cuDeviceTarget) {
                return i;
            }
        }
        pD3D->Release();
        return NULL;
    }

private:
    bool bReady = false;
    bool bQuit = false;
    std::mutex mtx;
    std::thread *pthMsgLoop = NULL;

    IDirect3DDevice9 *pDevice = NULL;
    IDirect3DSurface9 *pBackBuffer = NULL, *pSurface = NULL;
    HANDLE hTimer;
    HANDLE hTimerQueue;
    HANDLE hPresentEvent;
};
