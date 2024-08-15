#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <cuda.h>
#include <atomic>
#include <memory>
#include "NvEncoder/NvEncoderCuda.h"
#include "../Utils/NvEncoderCLIOptions.h"
#include "../Utils/NvCodecUtils.h"
#include <condition_variable>
#include <mutex>

#define bufSize 2

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class NvCUStream
{
public:
	NvCUStream(CUcontext cuDevice, int cuStreamType, std::unique_ptr<NvEncoderCuda> &pEnc)
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

struct EncodedFrameData {
	uint8_t* data;
	uint32_t size;
	uint32_t offset;
};

struct safeBuffer {
	std::atomic<bool> readyToEdit; // true = can EDIT content / false = can READ content
	std::condition_variable condVarReady;
	std::mutex mutex;
	uint8_t* data;
};

struct IOEncoderMem
{
	safeBuffer hostInBuf[bufSize];
	safeBuffer hostOutBuf;
	std::vector<EncodedFrameData> hostEncodedData;

	~IOEncoderMem() {
		for (int inBuf = 0; inBuf < bufSize; inBuf++){
			if (hostInBuf[inBuf].data) {
				cuMemFreeHost(hostInBuf[inBuf].data);
				hostInBuf[inBuf].data = nullptr;
			}
		}
		if (hostOutBuf.data) {
			cuMemFreeHost(hostOutBuf.data);
			hostOutBuf.data = nullptr;
		}
	}
};

struct ThreadData
{
	std::unique_ptr<NvEncoderCuda> encSession;
	std::unique_ptr<NvCUStream> cuStream;
	CUcontext *cuContext;

	~ThreadData() {
		cuStream.release();
		encSession.release();
	};
};

struct fileReadData
{
	ThreadData* threadData;
	IOEncoderMem* ioVideoMem;
	uint32_t vidPortionNum;
	uint32_t vidThreadIdx;
	uint32_t numFrames;
	bool isLast;
	bool isSingleThread;
	uint64_t videoSize;
	uint64_t offset;
	char* filePath;
};

struct encodeData
{
	ThreadData* threadData;
	IOEncoderMem* ioVideoMem;
	uint32_t vidPortionNum;
	uint32_t vidThreadIdx;
	uint32_t numFrames;
	bool isLast;
	bool isSingleThread;
	uint64_t videoSize;
	uint64_t offset;
	char* filePath;
};

struct fileWriteData
{
	std::ofstream* fpOut;
	IOEncoderMem* ioVideoMem;
	uint32_t vidPortionNum;
	uint32_t vidThreadIdx;
	char *outPath;
    bool isFirst;
	bool isLast;
    bool isAV1;
};