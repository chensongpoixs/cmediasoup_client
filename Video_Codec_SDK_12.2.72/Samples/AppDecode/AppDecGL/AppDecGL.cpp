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
//! \file AppDecGL.cpp
//! \brief Source file for AppDecGL sample
//!
//! This sample application illustrates the decoding of media file and display of decoded frames in a window.
//! This is done by CUDA interop with OpenGL.
//! Synchronization between rendering and decode thread is achieved using ConcurrentQueue implementation.
//! For a detailed list of supported codecs on your NVIDIA GPU, refer : https://developer.nvidia.com/nvidia-video-codec-sdk#NVDECFeatures
//---------------------------------------------------------------------------

#include <cuda.h>
#include <iostream>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "../Common/AppDecUtils.h"
#include "../Utils/ColorSpace.h"

#include "FramePresenter.h"
#include "FramePresenterGLUT.h"

#if !defined (_WIN32)
#include "FramePresenterGLX.h"
#endif

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

/**
*   @brief  Function to decode media file pointed by "szInFilePath" parameter.
*           The decoded frames are displayed by using the OpenGL-CUDA interop.
*   @param  cuContext - Handle to CUDA context
*   @param  szInFilePath - Path to file to be decoded
*   @return 0 on failure
*   @return 1 on success
*/
int Decode(CUcontext cuContext, char *szInFilePath) {

    FFmpegDemuxer demuxer(szInFilePath);
    NvDecoder dec(cuContext, true, FFmpeg2NvCodecId(demuxer.GetVideoCodec()));

    // Presenter need aligned width
    int nWidth = (demuxer.GetWidth() + 1) & ~1;
    int nPitch = nWidth * 4;

#if defined (_WIN32)
    FramePresenterGLUT gInstance(cuContext, nWidth, demuxer.GetHeight());
#else
    FramePresenterGLX gInstance(nWidth, demuxer.GetHeight());
#endif

    int &nFrame = gInstance.nFrame;

    // Check whether we have valid NVIDIA libraries installed
    if (!gInstance.isVendorNvidia()) {
        std::cout<<"\nFailed to find NVIDIA libraries\n";
        return 0;
    }

    CUdeviceptr dpFrame;
    int nVideoBytes = 0, nFrameReturned = 0, iMatrix = 0;
    uint8_t *pVideo = NULL;
    uint8_t *pFrame;
    do {
        demuxer.Demux(&pVideo, &nVideoBytes);
        nFrameReturned = dec.Decode(pVideo, nVideoBytes);
        if (!nFrame && nFrameReturned)
            LOG(INFO) << dec.GetVideoInfo();

        for (int i = 0; i < nFrameReturned; i++) {
            pFrame = dec.GetFrame();
            gInstance.GetDeviceFrameBuffer(&dpFrame, &nPitch);
            iMatrix = dec.GetVideoFormatInfo().video_signal_description.matrix_coefficients;
            // Launch cuda kernels for colorspace conversion from raw video to raw image formats which OpenGL textures can work with
            if (dec.GetBitDepth() == 8) {
                if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                    YUV444ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t *)dpFrame, nPitch, dec.GetWidth(), dec.GetHeight(), iMatrix);
                else // default assumed NV12
                    Nv12ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t *)dpFrame, nPitch, dec.GetWidth(), dec.GetHeight(), iMatrix);
            }
            else {
                if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                    YUV444P16ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t *)dpFrame, nPitch, dec.GetWidth(), dec.GetHeight(), iMatrix);
                else // default assumed P016
                    P016ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t *)dpFrame, nPitch, dec.GetWidth(), dec.GetHeight(), iMatrix);
            }

            gInstance.ReleaseDeviceFrameBuffer();

        }
        nFrame += nFrameReturned;
    } while (nVideoBytes);

    std::cout << "Total frame decoded: " << nFrame << std::endl;
    return 1;
}

int main(int argc, char **argv)
{
    char szInFilePath[256] = "";
    int iGpu = 0;
    try
    {
        ParseCommandLine(argc, argv, szInFilePath, NULL, iGpu, NULL, NULL);
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

        CUcontext cuContext = NULL;
        createCudaContext(&cuContext, iGpu, CU_CTX_SCHED_BLOCKING_SYNC);

        std::cout << "Decode with NvDecoder." << std::endl;
        Decode(cuContext, szInFilePath);

        ck(cuCtxDestroy(cuContext));
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
