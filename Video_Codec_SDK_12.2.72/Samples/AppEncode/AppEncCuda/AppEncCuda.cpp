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
*  This sample application illustrates encoding of frames in CUDA device buffers.
*  The application reads the image data from file and loads it to CUDA input
*  buffers obtained from the encoder using NvEncoder::GetNextInputFrame().
*  The encoder subsequently maps the CUDA buffers for encoder using NvEncodeAPI
*  and submits them to NVENC hardware for encoding as part of EncodeFrame() function.
*  The NVENC hardware output is written in system memory for this case.
*
*  This sample application also illustrates the use of video memory buffer allocated
*  by the application to get the NVENC hardware output. This feature can be used
*  for H264 ME-only mode, H264 encode and HEVC encode. This application copies the NVENC output
*  from video memory buffer to host memory buffer in order to dump to a file, but this
*  is not needed if application choose to use it in some other way.
*
*  Since, encoding may involve CUDA pre-processing on the input and post-processing on
*  output, use of CUDA streams is also illustrated to pipeline the CUDA pre-processing
*  and post-processing tasks, for output in video memory case.
*
*  CUDA streams can be used for H.264 ME-only, HEVC ME-only, H264 encode, HEVC encode and AV1 encode.
*/

#include <fstream>
#include <iostream>
#include <memory>
#include <cuda.h>
#include "../Utils/NvCodecUtils.h"
#include "NvEncoder/NvEncoderCuda.h"
#include "NvEncoder/NvEncoderOutputInVidMemCuda.h"
#include "../Utils/Logger.h"
#include "../Utils/NvEncoderCLIOptions.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

// This class allocates CUStream.
// It also sets the input and output CUDA stream in the driver, which will be used for pipelining
// pre and post processing CUDA tasks
class NvCUStream
{
public:
	NvCUStream(CUcontext cuDevice, int cuStreamType, std::unique_ptr<NvEncoderOutputInVidMemCuda> &pEnc)
	{
		device = cuDevice;
		CUDA_DRVAPI_CALL(cuCtxPushCurrent(device));

		// Create CUDA streams
		if (cuStreamType == 1)
		{
			ck(cuStreamCreate(&inputStream, CU_STREAM_DEFAULT));
			outputStream = inputStream;
		}
		else if (cuStreamType == 2)
		{
			ck(cuStreamCreate(&inputStream, CU_STREAM_DEFAULT));
			ck(cuStreamCreate(&outputStream, CU_STREAM_DEFAULT));
		}

		CUDA_DRVAPI_CALL(cuCtxPopCurrent(NULL));

		// Set input and output CUDA streams in driver
		pEnc->SetIOCudaStreams((NV_ENC_CUSTREAM_PTR)&inputStream, (NV_ENC_CUSTREAM_PTR)&outputStream);
	}

	~NvCUStream()
	{
		ck(cuCtxPushCurrent(device));

		if (inputStream == outputStream)
		{
			if (inputStream != NULL)
				ck(cuStreamDestroy(inputStream));
		}
		else
		{
			if (inputStream != NULL)
				ck(cuStreamDestroy(inputStream));

			if (outputStream != NULL)
				ck(cuStreamDestroy(outputStream));
		}

		ck(cuCtxPopCurrent(NULL));
	}

	CUstream GetOutputCUStream() { return outputStream; };
	CUstream GetInputCUStream() { return inputStream; };

private:
	CUcontext device;
	CUstream inputStream = NULL, outputStream = NULL;
};

// This class computes CRC of encode frame using CUDA kernel
class CRC
{
public:
	CRC(CUcontext cuDevice, uint32_t bufferSize)
	{
		device = cuDevice;

		ck(cuCtxPushCurrent(device));

		// Allocate video memory buffer to store CRC of encoded frame
		ck(cuMemAlloc(&crcVidMem, bufferSize));

		ck(cuCtxPopCurrent(NULL));
	}

	~CRC()
	{
		ck(cuCtxPushCurrent(device));

		ck(cuMemFree(crcVidMem));

		ck(cuCtxPopCurrent(NULL));
	}

