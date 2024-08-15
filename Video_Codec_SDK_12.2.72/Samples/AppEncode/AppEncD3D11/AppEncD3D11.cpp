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

/**
*  This sample application illustrates encoding of frames in ID3D11Texture2D textures.
*  There are 2 modes of operation demonstrated in this application.
*  In the default mode application reads RGB data from file and copies it to D3D11 textures
*  obtained from the encoder using NvEncoder::GetNextInputFrame() and the RGB texture is
*  submitted to NVENC for encoding. In the second case ("-nv12" option) the application converts
*  RGB textures to NV12 textures using DXVA's VideoProcessBlt API call and the NV12 texture is
*  submitted for encoding.
*
*  This sample application also illustrates the use of video memory buffer allocated 
*  by the application to get the NVENC hardware output. This feature can be used
*  for H264 ME-only mode, H264 encode, HEVC encode and AV1 encode.
*/

#include <d3d11.h>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <wrl.h>
#include "NvEncoder/NvEncoderD3D11.h"
#include "NvEncoder/NvEncoderOutputInVidMemD3D11.h"
#include "../Utils/Logger.h"
#include "../Utils/NvCodecUtils.h"
#include "../Common/AppEncUtils.h"
#include "../Common/AppEncUtilsD3D11.h"

using Microsoft::WRL::ComPtr;

// This class dumps the output to a file, when output in video memory is enabled.
// Output is first copied to host buffer and then dumped to a file.
class DumpVidMemOutput
{
public:
    DumpVidMemOutput(ID3D11Device *pDev, ID3D11DeviceContext *pCtx, uint32_t size)
    {
        pDevice = pDev;
        pContext = pCtx;
        bfrSize = size;

        ZeroMemory(&desc, sizeof(D3D11_BUFFER_DESC));

        desc.ByteWidth = size;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        if (pDevice->CreateBuffer(&desc, NULL, (ID3D11Buffer **)&pHostMem) != S_OK)
        {
            NVENC_THROW_ERROR("Failed to create ID3D11Buffer", NV_ENC_ERR_OUT_OF_MEMORY);
        }
    }

    ~DumpVidMemOutput()
    {
        pHostMem->Release();
    }

    void DumpOutputToFile(void * pVideoMemoryBuffer, std::ofstream &fpOut, bool &bWriteIVFFileHeader, NV_ENC_INITIALIZE_PARAMS &pInitializeParams)
    {
        pContext->CopyResource((ID3D11Buffer *)pHostMem, (ID3D11Buffer *)(pVideoMemoryBuffer));

        D3D11_MAPPED_SUBRESOURCE D3D11MappedResource;

        if (pContext->Map((ID3D11Buffer *)(pHostMem), 0, D3D11_MAP_READ, NULL, &D3D11MappedResource) != S_OK)
        {
            NVENC_THROW_ERROR("Failed to Map ID3D11Buffer", NV_ENC_ERR_INVALID_PTR);
        }

        uint8_t *pEncOutput = nullptr;
        
        pEncOutput = (uint8_t *)((uint8_t *)D3D11MappedResource.pData + sizeof(NV_ENC_ENCODE_OUT_PARAMS));
        NV_ENC_ENCODE_OUT_PARAMS *pEncOutParams = (NV_ENC_ENCODE_OUT_PARAMS *)D3D11MappedResource.pData;
        unsigned int bitstreamBufferSize = bfrSize - sizeof(NV_ENC_ENCODE_OUT_PARAMS);

        unsigned int numBytesToCopy = (pEncOutParams->bitstreamSizeInBytes > bitstreamBufferSize) ? bitstreamBufferSize : pEncOutParams->bitstreamSizeInBytes;

        IVFUtils iVFUtils;
        int64_t pts = 0;
        std::vector<uint8_t> vPacket;
        if (pInitializeParams.encodeGUID == NV_ENC_CODEC_AV1_GUID)
        {
            if (bWriteIVFFileHeader)
            {
                iVFUtils.WriteFileHeader(vPacket, MAKE_FOURCC('A', 'V', '0', '1'), pInitializeParams.encodeWidth, pInitializeParams.encodeHeight, pInitializeParams.frameRateNum, pInitializeParams.frameRateDen, 0xFFFF);
                fpOut.write(reinterpret_cast<char*>(vPacket.data()), vPacket.size());
                bWriteIVFFileHeader = false;
                vPacket.clear();
            }
            iVFUtils.WriteFrameHeader(vPacket, numBytesToCopy, pts);
            fpOut.write(reinterpret_cast<char*>(vPacket.data()), vPacket.size());
        }

        fpOut.write((const char *)pEncOutput, numBytesToCopy);

        pContext->Unmap((ID3D11Buffer *)(pHostMem), 0);
    }

private:
    ID3D11Device *pDevice;
    ID3D11DeviceContext *pContext;
    uint32_t bfrSize;
    ID3D11Buffer *pHostMem = NULL;
    D3D11_BUFFER_DESC desc;
};

