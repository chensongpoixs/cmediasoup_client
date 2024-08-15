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
//! \file AppDecPerf.cpp
//! \brief Source file for AppDecPerf sample
//!
//!  This sample application measures decoding performance in FPS.
//!  The application creates multiple host threads and runs a different decoding session on each thread.
//!  The number of threads can be controlled by the CLI option "-thread".
//!  The application creates 2 host threads, each with a separate decode session, by default.
//!  The application supports measuring the decode performance only (keeping decoded
//!  frames in device memory as well as measuring the decode performance including transfer
//!  of frames to the host memory.
//---------------------------------------------------------------------------

#include <cuda.h>
#include <cudaProfiler.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <string.h>
#include <memory>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include <chrono>
#include <future>

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

struct SessionStats
{
    int64_t initTime;   // session initialization time
    int64_t decodeTime; // time taken by actual decoding operation
    int frames;     // number of frames decoded
};

using NvDecodePromise = std::promise<SessionStats>;
using NvDecodeFuture = std::future<SessionStats>;


class NvDecoderPerf : public NvDecoder
{
public:
    NvDecoderPerf(CUcontext cuContext, bool bUseDeviceFrame, cudaVideoCodec eCodec);
    void SetSessionInitTime(int64_t duration) { m_sessionInitTime = duration; }
    int64_t GetSessionInitTime() { return m_sessionInitTime; }

    static void IncrementSessionInitCounter() { m_sessionInitCounter++; }
    static uint32_t GetSessionInitCounter() { return m_sessionInitCounter; }
    static void SetSessionCount(uint32_t count) { m_sessionCount = count; }
    static uint32_t GetSessionCount(void) { return m_sessionCount; }

protected:
    int HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat);

    int64_t m_sessionInitTime;
    static std::mutex m_initMutex;
    static std::condition_variable m_cvInit;
    static uint32_t m_sessionInitCounter;
    static uint32_t m_sessionCount;
};

std::mutex NvDecoderPerf::m_initMutex;
std::condition_variable NvDecoderPerf::m_cvInit;
uint32_t NvDecoderPerf::m_sessionInitCounter = 0;
uint32_t NvDecoderPerf::m_sessionCount = 1;

NvDecoderPerf::NvDecoderPerf(CUcontext cuContext, bool bUseDeviceFrame, cudaVideoCodec eCodec)
: NvDecoder(cuContext, bUseDeviceFrame, eCodec)
{
}

int NvDecoderPerf::HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat)
{
    auto sessionStart = std::chrono::high_resolution_clock::now();

    int nDecodeSurface = NvDecoder::HandleVideoSequence(pVideoFormat);

    std::unique_lock<std::mutex> lock(m_initMutex);

    IncrementSessionInitCounter();

    // Wait for all threads to finish initialization of the decoder session.
    // This ensures that all threads start decoding frames at the same
    // time and saturate the decoder engines. This also leads to more
    // accurate measurement of decoding performance.
    if (GetSessionInitCounter() == GetSessionCount())
    {
        m_cvInit.notify_all();
    }
    else
    {
        m_cvInit.wait(lock, [] { return NvDecoderPerf::GetSessionInitCounter() >= NvDecoderPerf::GetSessionCount(); });
    }

    auto sessionEnd = std::chrono::high_resolution_clock::now();
    int64_t elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(sessionEnd - sessionStart).count();

    SetSessionInitTime(elapsedTime);
    return nDecodeSurface;
}


