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

//---------------------------------------------------------------------------
//! \file AppDecD3D.cpp
//! \brief Source file for AppDecD3D sample
//!
//! This sample application illustrates the decoding of media file and display of decoded frames in a window.
//! This is done by CUDA interop with D3D(both D3D9 and D3D11).
//! For a detailed list of supported codecs on your NVIDIA GPU, refer : https://developer.nvidia.com/nvidia-video-codec-sdk#NVDECFeatures


#include <cuda.h>
#include <iostream>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "FramePresenterD3D9.h"
#include "FramePresenterD3D11.h"
#include "../Common/AppDecUtils.h"
#include "../Utils/ColorSpace.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

/**
*   @brief Function template to decode media file pointed to by szInFilePath parameter.
           The decoded frames are displayed by using the D3D-CUDA interop.
           In this app FramePresenterType is either FramePresenterD3D9 or FramePresenterD3D11.
           The presentation rate is based on per frame time stamp.
*   @param  cuContext - Handle to CUDA context
*   @param  szInFilePath - Path to file to be decoded
*   @return 0 on success
*/
template<class FramePresenterType, typename = std::enable_if<std::is_base_of<FramePresenterD3D, FramePresenterType>::value>>
int NvDecD3D(CUcontext cuContext, char *szInFilePath)
{
    unsigned int timescale = 1000; // get timestamp in milisecond
    FFmpegDemuxer demuxer(szInFilePath, timescale);
    NvDecoder dec(cuContext, true, FFmpeg2NvCodecId(demuxer.GetVideoCodec()), false, false, NULL, NULL, false, 0, 0, timescale);
    int nRGBWidth = (demuxer.GetWidth() + 1) & ~1;
    FramePresenterType presenter(cuContext, nRGBWidth, demuxer.GetHeight());
    CUdeviceptr dpFrame = 0;
    ck(cuMemAlloc(&dpFrame, nRGBWidth * demuxer.GetHeight() * 4));
    int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
    uint8_t *pVideo = NULL, *pFrame;
    int64_t pts, timestamp = 0;
    bool m_bFirstFrame = true;
    int64_t firstPts = 0, startTime = 0;
    LARGE_INTEGER m_Freq;
    int iMatrix = 0;

    QueryPerformanceFrequency(&m_Freq);

    do
    {
        demuxer.Demux(&pVideo, &nVideoBytes, &pts);
        nFrameReturned = dec.Decode(pVideo, nVideoBytes, 0, pts);
        if (!nFrame && nFrameReturned)
            LOG(INFO) << dec.GetVideoInfo();

        for (int i = 0; i < nFrameReturned; i++)
        {
            pFrame = dec.GetFrame(&timestamp);
            iMatrix = dec.GetVideoFormatInfo().video_signal_description.matrix_coefficients;
            if (dec.GetBitDepth() == 8)
            {
                 if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                    YUV444ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t *)dpFrame, 4 * nRGBWidth, dec.GetWidth(), dec.GetHeight(), iMatrix);
                else    // default assumed as NV12
                    Nv12ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t *)dpFrame, 4 * nRGBWidth, dec.GetWidth(), dec.GetHeight(), iMatrix);
            }
            else
            {
                if(dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                    YUV444P16ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t *)dpFrame, 4 * nRGBWidth, dec.GetWidth(), dec.GetHeight(), iMatrix);
                else // default assumed as P016
                    P016ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t *)dpFrame, 4 * nRGBWidth, dec.GetWidth(), dec.GetHeight(), iMatrix);
            }

            LARGE_INTEGER counter;
            if (m_bFirstFrame)
            {
                firstPts = timestamp;
                QueryPerformanceCounter(&counter);
                startTime = 1000 * counter.QuadPart / m_Freq.QuadPart;
                m_bFirstFrame = false;
            }

            QueryPerformanceCounter(&counter);
            int64_t curTime = timescale * counter.QuadPart / m_Freq.QuadPart;

            int64_t expectedRenderTime = timestamp - firstPts + startTime;
            int64_t delay = expectedRenderTime - curTime;
            if (timestamp == 0)
                delay = 0;
            if (delay < 0)
                continue;

            presenter.PresentDeviceFrame((uint8_t *)dpFrame, nRGBWidth * 4, delay);
        }
        nFrame += nFrameReturned;
    } while (nVideoBytes);
    ck(cuMemFree(dpFrame));
    std::cout << "Total frame decoded: " << nFrame << std::endl;
    return 0;
}

int main(int argc, char **argv) 
{
    char szInFilePath[256] = "";
    int iGpu = 0;
    int iD3d = 0;
    try
    {
        ParseCommandLine(argc, argv, szInFilePath, NULL, iGpu, NULL, &iD3d);
        CheckInputFile(szInFilePath);

        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu)
        {
            std::ostringstream err;
            err << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
            throw std::invalid_argument(err.str());
        }
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, CU_CTX_SCHED_BLOCKING_SYNC, cuDevice));

        switch (iD3d) {
        default:
        case 9:
            std::cout << "Display with D3D9." << std::endl;
            return NvDecD3D<FramePresenterD3D9>(cuContext, szInFilePath);
        case 11:
            std::cout << "Display with D3D11." << std::endl;
            return NvDecD3D<FramePresenterD3D11>(cuContext, szInFilePath);
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