	void GetCRC(NV_ENC_OUTPUT_PTR pVideoMemBfr, CUstream outputStream)
	{
		ComputeCRC((uint8_t *)pVideoMemBfr, (uint32_t *)crcVidMem, outputStream);
	}

	CUdeviceptr GetCRCVidMemPtr() { return crcVidMem; };

private:
	CUcontext device;
	CUdeviceptr crcVidMem = 0;
};

// This class dumps the output - CRC and encoded stream, to a file.
// Output is first copied to host buffer and then dumped to a file.
class DumpVidMemOutput
{
public:
	DumpVidMemOutput(CUcontext cuDevice, uint32_t size, char *outFilePath, bool bUseCUStream)
	{
		device = cuDevice;
		bfrSize = size;
		bCRC = bUseCUStream;

		ck(cuCtxPushCurrent(device));

		// Allocate host memory buffer to copy encoded output and CRC
		ck(cuMemAllocHost((void **)&pHostMemEncOp, (bfrSize + (bCRC ? 4 : 0))));

		ck(cuCtxPopCurrent(NULL));

		// Open file to dump CRC 
		if (bCRC)
		{
			crcFile = std::string(outFilePath) + "_crc.txt";
			fpCRCOut.open(crcFile, std::ios::out);
			pHostMemCRC = (uint32_t *)((uint8_t *)pHostMemEncOp + bfrSize);
		}
	}

	~DumpVidMemOutput()
	{
		ck(cuCtxPushCurrent(device));

		ck(cuMemFreeHost(pHostMemEncOp));

		ck(cuCtxPopCurrent(NULL));

		if (bCRC)
		{
			fpCRCOut.close();
			std::cout << "CRC saved in file: " << crcFile << std::endl;
		}
	}

	void DumpOutputToFile(CUdeviceptr pEncFrameBfr, CUdeviceptr pCRCBfr, std::ofstream &fpOut, uint32_t nFrame, bool &bWriteIVFFileHeader, NV_ENC_INITIALIZE_PARAMS &pInitializeParams)
	{
		ck(cuCtxPushCurrent(device));

		// Copy encoded frame from video memory buffer to host memory buffer
		ck(cuMemcpyDtoH(pHostMemEncOp, pEncFrameBfr, bfrSize));

		// Copy encoded frame CRC from video memory buffer to host memory buffer
		if (bCRC)
		{
			ck(cuMemcpyDtoH(pHostMemCRC, pCRCBfr, 4));
		}

		ck(cuCtxPopCurrent(NULL));

		// Write encoded bitstream in file
		uint32_t offset = sizeof(NV_ENC_ENCODE_OUT_PARAMS);
		uint32_t bitstream_size = ((NV_ENC_ENCODE_OUT_PARAMS *)pHostMemEncOp)->bitstreamSizeInBytes;
		uint8_t * ptr = pHostMemEncOp + offset;

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
			iVFUtils.WriteFrameHeader(vPacket, bitstream_size, pts);
			fpOut.write(reinterpret_cast<char*>(vPacket.data()), vPacket.size());
		}

		fpOut.write((const char *)ptr, bitstream_size);

		// Write CRC in file
		if (bCRC)
		{
			if (!nFrame)
			{
				fpCRCOut << "Frame num" << std::setw(10) << "CRC" << std::endl;
			}
			fpCRCOut << std::dec << std::setfill(' ') << std::setw(5) << nFrame << "          ";
			fpCRCOut << std::hex << std::setfill('0') << std::setw(8) << *pHostMemCRC << std::endl;
		}
	}
private:
	CUcontext device;
	uint32_t bfrSize;
	uint8_t *pHostMemEncOp = NULL;
	uint32_t *pHostMemCRC = NULL;
	bool bCRC;
	std::string crcFile;
	std::ofstream fpCRCOut;
};

