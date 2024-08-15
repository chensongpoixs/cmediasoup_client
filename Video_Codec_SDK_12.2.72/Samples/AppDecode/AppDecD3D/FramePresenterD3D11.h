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
#include <d3d11.h>
#include <cuda.h>
#include <cudaD3D11.h>
#include "FramePresenterD3D.h"
#include "../Utils/NvCodecUtils.h"

/**
* @brief D3D11 presenter class derived from FramePresenterD3D
*/
class FramePresenterD3D11 : public FramePresenterD3D
{
public:
    /**
    *   @brief  FramePresenterD3D11 constructor. This will launch a rendering thread which will be fed with decoded frames
    *   @param  cuContext - CUDA context handle
    *   @param  nWidth - Width of D3D surface
    *   @param  nHeight - Height of D3D surface
    */
    FramePresenterD3D11(CUcontext cuContext, int nWidth, int nHeight) : 
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
    *   @brief  FramePresenterD3D11 destructor.
    */
    ~FramePresenterD3D11() {
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
    *           data to a d3d staging texture and then copies it to the swap chain backbuffer for presentation
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

        D3D11_MAPPED_SUBRESOURCE mappedTexture;
        ck(pContext->Map(pStagingTexture, 0, D3D11_MAP_WRITE, 0, &mappedTexture));
        memcpy(mappedTexture.pData, pData, min(nWidth * nHeight * 4, nBytes));
        pContext->Unmap(pStagingTexture, 0);
        pContext->CopyResource(pBackBuffer, pStagingTexture);
        ck(pSwapChain->Present(0, 0));
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
    *   @param  This - pointer to FramePresenterD3D11 object
    */
    static void ThreadProc(FramePresenterD3D11 *This) {
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
        FramePresenterD3D11* presenter = (FramePresenterD3D11 *)lpParam;
        presenter->pSwapChain->Present(1, 0);
        SetEvent(presenter->hPresentEvent);
    }

    /**
    *   @brief This function is on a thread spawned during FramePresenterD3D11 construction.
    *          It creates the D3D window and monitors window messages in a loop. This function
    *          also creates swap chain for presentation and also registers the swap chain backbuffer
    *          with cuda.
    */
    void Run() {
        HWND hwndMain = CreateAndShowWindow(nWidth, nHeight);

        DXGI_SWAP_CHAIN_DESC sc = { 0 };
        sc.BufferCount = 1;
        sc.BufferDesc.Width = nWidth;
        sc.BufferDesc.Height = nHeight;
        sc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sc.BufferDesc.RefreshRate.Numerator = 0;
        sc.BufferDesc.RefreshRate.Denominator = 1;
        sc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sc.OutputWindow = hwndMain;
        sc.SampleDesc.Count = 1;
        sc.SampleDesc.Quality = 0;
        sc.Windowed = TRUE;

        ID3D11Device *pDevice = NULL;
        ck(D3D11CreateDeviceAndSwapChain(GetAdapterByContext(cuContext), D3D_DRIVER_TYPE_UNKNOWN,
            NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sc, &pSwapChain, &pDevice, NULL, &pContext));
        ck(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer));

        D3D11_TEXTURE2D_DESC td;
        pBackBuffer->GetDesc(&td);
        td.BindFlags = 0;
        td.Usage = D3D11_USAGE_STAGING;
        td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ck(pDevice->CreateTexture2D(&td, NULL, &pStagingTexture));

        ck(cuCtxPushCurrent(cuContext));
        ck(cuGraphicsD3D11RegisterResource(&cuResource, pBackBuffer, CU_GRAPHICS_REGISTER_FLAGS_NONE));
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
        pStagingTexture->Release();
        pBackBuffer->Release();
        pContext->Release();
        pDevice->Release();
        pSwapChain->Release();
        DestroyWindow(hwndMain);
        mtx.unlock();
    }

    /**
    *   @brief  Gets the DXGI adapter on which the given cuda context is current
    *   @param   CUcontext - handle to cuda context
    *   @return  pAdapter - pointer to DXGI adapter
    *   @return  NULL - In case there is no adapter corresponding to the supplied cuda context
    */
    static IDXGIAdapter *GetAdapterByContext(CUcontext cuContext) {
        CUdevice cuDeviceTarget;
        ck(cuCtxPushCurrent(cuContext));
        ck(cuCtxGetDevice(&cuDeviceTarget));
        ck(cuCtxPopCurrent(NULL));

        IDXGIFactory1 *pFactory = NULL;
        ck(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&pFactory));
        IDXGIAdapter *pAdapter = NULL;
        for (unsigned i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
            CUdevice cuDevice;
            ck(cuD3D11GetDevice(&cuDevice, pAdapter));
            if (cuDevice == cuDeviceTarget) {
                pFactory->Release();
                return pAdapter;
            }
            pAdapter->Release();
        }
        pFactory->Release();
        return NULL;
    }

private:
    bool bReady = false;
    bool bQuit = false;
    std::mutex mtx;
    std::thread *pthMsgLoop = NULL;

    IDXGISwapChain *pSwapChain = NULL;
    ID3D11DeviceContext *pContext = NULL;
    ID3D11Texture2D *pBackBuffer = NULL, *pStagingTexture = NULL;
    HANDLE hTimer;
    HANDLE hTimerQueue;
    HANDLE hPresentEvent;
};
