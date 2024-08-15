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
//! \file AppDecLowLatency.cpp
//! \brief Source file for AppDecLowLatency sample
//!
//! This sample application demonstrates low latency decoding feature.
//! This feature helps to get output frame as soon as it is decoded without any delay.
//! The feature will work for streams having I and P frames only.
//---------------------------------------------------------------------------

#include <vector>
#include <cuda.h>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "../Common/AppDecUtils.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

int main(int argc, char *argv[]) 
{
    char szInFilePath[256] = "", szOutFilePath[256] = "out.yuv";
    int iGpu = 0;
    bool bVerbose = false;
    // With zero latency flag enabled, HandlePictureDisplay() callback happens immediately for 
    // the All-Intra/IPPP streams
    bool force_zero_latency = false;
    try
    {
        ParseCommandLine(argc, argv, szInFilePath, szOutFilePath, iGpu, &bVerbose, NULL, &force_zero_latency);
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
        createCudaContext(&cuContext, iGpu, 0);

        FFmpegDemuxer demuxer(szInFilePath);
        // Here set bLowLatency=true in the constructor.
        // Please don't use this flag except for low latency, it is harder to get 100% utilization of
        // hardware decoder with this flag set.
        NvDecoder dec(cuContext, false, FFmpeg2NvCodecId(demuxer.GetVideoCodec()), true, false, NULL, NULL, false, 0, 0, 1000, force_zero_latency);

        int nFrame = 0;
        uint8_t *pVideo = NULL;
        int nVideoBytes = 0;
        std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
        if (!fpOut)
        {
            std::ostringstream err;
            err << "Unable to open output file: " << szOutFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }

        int n = 0;
        bool bOneInOneOut = true;
        int nFrameReturned = 0;
        int64_t timestamp = 0;
        do {
            demuxer.Demux(&pVideo, &nVideoBytes);
            // Set flag CUVID_PKT_ENDOFPICTURE to signal that a complete packet has been sent to decode
            nFrameReturned = dec.Decode(pVideo, nVideoBytes, CUVID_PKT_ENDOFPICTURE, n++);
            if (!nFrame && nFrameReturned)
                LOG(INFO) << dec.GetVideoInfo();

            nFrame += nFrameReturned;
            // For a stream without B-frames, "one in and one out" is expected, and nFrameReturned should be always 1 for each input packet
            if (bVerbose)
            {
                std::cout << "Decode: nVideoBytes=" << nVideoBytes << ", nFrameReturned=" << nFrameReturned << ", total=" << nFrame << std::endl;
            }
            if (nVideoBytes && nFrameReturned != 1)
            {
                bOneInOneOut = false;
            }
            for (int i = 0; i < nFrameReturned; i++) 
            {
                fpOut.write(reinterpret_cast<char*>(dec.GetFrame(&timestamp)), dec.GetFrameSize());
                if (bVerbose)
                {
                    std::cout << "Timestamp: " << timestamp << std::endl;
                }
            }
        } while (nVideoBytes);

        fpOut.close();
        std::cout << "One packet in and one frame out: " << (bOneInOneOut ? "true" : "false") << std::endl;
    }
    catch(const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