/**
*   @brief  Function to decode media file using NvDecoder interface
*   @param  pDec    - Handle to NvDecoder
*   @param  demuxer - Pointer to an FFmpegDemuxer instance
*   @param  pnFrame - Variable to record the number of frames decoded
*   @param  ex      - Stores current exception in case of failure
*/
void DecProc(NvDecoderPerf *pDec, FFmpegDemuxer *demuxer, NvDecodePromise& promise, std::exception_ptr &ex)
{
    SessionStats stats = {0, 0, 0};
    auto sessionStart = std::chrono::high_resolution_clock::now();

    try
    {
        int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
        uint8_t *pVideo = NULL, *pFrame = NULL;

        do {
            demuxer->Demux(&pVideo, &nVideoBytes);
            nFrameReturned = pDec->Decode(pVideo, nVideoBytes);
            if (!nFrame && nFrameReturned)
                LOG(INFO) << pDec->GetVideoInfo();

            nFrame += nFrameReturned;
        } while (nVideoBytes);
        stats.frames = nFrame;
        stats.initTime = pDec->GetSessionInitTime();
    }
    catch (std::exception&)
    {
        ex = std::current_exception();
    }

    auto sessionEnd = std::chrono::high_resolution_clock::now();
    stats.decodeTime = std::chrono::duration_cast<std::chrono::milliseconds>(sessionEnd - sessionStart).count();

    promise.set_value(stats);
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
        << "-gpu         Ordinal of GPU to use" << std::endl
        << "-thread      Number of decoding thread" << std::endl
        << "-single      (No value) Use single context (this may result in suboptimal performance; default is multiple contexts)" << std::endl
        << "-host        (No value) Copy frame to host memory (this may result in suboptimal performance; default is device memory)" << std::endl
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

void ParseCommandLine(int argc, char *argv[], char *szInputFileName, int &iGpu, int &nThread, bool &bSingle, bool &bHost) 
{
    for (int i = 1; i < argc; i++) {
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
        if (!_stricmp(argv[i], "-gpu")) {
            if (++i == argc) {
                ShowHelpAndExit("-gpu");
            }
            iGpu = atoi(argv[i]);
            continue;
        }
        if (!_stricmp(argv[i], "-thread")) {
            if (++i == argc) {
                ShowHelpAndExit("-thread");
            }
            nThread = atoi(argv[i]);
            continue;
        }
        if (!_stricmp(argv[i], "-single")) {
            bSingle = true;
            continue;
        }
        if (!_stricmp(argv[i], "-host")) {
            bHost = true;
            continue;
        }
        ShowHelpAndExit(argv[i]);
    }
}

struct NvDecPerfData
{
    uint8_t *pBuf;
    std::vector<uint8_t *> *pvpPacketData; 
    std::vector<int> *pvpPacketDataSize;
};

int CUDAAPI HandleVideoData(void *pUserData, CUVIDSOURCEDATAPACKET *pPacket) {
    NvDecPerfData *p = (NvDecPerfData *)pUserData;
    memcpy(p->pBuf, pPacket->payload, pPacket->payload_size);
    p->pvpPacketData->push_back(p->pBuf);
    p->pvpPacketDataSize->push_back(pPacket->payload_size);
    p->pBuf += pPacket->payload_size;
    return 1;
}

int main(int argc, char **argv)
{
    char szInFilePath[256] = "";
    int iGpu = 0;
    int nThread = 2; 
    bool bSingle = false;
    bool bHost = false;
    std::vector<std::exception_ptr> vExceptionPtrs;
    std::vector<NvDecodePromise> vPromise;
    std::vector<NvDecodeFuture> vFuture;

    try {
        ParseCommandLine(argc, argv, szInFilePath, iGpu, nThread, bSingle, bHost);
        CheckInputFile(szInFilePath);

        struct stat st;
        if (stat(szInFilePath, &st) != 0) {
            return 1;
        }
        int nBufSize = st.st_size;

        uint8_t *pBuf = NULL;
        try {
            pBuf = new uint8_t[nBufSize];
        }
        catch (std::bad_alloc) {
            std::cout << "Failed to allocate memory in BufferedReader" << std::endl;
            return 1;
        }
        std::vector<uint8_t *> vpPacketData;
        std::vector<int> vnPacketData;

        NvDecPerfData userData = { pBuf, &vpPacketData, &vnPacketData };

        ck(cuInit(0));
        int nGpu = 0;
        ck(cuDeviceGetCount(&nGpu));
        if (iGpu < 0 || iGpu >= nGpu) {
            std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
            return 1;
        }
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        std::cout << "GPU in use: " << szDeviceName << std::endl;

        std::vector<std::unique_ptr<FFmpegDemuxer>> vDemuxer;
        std::vector<std::unique_ptr<NvDecoderPerf>> vDec;
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));
        vExceptionPtrs.resize(nThread);
        vPromise.resize(nThread);

        for (int i = 0; i < nThread; i++)
        {
            if (!bSingle)
            {
                ck(cuCtxCreate(&cuContext, 0, cuDevice));
            }
            std::unique_ptr<FFmpegDemuxer> demuxer(new FFmpegDemuxer(szInFilePath));

            NvDecoderPerf* sessionObject = new NvDecoderPerf(cuContext, !bHost, FFmpeg2NvCodecId(demuxer->GetVideoCodec()));
            std::unique_ptr<NvDecoderPerf> dec(sessionObject);
            vDemuxer.push_back(std::move(demuxer));
            vDec.push_back(std::move(dec));
        }

        NvDecoderPerf::SetSessionCount(nThread);

        float totalFPS = 0;
        std::vector<NvThread> vThread;

        for (int i = 0; i < nThread; i++)
        {
            vThread.push_back(NvThread(std::thread(DecProc, vDec[i].get(), vDemuxer[i].get(), std::ref(vPromise[i]), std::ref(vExceptionPtrs[i]))));
            vFuture.push_back(vPromise[i].get_future());
        }

        int nTotal = 0;
        for (int i = 0; i < nThread; i++)
        {
            SessionStats stats = vFuture[i].get();
            nTotal += stats.frames;

            totalFPS += (stats.frames / ((stats.decodeTime - stats.initTime) / 1000.0f));

            vThread[i].join();
            vDec[i].reset(nullptr);
        }

        std::cout << "Total Frames Decoded=" << nTotal << " FPS = " << totalFPS << std::endl;

        ck(cuProfilerStop());

        for (int i = 0; i < nThread; i++)
        {
            if (vExceptionPtrs[i])
            {
                std::rethrow_exception(vExceptionPtrs[i]);
            }
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
