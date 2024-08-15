/*
* Copyright 2024 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

/**
*  This sample application illustrates encoding of ID3D12Resource. This feature can be used
*  for H264 encode, HEVC encode and AV1 encode.
*/

#include <d3d12.h>
#include <windows.h>
#include <dxgi.h>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <wrl.h>
#include "../Utils/Logger.h"
#include "../Utils/NvCodecUtils.h"
#include "../Common/AppEncUtils.h"
#include "NvEncoder/NvEncoderD3D12.h"

// Set Agility SDK parameters

// D3D12SDKVersion is the SDK version of the D3D12Core.dll from the Agility SDK you are using
// See https://devblogs.microsoft.com/directx/directx12agility/
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = AGILITY_SDK_VER; }

// D3D12SDKPath is the path to the Agility SDK binaries you are using relative to the application exe
// On building AppEncD3D12, D3D12 directory containing D3D12Core.dll from the Agility SDK is generated (in the same directory as AppEncD3D12.exe)
// Make sure that you copy the generated D3D12 directory along with AppEncD3D12.exe if you move it to some other location
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\"; }

using Microsoft::WRL::ComPtr;

simplelogger::Logger* logger = simplelogger::LoggerFactory::CreateConsoleLogger();

// This class reads the input from a file.
// Input is first copied to upload buffer and then transferred to texture resource
class UploadInput
{
public:
    UploadInput(ID3D12Device* pDev, unsigned int numBfrs, unsigned int uploadBfrSize, unsigned int width, unsigned int height, NV_ENC_BUFFER_FORMAT bfrFormat)
    {
        pDevice = pDev;
        nWidth = width;
        nHeight = height;
        bufferFormat = bfrFormat;
        nInpBfrs = numBfrs;
        nCurIdx = 0;

        nFrameSize = nWidth * nHeight * 4;
        pHostFrame = std::unique_ptr<char[]>(new char[nFrameSize]);

        AllocateUploadBuffers(uploadBfrSize, nInpBfrs);

        D3D12_COMMAND_QUEUE_DESC gfxCommandQueueDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
        if (pDevice->CreateCommandQueue(&gfxCommandQueueDesc, IID_PPV_ARGS(&pGfxCommandQueue)) != S_OK)
        {
            NVENC_THROW_ERROR("Failed to create command queue", NV_ENC_ERR_OUT_OF_MEMORY);
        }

        vCmdAlloc.resize(numBfrs);
        for (unsigned int i = 0; i < numBfrs; ++i)
        {
            if (pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&vCmdAlloc[i])) != S_OK)
            {
                NVENC_THROW_ERROR("Failed to create command allocator", NV_ENC_ERR_OUT_OF_MEMORY);
            }
        }

        if (pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, vCmdAlloc[0].Get(), nullptr, IID_PPV_ARGS(&pGfxCommandList)) != S_OK)
        {
            NVENC_THROW_ERROR("Failed to create command list", NV_ENC_ERR_OUT_OF_MEMORY);
        }

        if (pGfxCommandList->Close() != S_OK)
        {
            NVENC_THROW_ERROR("Failed to create command queue", NV_ENC_ERR_OUT_OF_MEMORY);
        }
    }

    ~UploadInput() {}
    
    void AllocateUploadBuffers(unsigned int uploadBfrSize, unsigned int numBfrs)
    {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = uploadBfrSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        vUploadBfr.resize(numBfrs);
        for (unsigned int i = 0; i < numBfrs; i++)
        {
            if (pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                IID_PPV_ARGS(&vUploadBfr[i])) != S_OK)
            {
                NVENC_THROW_ERROR("Failed to create upload buffer", NV_ENC_ERR_OUT_OF_MEMORY);
            }
        }
    }

    void CopyToTexture(const NvEncInputFrame* encoderInputFrame, ID3D12Resource *pUploadBfr, ID3D12Fence* pInpFence, uint64_t* pInpFenceVal)
    {
        ID3D12Resource* pRsrc = (ID3D12Resource *)encoderInputFrame->inputPtr;
        ID3D12CommandAllocator* pGfxCommandAllocator = vCmdAlloc[nCurIdx % nInpBfrs].Get();
        D3D12_RESOURCE_DESC desc = pRsrc->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT inputUploadFootprint[2];

        pDevice->GetCopyableFootprints(&desc, 0, 1, 0, inputUploadFootprint, nullptr, nullptr, nullptr);

        if (pGfxCommandAllocator->Reset() != S_OK)
            NVENC_THROW_ERROR("Failed to reset command allocator", NV_ENC_ERR_OUT_OF_MEMORY);
                
        if (pGfxCommandList->Reset(pGfxCommandAllocator, NULL) != S_OK)
            NVENC_THROW_ERROR("Failed to reset command list", NV_ENC_ERR_OUT_OF_MEMORY);

        D3D12_RESOURCE_BARRIER barrier{};
        memset(&barrier, 0, sizeof(barrier));
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pRsrc;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = 0;

        pGfxCommandList->ResourceBarrier(1, &barrier);

        {
            D3D12_TEXTURE_COPY_LOCATION copyDst{};
            copyDst.pResource = pRsrc;
            copyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            copyDst.SubresourceIndex = 0;

            D3D12_TEXTURE_COPY_LOCATION copySrc{};
            copySrc.pResource = pUploadBfr;
            copySrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            copySrc.PlacedFootprint = inputUploadFootprint[0];

            pGfxCommandList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);
        }

        memset(&barrier, 0, sizeof(barrier));
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pRsrc;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
        barrier.Transition.Subresource = 0;

        pGfxCommandList->ResourceBarrier(1, &barrier);

        if (pGfxCommandList->Close() != S_OK)
            NVENC_THROW_ERROR("Failed to close command list", NV_ENC_ERR_OUT_OF_MEMORY);

        ID3D12CommandList* const ppCommandList[] = { pGfxCommandList.Get() };
       
        pGfxCommandQueue->ExecuteCommandLists(1, ppCommandList);

        InterlockedIncrement(pInpFenceVal);

        // Signal fence from GPU side, encode will wait on this fence before reading the input
        pGfxCommandQueue->Signal(pInpFence, *pInpFenceVal);
    }
    
    std::streamsize ReadInputFrame(std::ifstream& fpBgra, const NvEncInputFrame* encoderInputFrame, ID3D12Fence *pInpFence, uint64_t* pInpFenceVal)
    {
        std::streamsize nRead = fpBgra.read(pHostFrame.get(), nFrameSize).gcount();
        if (nRead == nFrameSize)
        {
            ID3D12Resource* pUploadBfr = vUploadBfr[nCurIdx % nInpBfrs].Get();
 
            void* pData = nullptr;
            ck(pUploadBfr->Map(0, nullptr, &pData));

            char* pDst = (char*)pData;
            char* pSrc = (char*)pHostFrame.get();
            unsigned int pitch = encoderInputFrame->pitch;
            for (unsigned int y = 0; y < nHeight; y++)
            {
                memcpy(pDst + y * pitch, pSrc + y * nWidth * 4, nWidth *4);
            }
            
            pUploadBfr->Unmap(0, nullptr);

            CopyToTexture(encoderInputFrame, pUploadBfr, pInpFence, pInpFenceVal);

            nCurIdx++;
        }
        return nRead;
    }