template<class EncoderClass> 
void InitializeEncoder(EncoderClass &enc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat)
{
    NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

    initializeParams.encodeConfig = &encodeConfig;
    enc.CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());

    encodeCLIOptions.SetInitParams(&initializeParams, eFormat);

    enc.CreateEncoder(&initializeParams);
}

std::streamsize ReadInputFrame(const NvEncInputFrame* encoderInputFrame, std::ifstream &fpBgra, char *pHostFrame, ID3D11DeviceContext *pContext, RGBToNV12ConverterD3D11 *pConverter,
                               ID3D11Texture2D *pTexSysMem, int nSize, int nHeight, int nWidth, bool bForceNv12)
{
    std::streamsize nRead = fpBgra.read(pHostFrame, nSize).gcount();
    if (nRead == nSize)
    {
        D3D11_MAPPED_SUBRESOURCE map;
        ck(pContext->Map(pTexSysMem, D3D11CalcSubresource(0, 0, 1), D3D11_MAP_WRITE, 0, &map));
        for (int y = 0; y < nHeight; y++)
        {
            memcpy((uint8_t *)map.pData + y * map.RowPitch, pHostFrame + y * nWidth * 4, nWidth * 4);
        }
        pContext->Unmap(pTexSysMem, D3D11CalcSubresource(0, 0, 1));
        if (bForceNv12)
        {
            ID3D11Texture2D *pNV12Textyure = reinterpret_cast<ID3D11Texture2D*>(encoderInputFrame->inputPtr);
            pConverter->ConvertRGBToNV12(pTexSysMem, pNV12Textyure);
        }
        else
        {
            ID3D11Texture2D *pTexBgra = reinterpret_cast<ID3D11Texture2D*>(encoderInputFrame->inputPtr);
            pContext->CopyResource(pTexBgra, pTexSysMem);
        }    
    }
    return nRead;
}

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

