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
//! \file AppDecMem.cpp
//! \brief Source file for AppDecMem sample
//!
//! This sample application is similar to AppDec. It illustrates how to demux and decode media content from memory buffer.
//! It allocates AVIOContext explicitely and also defines method to read data packets from input file.
//! For simplicity, this application reads the input stream and stores it in a buffer before invoking the demuxer.
//---------------------------------------------------------------------------

#include <cuda.h>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "../Common/AppDecUtils.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

class FileDataProvider : public FFmpegDemuxer::DataProvider {
public:
    FileDataProvider(const char *szInFilePath) {
        fpIn.open(szInFilePath, std::ifstream::in | std::ifstream::binary);
        if (!fpIn)
        {
            std::cout << "Unable to open input file: " << szInFilePath << std::endl;
            return;
        }
    }
    ~FileDataProvider() {
        fpIn.close();
    }
    // Fill in the buffer owned by the demuxer/decoder
    int GetData(uint8_t *pBuf, int nBuf) {
        if (fpIn.eof())
        {
            return AVERROR_EOF;
        }

        // We read a file for this example. You may get your data from network or somewhere else
        return (int)fpIn.read(reinterpret_cast<char*>(pBuf), nBuf).gcount();
    }

private:
    std::ifstream fpIn;
};

int main(int argc, char *argv[])
{
    char szInFilePath[256] = "", szOutFilePath[256] = "out.yuv";
    int iGpu = 0;
    try
    {
        ParseCommandLine(argc, argv, szInFilePath, szOutFilePath, iGpu);
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

        FileDataProvider dp(szInFilePath);
        // Instead of passing in a media file path, here we pass in a DataProvider, which reads from the file.
        // Note that the data is passed into the demuxer chunk-by-chunk sequentially. If the meta data is at the end of the file
        // (as for MP4) and the buffer isn't large enough to hold the whole file, the file may never get demuxed.
        FFmpegDemuxer demuxer(&dp);
        NvDecoder dec(cuContext, false, FFmpeg2NvCodecId(demuxer.GetVideoCodec()));

        int nFrame = 0;
        uint8_t *pVideo = NULL;
        int nVideoBytes = 0;
        uint8_t *pFrame;
        int nFrameReturned = 0;
        std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
        if (!fpOut)
        {
            std::ostringstream err;
            err << "Unable to open output file: " << szOutFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }
        do
        {
            demuxer.Demux(&pVideo, &nVideoBytes);
            nFrameReturned = dec.Decode(pVideo, nVideoBytes);
            if (!nFrame && nFrameReturned)
                LOG(INFO) << dec.GetVideoInfo();

            nFrame += nFrameReturned;
            for (int i = 0; i < nFrameReturned; i++) {
                pFrame = dec.GetFrame();
                fpOut.write(reinterpret_cast<char*>(pFrame), dec.GetFrameSize());
            }
        } while (nVideoBytes);
        fpOut.close();
        const char *aszDecodeOutFormat[] = { "NV12", "P016", "YUV444", "YUV444P16" };
        std::cout << "Total frame decoded: " << nFrame << std::endl << "Saved in file " << szOutFilePath << " in format " << aszDecodeOutFormat[dec.GetOutputFormat()] << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