private:
    ID3D12Device* pDevice;
    unsigned int nWidth, nHeight, nFrameSize;
    unsigned int nInpBfrs, nCurIdx;
    std::unique_ptr<char[]> pHostFrame;
    NV_ENC_BUFFER_FORMAT bufferFormat;

    ComPtr<ID3D12GraphicsCommandList> pGfxCommandList;
    ComPtr<ID3D12CommandQueue> pGfxCommandQueue;

    std::vector<ComPtr<ID3D12CommandAllocator>> vCmdAlloc;
    std::vector<ComPtr<ID3D12Resource>> vUploadBfr;
};

void Encode(ID3D12Device* pDevice, int nWidth, int nHeight, NvEncoderInitParam encodeCLIOptions, std::ifstream& fpBgra, std::ofstream& fpOut)
{
    NV_ENC_BUFFER_FORMAT bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
    NvEncoderD3D12 enc(pDevice, nWidth, nHeight, bufferFormat);
    
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
    initializeParams.encodeConfig = &encodeConfig;
    
    enc.CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
    encodeCLIOptions.SetInitParams(&initializeParams, bufferFormat);

    enc.CreateEncoder(&initializeParams);

    int nSize = nWidth * nHeight * 4;
    std::unique_ptr<UploadInput> pUploadInput(new UploadInput(pDevice, enc.GetNumBfrs(), enc.GetInputSize(), nWidth, nHeight, bufferFormat));
    int nFrame = 0;

    while (true)
    {
        std::vector<std::vector<uint8_t>> vPacket;

        const NvEncInputFrame* encoderInputFrame = enc.GetNextInputFrame();

        std::streamsize nRead = pUploadInput->ReadInputFrame(fpBgra, encoderInputFrame, enc.GetInpFence(), enc.GetInpFenceValPtr());

        if (nRead == nSize)
        {
            enc.EncodeFrame(vPacket);
        }
        else
        {
            enc.EndEncode(vPacket);
        }
        nFrame += (int)vPacket.size();
        for (std::vector<uint8_t>& packet : vPacket)
        {
            fpOut.write(reinterpret_cast<char*>(packet.data()), packet.size());
        }
        if (nRead != nSize) {
            break;
        }
    }

    enc.DestroyEncoder();

    fpOut.close();
    fpBgra.close();

    std::cout << "Total frames encoded: " << nFrame << std::endl;
}