void ShowEncoderCapability()
{
    ck(cuInit(0));
    int nGpu = 0;
    ck(cuDeviceGetCount(&nGpu));
    std::cout << "Encoder Capability" << std::endl << std::endl;
    for (int iGpu = 0; iGpu < nGpu; iGpu++) {
        CUdevice cuDevice = 0;
        ck(cuDeviceGet(&cuDevice, iGpu));
        char szDeviceName[80];
        ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
        CUcontext cuContext = NULL;
        ck(cuCtxCreate(&cuContext, 0, cuDevice));
        NvEncoderCuda enc(cuContext, 1280, 720, NV_ENC_BUFFER_FORMAT_NV12);

        std::cout << "GPU " << iGpu << " - " << szDeviceName << std::endl << std::endl;
        std::cout << "\tH264:\t\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
            NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
            "\tH264_444:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
            NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
            "\tH264_ME:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
            NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl <<
            "\tH264_WxH:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID,
            NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_H264_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl <<
            "\tHEVC:\t\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
            "\tHEVC_Main10:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORT_10BIT_ENCODE) ? "yes" : "no") << std::endl <<
            "\tHEVC_Lossless:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE) ? "yes" : "no") << std::endl <<
            "\tHEVC_SAO:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORT_SAO) ? "yes" : "no") << std::endl <<
            "\tHEVC_444:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
            "\tHEVC_ME:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_SUPPORT_MEONLY_MODE) ? "yes" : "no") << std::endl <<
            "\tHEVC_WxH:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID,
            NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_HEVC_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl <<
            "\tAV1:\t\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
            NV_ENC_CAPS_SUPPORTED_RATECONTROL_MODES) ? "yes" : "no") << std::endl <<
            "\tAV1_444:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
            NV_ENC_CAPS_SUPPORT_YUV444_ENCODE) ? "yes" : "no") << std::endl <<
            "\tAV1_WxH:\t" << "  " <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID,
            NV_ENC_CAPS_WIDTH_MAX)) << "*" <<
            (enc.GetCapabilityValue(NV_ENC_CODEC_AV1_GUID, NV_ENC_CAPS_HEIGHT_MAX)) << std::endl;

        std::cout << std::endl;

        enc.DestroyEncoder();
        ck(cuCtxDestroy(cuContext));
	}
}

void ShowHelpAndExit(const char *szBadOption = NULL)
{
	bool bThrowError = false;
	std::ostringstream oss;
	if (szBadOption)
	{
		bThrowError = true;
		oss << "Error parsing \"" << szBadOption << "\"" << std::endl;
	}
	oss << "Options:" << std::endl
		<< "-i               Input file path" << std::endl
		<< "-o               Output file path" << std::endl
		<< "-s               Input resolution in this form: WxH" << std::endl
		<< "-if              Input format: iyuv nv12 yuv444 p010 yuv444p16 bgra bgra10 ayuv abgr abgr10" << std::endl
		<< "-gpu             Ordinal of GPU to use" << std::endl
		<< "-outputInVidMem  Set this to 1 to enable output in Video Memory" << std::endl
		<< "-cuStreamType    Use CU stream for pre and post processing when outputInVidMem is set to 1" << std::endl
		<< "                 CRC of encoded frames will be computed and dumped to file with suffix '_crc.txt' added" << std::endl
		<< "                 to file specified by -o option " << std::endl
		<< "                 0 : both pre and post processing are on NULL CUDA stream" << std::endl
		<< "                 1 : both pre and post processing are on SAME CUDA stream" << std::endl
		<< "                 2 : both pre and post processing are on DIFFERENT CUDA stream" << std::endl
		;
	oss << NvEncoderInitParam().GetHelpMessage() << std::endl;
	if (bThrowError)
	{
		throw std::invalid_argument(oss.str());
	}
	else
	{
		std::cout << oss.str();
		ShowEncoderCapability();
		exit(0);
	}
}