void Encode(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, RGBToNV12ConverterD3D11 *pConverter, int nWidth, int nHeight,
            NvEncoderInitParam encodeCLIOptions, bool bForceNv12, ID3D11Texture2D *pTexSysMem, 
            std::ifstream &fpBgra, std::ofstream &fpOut)
{
    NvEncoderD3D11 enc(pDevice, nWidth, nHeight, bForceNv12 ? NV_ENC_BUFFER_FORMAT_NV12 : NV_ENC_BUFFER_FORMAT_ARGB);

    InitializeEncoder(enc, encodeCLIOptions, bForceNv12 ? NV_ENC_BUFFER_FORMAT_NV12 : NV_ENC_BUFFER_FORMAT_ARGB);

    int nSize = nWidth * nHeight * 4;
    std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nSize]);
    int nFrame = 0;

    while (true) 
    {
        std::vector<std::vector<uint8_t>> vPacket;
        const NvEncInputFrame* encoderInputFrame = enc.GetNextInputFrame(); 
        std::streamsize nRead = ReadInputFrame(encoderInputFrame, fpBgra, reinterpret_cast<char*>(pHostFrame.get()), pContext, pConverter, pTexSysMem,
                                               nSize, nHeight, nWidth, bForceNv12);

        if (nRead == nSize)
        {
            enc.EncodeFrame(vPacket);
        }
        else
        {
            enc.EndEncode(vPacket);
        }
        nFrame += (int)vPacket.size();
        for (std::vector<uint8_t> &packet : vPacket)
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

void EncodeOutputInVidMem(ID3D11Device *pDevice, ID3D11DeviceContext *pContext, RGBToNV12ConverterD3D11 *pConverter,
                          int nWidth, int nHeight, NvEncoderInitParam encodeCLIOptions, bool bForceNv12, 
                          ID3D11Texture2D *pTexSysMem, std::ifstream &fpBgra, std::ofstream &fpOut)
{
    NvEncoderOutputInVidMemD3D11 enc(pDevice, nWidth, nHeight, bForceNv12 ? NV_ENC_BUFFER_FORMAT_NV12 : NV_ENC_BUFFER_FORMAT_ARGB, false);

    InitializeEncoder(enc, encodeCLIOptions, bForceNv12 ? NV_ENC_BUFFER_FORMAT_NV12 : NV_ENC_BUFFER_FORMAT_ARGB);

    int nSize = nWidth * nHeight * 4;
    std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nSize]);
    int nFrame = 0;

    NV_ENC_INITIALIZE_PARAMS initializeParams = enc.GetinitializeParams();    
    bool bWriteIVFFileHeader = true;

    // For dumping output to a file
    std::unique_ptr<DumpVidMemOutput> pDumpVidMemOutput(new DumpVidMemOutput(pDevice, pContext, enc.GetOutputBufferSize()));

    while (true) 
    {
        std::vector<std::vector<uint8_t>> vPacket;
        std::vector<NV_ENC_OUTPUT_PTR> pVideoMemBfr;
        
        const NvEncInputFrame* encoderInputFrame = enc.GetNextInputFrame();
        std::streamsize nRead = ReadInputFrame(encoderInputFrame, fpBgra, reinterpret_cast<char*>(pHostFrame.get()), pContext, pConverter, pTexSysMem,
                                               nSize, nHeight, nWidth, bForceNv12);

        if (nRead == nSize)
        {
            enc.EncodeFrame(pVideoMemBfr);
        }
        else
        {
            enc.EndEncode(pVideoMemBfr);
        }
 
        for (uint32_t i = 0; i < pVideoMemBfr.size(); ++i)
        {
            pDumpVidMemOutput->DumpOutputToFile(pVideoMemBfr[i], fpOut, bWriteIVFFileHeader, initializeParams);

            nFrame++;
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
        bool bForceNv12 = false;
        int isOutputInVideoMem = 0;
 
        ParseCommandLine_AppEncD3D(argc, argv, szInFilePath, nWidth, nHeight, szOutFilePath, encodeCLIOptions, iGpu, bForceNv12, &isOutputInVideoMem, true);

        CheckInputFile(szInFilePath);
        
        //open input and output files
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

        ComPtr<ID3D11Device> pDevice;
        ComPtr<ID3D11DeviceContext> pContext;
        ComPtr<IDXGIFactory1> pFactory;
        ComPtr<IDXGIAdapter> pAdapter;

        ck(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)pFactory.GetAddressOf()));
        ck(pFactory->EnumAdapters(iGpu, pAdapter.GetAddressOf()));
        
        ck(D3D11CreateDevice(pAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, pDevice.GetAddressOf(), NULL, pContext.GetAddressOf()));
        
        DXGI_ADAPTER_DESC adapterDesc;
        pAdapter->GetDesc(&adapterDesc);
        char szDesc[80];
        wcstombs(szDesc, adapterDesc.Description, sizeof(szDesc));
        std::cout << "GPU in use: " << szDesc << std::endl;

        ComPtr<ID3D11Texture2D> pTexSysMem;
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
        desc.Width = nWidth;
        desc.Height = nHeight;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ck(pDevice->CreateTexture2D(&desc, NULL, pTexSysMem.GetAddressOf()));
        
        std::unique_ptr<RGBToNV12ConverterD3D11> pConverter;
        if (bForceNv12)
        {
            pConverter.reset(new RGBToNV12ConverterD3D11(pDevice.Get(), pContext.Get(), nWidth, nHeight));
        }

        if (isOutputInVideoMem)
        {
            EncodeOutputInVidMem(pDevice.Get(), pContext.Get(), pConverter.get(), nWidth, nHeight, encodeCLIOptions, 
                                 bForceNv12, pTexSysMem.Get(), fpBgra, fpOut);
        }
        else
        {
            Encode(pDevice.Get(), pContext.Get(), pConverter.get(), nWidth, nHeight, encodeCLIOptions, bForceNv12,
                pTexSysMem.Get(), fpBgra, fpOut);
        }

        std::cout << "Saved in file " << szOutFilePath << std::endl;
    }
    catch (const std::exception &ex)
    {
        std::cout << ex.what();
        exit(1);
    }
    return 0;
}