int main(int argc, char **argv)
{
    char szInFilePath[256] = "";
    char szOutFilePath[256] = "out.h264";
    int nWidth = 0, nHeight = 0;
    try
    {
        NvEncoderInitParam encodeCLIOptions;
        int iGpu = 0;
        bool bForceNV12 = false;
        ParseCommandLine_AppEncD3D(argc, argv, szInFilePath, nWidth, nHeight, szOutFilePath, encodeCLIOptions, iGpu, bForceNV12, nullptr, false, true);

        CheckInputFile(szInFilePath);
        
        std::ifstream fpBgra(szInFilePath, std::ifstream::in | std::ifstream::binary);
        if (!fpBgra)
        {
            std::ostringstream err;
            err << "Unable to open input file: " << szInFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }

        std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
        if (!fpOut)
        {
            std::ostringstream err;
            err << "Unable to open output file: " << szOutFilePath << std::endl;
            throw std::invalid_argument(err.str());
        }
    
        ValidateResolution(nWidth, nHeight);

        ComPtr<ID3D12Device> pDevice;
        ComPtr<IDXGIFactory1> pFactory;
        ComPtr<IDXGIAdapter> pAdapter;
        ComPtr<ID3D12Debug> debugController;

        ck(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)pFactory.GetAddressOf()));
        ck(pFactory->EnumAdapters(iGpu, pAdapter.GetAddressOf()));
     
#if defined(_DEBUG)
        // Enable the debug layer (requires the Graphics Tools "optional feature").
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
            }
        }
#endif
        ck(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(pDevice.GetAddressOf())));
        
        DXGI_ADAPTER_DESC adapterDesc;
        pAdapter->GetDesc(&adapterDesc);
        char szDesc[80];
        wcstombs(szDesc, adapterDesc.Description, sizeof(szDesc));
        std::cout << "GPU in use: " << szDesc << std::endl;

        Encode(pDevice.Get(), nWidth, nHeight, encodeCLIOptions, fpBgra, fpOut);

        std::cout << "Saved in file " << szOutFilePath << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
