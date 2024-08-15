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
//! \file AppDecImageProvider.cpp
//! \brief Source file for AppDecImageProvider sample
//!
//! This sample application illustrates the decoding of a media file in a desired color format.
//! The application supports native (nv12 or p016), bgra, bgrp and bgra64 output formats.
//---------------------------------------------------------------------------

#include <cuda.h>
#include <iostream>
#include <algorithm>
#include <memory>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/FFmpegDemuxer.h"
#include "../Utils/ColorSpace.h"
#include "../Common/AppDecUtils.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

/**
*   @brief  Function to copy image data from CUDA device pointer to host buffer
*   @param  dpSrc   - CUDA device pointer which holds decoded raw frame
*   @param  pDst    - Pointer to host buffer which acts as the destination for the copy
*   @param  nWidth  - Width of decoded frame
*   @param  nHeight - Height of decoded frame
*/
void GetImage(CUdeviceptr dpSrc, uint8_t *pDst, int nWidth, int nHeight)
{
    CUDA_MEMCPY2D m = { 0 };
    m.WidthInBytes = nWidth;
    m.Height = nHeight;
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = (CUdeviceptr)dpSrc;
    m.srcPitch = m.WidthInBytes;
    m.dstMemoryType = CU_MEMORYTYPE_HOST;
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDst);
    m.dstPitch = m.WidthInBytes;
    cuMemcpy2D(&m);
}

enum OutputFormat
{
    native = 0, bgrp, rgbp, bgra, rgba, bgra64, rgba64
};

std::vector<std::string> vstrOutputFormatName =
{
    "native", "bgrp", "rgbp", "bgra", "rgba", "bgra64", "rgba64"
};

std::string GetSupportedFormats()
{
    std::ostringstream oss;
    for (auto& v : vstrOutputFormatName)
    {
        oss << " " << v;
    }

    return oss.str();
}

void ShowHelpAndExit(const char *szBadOption = NULL)
{
    std::ostringstream oss;
    bool bThrowError = false;
    if (szBadOption)
    {
        bThrowError = true;
        oss << "Error parsing \"" << szBadOption << "\"" << std::endl;
    }
    oss << "Options:" << std::endl
        << "-i           Input file path" << std::endl
        << "-o           Output file path" << std::endl
        << "-of          Output format:"<< GetSupportedFormats() << std::endl
        << "-gpu         Ordinal of GPU to use" << std::endl
        ;
    if (bThrowError)
    {
        throw std::invalid_argument(oss.str());
    }
    else
    {
        std::cout << oss.str();
        exit(0);
    }
}

void ParseCommandLine(int argc, char *argv[], char *szInputFileName,
    OutputFormat &eOutputFormat, char *szOutputFileName, int &iGpu)
{
    std::ostringstream oss;
    int i;
    for (i = 1; i < argc; i++) {
        if (!_stricmp(argv[i], "-h")) {
            ShowHelpAndExit();
        }
        if (!_stricmp(argv[i], "-i")) {
            if (++i == argc) {
                ShowHelpAndExit("-i");
            }
            sprintf(szInputFileName, "%s", argv[i]);
            continue;
        }
        if (!_stricmp(argv[i], "-o")) {
            if (++i == argc) {
                ShowHelpAndExit("-o");
            }
            sprintf(szOutputFileName, "%s", argv[i]);
            continue;
        }
        if (!_stricmp(argv[i], "-of")) {
            if (++i == argc) {
                ShowHelpAndExit("-of");
            }
            auto it = find(vstrOutputFormatName.begin(), vstrOutputFormatName.end(), argv[i]);
            if (it == vstrOutputFormatName.end()) {
                ShowHelpAndExit("-of");
            }
            eOutputFormat = (OutputFormat)(it - vstrOutputFormatName.begin());
            continue;
        }
        if (!_stricmp(argv[i], "-gpu")) {
            if (++i == argc) {
                ShowHelpAndExit("-gpu");
            }
            iGpu = atoi(argv[i]);
            continue;
        }
        ShowHelpAndExit(argv[i]);
    }
}

