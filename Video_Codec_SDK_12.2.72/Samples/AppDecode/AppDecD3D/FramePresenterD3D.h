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
#include <cuda.h>
#include "../Utils/NvCodecUtils.h"

/**
* @brief Base class for D3D presentation of decoded frames
*/
class FramePresenterD3D {
public:
    /**
    *   @brief  FramePresenterD3D constructor.
    *   @param  cuContext - CUDA context handle
    *   @param  nWidth - Width of D3D Surface
    *   @param  nHeight - Height of D3D Surface
    */
    FramePresenterD3D(CUcontext cuContext, int nWidth, int nHeight) : cuContext(cuContext), nWidth(nWidth), nHeight(nHeight) {}
    /**
    *   @brief  FramePresenterD3D destructor.
    */
    virtual ~FramePresenterD3D() {};
    /**
    *   @brief  Pure virtual to be implemented by derived classes. Should present decoded frames available in device memory
    *   @param  dpBgra - CUDA device pointer to BGRA surface
    *   @param  nPitch - pitch of the BGRA surface. Typically width in pixels * number of bytes per pixel
    *   @param  delay  - presentation delay. Cue to D3D presenter to inform about the time at which this frame needs to be presented
    *   @return true on success
    *   @return false on failure
    */
    virtual bool PresentDeviceFrame(unsigned char *dpBgra, int nPitch, int64_t delay) = 0;

protected:
    /**
    *   @brief  Create and show D3D window.
    *   @param  nWidth   - Width of the window
    *   @param  nHeight  - Height of the window
    *   @return hwndMain - handle to the created window
    */
    static HWND CreateAndShowWindow(int nWidth, int nHeight) {
        double r = max(nWidth / 1280.0, nHeight / 720.0);
        if (r > 1.0) {
            nWidth = (int)(nWidth / r);
            nHeight = (int)(nHeight / r);
        }

        static char szAppName[] = "D3DPresenter";
        WNDCLASS wndclass;
        wndclass.style = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc = WndProc;
        wndclass.cbClsExtra = 0;
        wndclass.cbWndExtra = 0;
        wndclass.hInstance = (HINSTANCE)GetModuleHandle(NULL);
        wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName = NULL;
        wndclass.lpszClassName = szAppName;
        RegisterClass(&wndclass);

        RECT rc{
            (GetSystemMetrics(SM_CXSCREEN) - nWidth) / 2,
            (GetSystemMetrics(SM_CYSCREEN) - nHeight) / 2,
            (GetSystemMetrics(SM_CXSCREEN) + nWidth) / 2,
            (GetSystemMetrics(SM_CYSCREEN) + nHeight) / 2
        };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

        HWND hwndMain = CreateWindow(szAppName, szAppName, WS_OVERLAPPEDWINDOW,
            rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
            NULL, NULL, wndclass.hInstance, NULL);
        ShowWindow(hwndMain, SW_SHOW);
        UpdateWindow(hwndMain);

        return hwndMain;
    }

    /**
    *   @brief  Copy device frame to cuda registered D3D surface. More specifically, this function maps the
    *           D3D swap chain backbuffer into a cuda array and copies the contents of dpBgra into it.
    *           This ensures that the swap chain back buffer will contain the next surface to be presented.
    *   @param  dpBgra  - CUDA device pointer to BGRA surface
    *   @param  nPitch  - pitch of the BGRA surface. Typically width in pixels * number of bytes per pixel
    */
    void CopyDeviceFrame(unsigned char *dpBgra, int nPitch) {
        ck(cuCtxPushCurrent(cuContext));
        ck(cuGraphicsMapResources(1, &cuResource, 0));
        CUarray dstArray;
        ck(cuGraphicsSubResourceGetMappedArray(&dstArray, cuResource, 0, 0));

        CUDA_MEMCPY2D m = { 0 };
        m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        m.srcDevice = (CUdeviceptr)dpBgra;
        m.srcPitch = nPitch ? nPitch : nWidth * 4;
        m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        m.dstArray = dstArray;
        m.WidthInBytes = nWidth * 4;
        m.Height = nHeight;
        ck(cuMemcpy2D(&m));

        ck(cuGraphicsUnmapResources(1, &cuResource, 0));
        ck(cuCtxPopCurrent(NULL));
    }

private:
    /**
    *   @brief  Callback called by D3D runtime. This callback is registered during window creation.
    *           On Window close this function posts a quit message. The thread runs to completion once
    *           it retrieves this quit message in its message queue. Refer to Run() function in each of the
    *           derived classes.
    *   @param  hwnd   - handle to the window
    *   @param  msg    - The message sent to this window
    *   @param  wParam - Message specific additional information (No Op in this case)
    *   @param  lParam - Message specific additional information (No Op on this case)
    *   @return 0 on posting quit message
    *   @return result of default window procedure in cases other than the above
    */
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

protected:
    int nWidth = 0, nHeight = 0;
    CUcontext cuContext = NULL;
    CUgraphicsResource cuResource = NULL;
};