void ParseCommandLine(int argc, char *argv[], char *szInputFileName, int &nWidth, int &nHeight,
	NV_ENC_BUFFER_FORMAT &eFormat, char *szOutputFileName, NvEncoderInitParam &initParam, int &iGpu,
	bool &bOutputInVidMem, int32_t &cuStreamType)
{
	std::ostringstream oss;
	int i;
	for (i = 1; i < argc; i++)
	{
		if (!_stricmp(argv[i], "-h"))
		{
			ShowHelpAndExit();
		}
		if (!_stricmp(argv[i], "-i"))
		{
			if (++i == argc)
			{
				ShowHelpAndExit("-i");
			}
			sprintf(szInputFileName, "%s", argv[i]);
			continue;
		}
		if (!_stricmp(argv[i], "-o"))
		{
			if (++i == argc)
			{
				ShowHelpAndExit("-o");
			}
			sprintf(szOutputFileName, "%s", argv[i]);
			continue;
		}
		if (!_stricmp(argv[i], "-s"))
		{
			if (++i == argc || 2 != sscanf(argv[i], "%dx%d", &nWidth, &nHeight))
			{
				ShowHelpAndExit("-s");
			}
			continue;
		}
		std::vector<std::string> vszFileFormatName =
		{
			"iyuv", "nv12", "yv12", "yuv444", "p010", "yuv444p16", "bgra", "bgra10", "ayuv", "abgr", "abgr10"
		};
		NV_ENC_BUFFER_FORMAT aFormat[] =
		{
			NV_ENC_BUFFER_FORMAT_IYUV,
			NV_ENC_BUFFER_FORMAT_NV12,
			NV_ENC_BUFFER_FORMAT_YV12,
			NV_ENC_BUFFER_FORMAT_YUV444,
			NV_ENC_BUFFER_FORMAT_YUV420_10BIT,
			NV_ENC_BUFFER_FORMAT_YUV444_10BIT,
			NV_ENC_BUFFER_FORMAT_ARGB,
			NV_ENC_BUFFER_FORMAT_ARGB10,
			NV_ENC_BUFFER_FORMAT_AYUV,
			NV_ENC_BUFFER_FORMAT_ABGR,
			NV_ENC_BUFFER_FORMAT_ABGR10,
		};
		if (!_stricmp(argv[i], "-if"))
		{
			if (++i == argc) {
				ShowHelpAndExit("-if");
			}
			auto it = std::find(vszFileFormatName.begin(), vszFileFormatName.end(), argv[i]);
			if (it == vszFileFormatName.end())
			{
				ShowHelpAndExit("-if");
			}
			eFormat = aFormat[it - vszFileFormatName.begin()];
			continue;
		}
		if (!_stricmp(argv[i], "-gpu"))
		{
			if (++i == argc)
			{
				ShowHelpAndExit("-gpu");
			}
			iGpu = atoi(argv[i]);
			continue;
		}
		if (!_stricmp(argv[i], "-outputInVidMem"))
		{
			if (++i == argc)
			{
				ShowHelpAndExit("-outputInVidMem");
			}
			bOutputInVidMem = (atoi(argv[i]) != 0) ? true : false;
			continue;
		}
		if (!_stricmp(argv[i], "-cuStreamType"))
		{
			if (++i == argc)
			{
				ShowHelpAndExit("-cuStreamType");
			}
			cuStreamType = atoi(argv[i]);
			continue;
		}

		// Regard as encoder parameter
		if (argv[i][0] != '-')
		{
			ShowHelpAndExit(argv[i]);
		}
		oss << argv[i] << " ";
		while (i + 1 < argc && argv[i + 1][0] != '-')
		{
			oss << argv[++i] << " ";
		}
	}
	initParam = NvEncoderInitParam(oss.str().c_str());
}

template<class EncoderClass>
void InitializeEncoder(EncoderClass &pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat)
{
	NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

	initializeParams.encodeConfig = &encodeConfig;
	pEnc->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
	encodeCLIOptions.SetInitParams(&initializeParams, eFormat);

	pEnc->CreateEncoder(&initializeParams);
}