int main(int argc, char **argv)
{
    char szInFilePath[256] = "", szOutFilePath[256] = "";
    OutputFormat eOutputFormat = native;
    int iGpu = 0;
    bool bReturn = 1;
    CUdeviceptr pTmpImage = 0;

    try
    {
        ParseCommandLine(argc, argv, szInFilePath, eOutputFormat, szOutFilePath, iGpu);
        CheckInputFile(szInFilePath);

        if (!*szOutFilePath)
        {
            sprintf(szOutFilePath, "out.%s", vstrOutputFormatName[eOutputFormat].c_str());
        }

        std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
        if (!fpOut)
        {
            std::ostringstream err;
            err << "Unable to open output file: " << szOutFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }

        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu)
        {
            std::ostringstream err;
            err << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]";
            throw std::invalid_argument(err.str());
        }

        CUcontext cuContext = NULL;
        createCudaContext(&cuContext, iGpu, 0);

        FFmpegDemuxer demuxer(szInFilePath);
        NvDecoder dec(cuContext, true, FFmpeg2NvCodecId(demuxer.GetVideoCodec()));
        int nWidth = 0, nHeight = 0, nFrameSize = 0;
        int anSize[] = { 0, 3, 3, 4, 4, 8, 8 };
        std::unique_ptr<uint8_t[]> pImage;

        int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0, iMatrix = 0;
        uint8_t *pVideo = nullptr;
        uint8_t *pFrame;

        do {
            demuxer.Demux(&pVideo, &nVideoBytes);
            nFrameReturned = dec.Decode(pVideo, nVideoBytes);
            if (!nFrame && nFrameReturned)
            {
                LOG(INFO) << dec.GetVideoInfo();
                // Get output frame size from decoder
                nWidth = dec.GetWidth(); nHeight = dec.GetHeight();
                nFrameSize = eOutputFormat == native ? dec.GetFrameSize() : nWidth * nHeight * anSize[eOutputFormat];
                std::unique_ptr<uint8_t[]> pTemp(new uint8_t[nFrameSize]);
                pImage = std::move(pTemp);
                cuMemAlloc(&pTmpImage, nWidth * nHeight * anSize[eOutputFormat]);
            }

            for (int i = 0; i < nFrameReturned; i++)
            {
                iMatrix = dec.GetVideoFormatInfo().video_signal_description.matrix_coefficients;
                pFrame = dec.GetFrame();
                if (dec.GetBitDepth() == 8) {
                    switch (eOutputFormat) {
                    case native:
                        GetImage((CUdeviceptr)pFrame, reinterpret_cast<uint8_t*>(pImage.get()), dec.GetWidth(), dec.GetHeight() + (dec.GetChromaHeight() * dec.GetNumChromaPlanes()));
                        break;
                    case bgrp:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColorPlanar<BGRA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColorPlanar<BGRA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), dec.GetWidth(), 3 * dec.GetHeight());
                        break;
                    case rgbp:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColorPlanar<RGBA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColorPlanar<RGBA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), dec.GetWidth(), 3 * dec.GetHeight());
                        break;
                    case bgra:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColor32<BGRA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 4 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case rgba:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColor32<RGBA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColor32<RGBA32>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 4 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case bgra64:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColor64<BGRA64>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColor64<BGRA64>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 8 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case rgba64:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
                            YUV444ToColor64<RGBA64>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            Nv12ToColor64<RGBA64>(pFrame, dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 8 * dec.GetWidth(), dec.GetHeight());
                        break;
                    }
                }
                else
                {
                    switch (eOutputFormat) {
                    case native:
                        GetImage((CUdeviceptr)pFrame, reinterpret_cast<uint8_t*>(pImage.get()), 2 * dec.GetWidth(), dec.GetHeight() + (dec.GetChromaHeight() * dec.GetNumChromaPlanes()));
                        break;
                    case bgrp:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColorPlanar<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColorPlanar<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), dec.GetWidth(), 3 * dec.GetHeight());
                        break;
                    case rgbp:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColorPlanar<RGBA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColorPlanar<RGBA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), dec.GetWidth(), 3 * dec.GetHeight());
                        break;
                    case bgra:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColor32<BGRA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 4 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case rgba:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColor32<RGBA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColor32<RGBA32>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 4 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 4 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case bgra64:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColor64<BGRA64>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColor64<BGRA64>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 8 * dec.GetWidth(), dec.GetHeight());
                        break;
                    case rgba64:
                        if (dec.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
                            YUV444P16ToColor64<RGBA64>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        else
                            P016ToColor64<RGBA64>(pFrame, 2 * dec.GetWidth(), (uint8_t*)pTmpImage, 8 * dec.GetWidth(), dec.GetWidth(), dec.GetHeight(), iMatrix);
                        GetImage(pTmpImage, reinterpret_cast<uint8_t*>(pImage.get()), 8 * dec.GetWidth(), dec.GetHeight());
                        break;
                    }
                }

                fpOut.write(reinterpret_cast<char*>(pImage.get()), nFrameSize);
            }
            nFrame += nFrameReturned;
        } while (nVideoBytes);

        if (pTmpImage) {
            cuMemFree(pTmpImage);
        }

        std::cout << "Total frame decoded: " << nFrame << std::endl << "Saved in file " << szOutFilePath << std::endl;
        fpOut.close();
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