void EncodeCuda(int nWidth, int nHeight, NV_ENC_BUFFER_FORMAT eFormat, NvEncoderInitParam encodeCLIOptions, CUcontext cuContext, std::ifstream &fpIn, std::ofstream &fpOut)
{
	std::unique_ptr<NvEncoderCuda> pEnc(new NvEncoderCuda(cuContext, nWidth, nHeight, eFormat));

	InitializeEncoder(pEnc, encodeCLIOptions, eFormat);

	int nFrameSize = pEnc->GetFrameSize();

	std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nFrameSize]);
	int nFrame = 0;
	while (true)
	{
		// Load the next frame from disk
		std::streamsize nRead = fpIn.read(reinterpret_cast<char*>(pHostFrame.get()), nFrameSize).gcount();
		// For receiving encoded packets
		std::vector<std::vector<uint8_t>> vPacket;
		if (nRead == nFrameSize)
		{
			const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
			NvEncoderCuda::CopyToDeviceFrame(cuContext, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
				(int)encoderInputFrame->pitch,
				pEnc->GetEncodeWidth(),
				pEnc->GetEncodeHeight(),
				CU_MEMORYTYPE_HOST,
				encoderInputFrame->bufferFormat,
				encoderInputFrame->chromaOffsets,
				encoderInputFrame->numChromaPlanes);

			pEnc->EncodeFrame(vPacket);
		}
		else
		{
			pEnc->EndEncode(vPacket);
		}
		nFrame += (int)vPacket.size();
		for (std::vector<uint8_t> &packet : vPacket)
		{
			// For each encoded packet
			fpOut.write(reinterpret_cast<char*>(packet.data()), packet.size());
		}

		if (nRead != nFrameSize) break;
	}

	pEnc->DestroyEncoder();

	std::cout << "Total frames encoded: " << nFrame << std::endl;
}

void EncodeCudaOpInVidMem(int nWidth, int nHeight, NV_ENC_BUFFER_FORMAT eFormat, NvEncoderInitParam encodeCLIOptions, CUcontext cuContext, std::ifstream &fpIn, std::ofstream &fpOut, char *outFilePath, int32_t cuStreamType)
{
	std::unique_ptr<NvEncoderOutputInVidMemCuda> pEnc(new NvEncoderOutputInVidMemCuda(cuContext, nWidth, nHeight, eFormat));

	InitializeEncoder(pEnc, encodeCLIOptions, eFormat);

	int nFrameSize = pEnc->GetFrameSize();
	bool bUseCUStream = cuStreamType != -1 ? true : false;

	NV_ENC_INITIALIZE_PARAMS initializeParams = pEnc->GetinitializeParams();
	bool bWriteIVFFileHeader = true;

	std::unique_ptr<CRC> pCRC;
	std::unique_ptr<NvCUStream> pCUStream;
	if (bUseCUStream)
	{
		// Allocate CUDA streams
		pCUStream.reset(new NvCUStream(reinterpret_cast<CUcontext>(pEnc->GetDevice()), cuStreamType, pEnc));

		// When CUDA streams are used, the encoded frame's CRC is computed using cuda kernel
		pCRC.reset(new CRC(reinterpret_cast<CUcontext>(pEnc->GetDevice()), pEnc->GetOutputBufferSize()));
	}

	// For dumping output - encoded frame and CRC, to a file
	std::unique_ptr<DumpVidMemOutput> pDumpVidMemOutput(new DumpVidMemOutput(reinterpret_cast<CUcontext>(pEnc->GetDevice()), pEnc->GetOutputBufferSize(), outFilePath, bUseCUStream));

	std::unique_ptr<uint8_t[]> pHostFrame(new uint8_t[nFrameSize]);
	int nFrame = 0;

	// Encoding loop
	while (true)
	{
		// Load the next frame from disk
		std::streamsize nRead = fpIn.read(reinterpret_cast<char*>(pHostFrame.get()), nFrameSize).gcount();
		// For receiving encoded packets
		std::vector<NV_ENC_OUTPUT_PTR>pVideoMemBfr;

		if (nRead == nFrameSize)
		{
			const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
			NvEncoderCuda::CopyToDeviceFrame(cuContext, pHostFrame.get(), 0, (CUdeviceptr)encoderInputFrame->inputPtr,
				(int)encoderInputFrame->pitch,
				pEnc->GetEncodeWidth(),
				pEnc->GetEncodeHeight(),
				CU_MEMORYTYPE_HOST,
				encoderInputFrame->bufferFormat,
				encoderInputFrame->chromaOffsets,
				encoderInputFrame->numChromaPlanes,
				false,
				bUseCUStream ? pCUStream->GetInputCUStream() : NULL);

			pEnc->EncodeFrame(pVideoMemBfr);
		}
		else
		{
			pEnc->EndEncode(pVideoMemBfr);
		}

		for (uint32_t i = 0; i < pVideoMemBfr.size(); ++i)
		{
			if (bUseCUStream)
			{
				// Compute CRC of encoded stream
				pCRC->GetCRC(pVideoMemBfr[i], pCUStream->GetOutputCUStream());
			}

			pDumpVidMemOutput->DumpOutputToFile((CUdeviceptr)(pVideoMemBfr[i]), bUseCUStream ? pCRC->GetCRCVidMemPtr() : 0, fpOut, nFrame, bWriteIVFFileHeader, initializeParams);

			nFrame++;
		}

		if (nRead != nFrameSize) break;
	}

	pEnc->DestroyEncoder();

	std::cout << "Total frames encoded: " << nFrame << std::endl;
}

int main(int argc, char **argv)
{
	char szInFilePath[256] = "",
		szOutFilePath[256] = "";
	int nWidth = 0, nHeight = 0;
	NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV;
	int iGpu = 0;
	try
	{
		NvEncoderInitParam encodeCLIOptions;
		int cuStreamType = -1;
		bool bOutputInVideoMem = false;
		ParseCommandLine(argc, argv, szInFilePath, nWidth, nHeight, eFormat, szOutFilePath, encodeCLIOptions, iGpu,
			bOutputInVideoMem, cuStreamType);

		CheckInputFile(szInFilePath);
		ValidateResolution(nWidth, nHeight);

		if (!*szOutFilePath)
		{
			sprintf(szOutFilePath, encodeCLIOptions.IsCodecH264() ? "out.h264" : "out.hevc");
		}

		ck(cuInit(0));
		int nGpu = 0;
		ck(cuDeviceGetCount(&nGpu));
		if (iGpu < 0 || iGpu >= nGpu)
		{
			std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
			return 1;
		}
		CUdevice cuDevice = 0;
		ck(cuDeviceGet(&cuDevice, iGpu));
		char szDeviceName[80];
		ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
		std::cout << "GPU in use: " << szDeviceName << std::endl;
		CUcontext cuContext = NULL;
		ck(cuCtxCreate(&cuContext, 0, cuDevice));

		// Open input file
		std::ifstream fpIn(szInFilePath, std::ifstream::in | std::ifstream::binary);
		if (!fpIn)
		{
			std::ostringstream err;
			err << "Unable to open input file: " << szInFilePath << std::endl;
			throw std::invalid_argument(err.str());
		}

		// Open output file
		std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
		if (!fpOut)
		{
			std::ostringstream err;
			err << "Unable to open output file: " << szOutFilePath << std::endl;
			throw std::invalid_argument(err.str());
		}

		// Encode
		if (bOutputInVideoMem)
		{
			EncodeCudaOpInVidMem(nWidth, nHeight, eFormat, encodeCLIOptions, cuContext, fpIn, fpOut, szOutFilePath, cuStreamType);
		}
		else
		{
			EncodeCuda(nWidth, nHeight, eFormat, encodeCLIOptions, cuContext, fpIn, fpOut);
		}

		fpOut.close();
		fpIn.close();

		std::cout << "Bitstream saved in file " << szOutFilePath << std::endl;
	}
	catch (const std::exception &ex)
	{
		std::cout << ex.what();
		return 1;
	}
	return 0;
}
