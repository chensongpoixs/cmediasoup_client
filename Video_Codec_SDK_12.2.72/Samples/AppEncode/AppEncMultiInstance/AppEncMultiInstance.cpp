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

/*
* This sample application was created to accelerate file compression storage applications.
* It does this by splitting the input video into N separate and independent video portions,
* i.e., independent GOPs (Split GOP). After being encoded independently, the compressed video 
* portions are then written to file preserving the original order generating a single output 
* bitstream.
* More than one encoding session thread can be used to encode the several independent video
* portions. Using more than 1 encoding session threads should allow for speedups when using
* NVIDIA GPUs with more than 1 NVENC.
* The number of portions the input video should be partitioned in is controlled by the CLI
* option "-nf" and the number of encoding session threads "-thread". Note that on systems
* with GeForce GPUs, the number of simultaneous encode sessions allowed on the system is
* restricted to 5 sessions.
* There are separate threads for: 1. reading the RAW input frames from disk; 2. copying the RAW
* frames from RAM to VRAM, encoding and copying the compressed data from VRAM to RAM; 3. writing
* the compressed data to the output file. Additionally, the main thread is only used for
* initialization and to create work queues for the described threads.
*/

#include "AppEncMultiInstance.h"

simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

inline void gatherEncodedData(std::vector<uint8_t>& encOutBuf, uint8_t* hostOutVidBuf, uint64_t &totalBitStreamSize, std::vector<EncodedFrameData>& hostEncodedData)
{
	EncodedFrameData frameData;
	frameData.offset = 0;
	frameData.data = hostOutVidBuf + totalBitStreamSize;
	frameData.size = static_cast<uint32_t>(encOutBuf.size()); // get size of the bitstream chunk
	std::memcpy(frameData.data, reinterpret_cast<char*>(encOutBuf.data()), encOutBuf.size());
	totalBitStreamSize += frameData.size + frameData.offset; // increment copied size
	hostEncodedData.push_back(std::ref(frameData));
}

void asyncFread(ConcurrentQueue<fileReadData>& freadeQueue, std::atomic<bool>& freadWorking)
{
	fileReadData input;
	while (freadWorking) {
		if (freadeQueue.size()) {
			input = freadeQueue.pop_front(); // always pop front to preserve order of video portions
			safeBuffer* inSafeBuf = input.ioVideoMem->hostInBuf;
			std::ifstream fpIn(input.filePath, std::ifstream::in | std::ifstream::binary); // open input file
			fpIn.seekg(input.offset, fpIn.beg); // get desired video portion
			if (!fpIn) {
				LOG(ERROR) << "Unable to open input file: " << input.filePath;
				break;
			}
			uint64_t nFrameSize = input.threadData->encSession->GetFrameSize();
			ck(cuCtxSetCurrent((CUcontext)input.threadData->encSession->GetDevice()));
			for (uint32_t i = 0; i < input.numFrames; i++)
			{
				int bufIdx = i % bufSize;
				std::unique_lock<std::mutex> inLock{ inSafeBuf[bufIdx].mutex };
				while (!inSafeBuf[bufIdx].readyToEdit) {
					inSafeBuf[bufIdx].condVarReady.wait(inLock); // wait until INPUT buffer is ready to be EDITED
				}
				std::streamsize nRead = fpIn.read(reinterpret_cast<char*>(inSafeBuf[bufIdx].data), nFrameSize).gcount(); // read one frame from desired video portion
				inSafeBuf[bufIdx].readyToEdit = false; // ENCODE step can start
				inSafeBuf[bufIdx].condVarReady.notify_all();
			}
			fpIn.close(); // close file
			if (input.isLast) { // if last end thread
				freadWorking = false;
				break;
			}
		}
	}
}

void asyncEncode(ConcurrentQueue<encodeData>& encodeQueue, std::atomic<bool>& encoderWorking)
{
	encodeData enc;
	while (encoderWorking) {
		if (encodeQueue.size()) {
			enc = encodeQueue.pop_front(); // always pop front to preserve order of video portions
			safeBuffer* inSafeBuf = enc.ioVideoMem->hostInBuf;
			safeBuffer* outSafeBuf = &enc.ioVideoMem->hostOutBuf;
			std::unique_lock<std::mutex> outLock{ outSafeBuf->mutex };
			while (!outSafeBuf->readyToEdit) {
				outSafeBuf->condVarReady.wait(outLock); // wait until OUTPUT buffer is ready to be EDITED
			}
			std::ifstream fpIn(enc.filePath, std::ifstream::in | std::ifstream::binary); // open input file
			fpIn.seekg(enc.offset, fpIn.beg); // get desired video portion
			if (!fpIn) {
				LOG(ERROR) << "Unable to open input file: " << enc.filePath;
				break;
			}
			enc.ioVideoMem->hostEncodedData.clear(); // clear last ouput data
			uint64_t nFrameSize = enc.threadData->encSession->GetFrameSize();
			uint64_t totalBitStreamSize = 0; // need to keep track of the size of each compressed frame
			ck(cuCtxSetCurrent((CUcontext)enc.threadData->encSession->GetDevice()));
			std::vector<std::vector<uint8_t>> encOutBuf;
			NV_ENC_PIC_PARAMS nvEncPicParams = { NV_ENC_PIC_PARAMS_VER };
			if (!enc.isSingleThread)
				nvEncPicParams.encodePicFlags = NV_ENC_PIC_FLAG_FORCEIDR; // force IDR frame for the first frame of each video portion

			std::vector <std::unique_lock<std::mutex>> inBufLocks;
			for (uint32_t i = 0; i < bufSize; i++)
				inBufLocks.push_back(std::unique_lock<std::mutex>(inSafeBuf[i].mutex, std::defer_lock));

			for (uint32_t i = 0; i < enc.numFrames; i++)
			{
				int bufIdx = i % bufSize;
				int lastBufIdx = bufIdx == 0 ? (bufSize-1) : (bufIdx-1);
				if(i > 0){
					ck(cuStreamSynchronize(enc.threadData->cuStream->GetInputCUStream())); // make sure the last memcpy is complete
					inSafeBuf[lastBufIdx].readyToEdit = true; // ENCODE step can start
					inSafeBuf[lastBufIdx].condVarReady.notify_all();
					inBufLocks[lastBufIdx].unlock();
				}
				inBufLocks[bufIdx].lock();
				while (inSafeBuf[bufIdx].readyToEdit) {
					inSafeBuf[bufIdx].condVarReady.wait(inBufLocks[bufIdx]); // wait until INPUT buffer is ready to be EDITED
				}
				const NvEncInputFrame* encoderInputFrame = enc.threadData->encSession->GetNextInputFrame();
				NvEncoderCuda::CopyToDeviceFrame((CUcontext)enc.threadData->encSession->GetDevice(),
					(uint8_t*)inSafeBuf[bufIdx].data,
					0,
					(CUdeviceptr)encoderInputFrame->inputPtr,
					encoderInputFrame->pitch,
					enc.threadData->encSession->GetEncodeWidth(),
					enc.threadData->encSession->GetEncodeHeight(),
					CU_MEMORYTYPE_HOST,
					encoderInputFrame->bufferFormat,
					encoderInputFrame->chromaOffsets,
					encoderInputFrame->numChromaPlanes,
					false,
					enc.threadData->cuStream->GetInputCUStream()); // do async frame copy from host to device
				enc.threadData->encSession->EncodeFrame(encOutBuf, i || enc.isSingleThread ? NULL : &nvEncPicParams); // if first frame than use IDR frame
				for (uint32_t j = 0; j < encOutBuf.size(); ++j) { // gather encoded data in output buffer to write to file later
					gatherEncodedData(encOutBuf[j], outSafeBuf->data, totalBitStreamSize, enc.ioVideoMem->hostEncodedData);
				}
			}
			if (!enc.isSingleThread || enc.isLast) {
				enc.threadData->encSession->EndEncode(encOutBuf); // get last compressed frames
				for (uint32_t j = 0; j < encOutBuf.size(); ++j) { // gather encoded data in output buffer to write to file later
					gatherEncodedData(encOutBuf[j], outSafeBuf->data, totalBitStreamSize, enc.ioVideoMem->hostEncodedData);
				}
			}
			outSafeBuf->readyToEdit = false; // OUTPUT buffer is ready to be READ
			outSafeBuf->condVarReady.notify_all();
			int lastBufIdx = (enc.numFrames-1) % bufSize;
			inSafeBuf[lastBufIdx].readyToEdit = true;
			inSafeBuf[lastBufIdx].condVarReady.notify_all();
			if (enc.isLast) { // if last end thread
				encoderWorking = false;
				break;
			}
		}
	}
}

void asyncFwrite(ConcurrentQueue<fileWriteData>& fwriteQueue, std::atomic<bool>& fwriteWorking)
{
	fileWriteData output;
    uint8_t skipIVF = 32;
	while (fwriteWorking) {
		if (fwriteQueue.size()) {
			output = fwriteQueue.pop_front(); // always pop front to preserve order of video portions
			safeBuffer* sB = &output.ioVideoMem->hostOutBuf;
			std::unique_lock<std::mutex> lock{ sB->mutex };
			while (sB->readyToEdit) {
				sB->condVarReady.wait(lock); // wait until OUTPUT buffer is ready to be READ
			}
            bool firstPackage = true;
            for(auto compressedData : output.ioVideoMem->hostEncodedData) {
                if(output.isAV1 && output.vidThreadIdx > 0 && output.isFirst && firstPackage){ // for AV1 we only need an IVF header on the first video portion
                    output.fpOut->write((char*)(compressedData.data + skipIVF), compressedData.size - skipIVF); // write all the compressed data to file skipping IVF header
                    firstPackage = false;
                }
                else{
				    output.fpOut->write((char*)(compressedData.data), compressedData.size); // write all the compressed data to file
                }
            }
			sB->readyToEdit = true;
			sB->condVarReady.notify_all();
			if (output.isLast) { // if last end thread
				output.fpOut->close(); // close file
				std::cout << "Bitstream saved in file " << output.outPath << std::endl;
				fwriteWorking = false;
				break;
			}
		}
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
		<< "-i           Input file path" << std::endl
		<< "-o           Output file path" << std::endl
		<< "-nf          Number of frames per video portions to extract from input file (default is 60)" << std::endl
		<< "-s           Input resolution in this form: WxH" << std::endl
		<< "-if          Input format: iyuv nv12 yuv444 p010 yuv444p16 bgra" << std::endl
		<< "-gpu         Ordinal of GPU to use" << std::endl
		<< "-thread      Number of encoding thread (default is 2)" << std::endl
		<< "-splitframe  Split Frame configuration (default is 0): 0 - no Split Frame, 1 - auto mode, 2 - 2-way Split Frame, 3 - 3-way Split Frame" << std::endl
		;
	oss << NvEncoderInitParam().GetHelpMessage();
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

void ParseCommandLine(int argc, char *argv[], uint64_t &nNumVideoPortions, char *szInputFileName, char *szOutputFileName, uint32_t &nWidth, uint32_t &nHeight,
	NV_ENC_BUFFER_FORMAT &eFormat, int &iGpu, int &nThread, int &nWaySplitFrame, NvEncoderInitParam &initParam)
{
	if (argc < 2)
	{
		ShowHelpAndExit();
	}
	else
	{
		std::ostringstream oss;
		for (int i = 1; i < argc; i++)
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
			if (!_stricmp(argv[i], "-s"))
			{
				if (++i == argc || 2 != sscanf(argv[i], "%dx%d", &nWidth, &nHeight))
				{
					ShowHelpAndExit("-s");
				}
				continue;
			}
			if (!_stricmp(argv[i], "-nf"))
			{
				if (++i == argc)
				{
					ShowHelpAndExit("-nf");
				}
				nNumVideoPortions = atoi(argv[i]);
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
			std::vector<std::string> vszFileFormatName =
			{
				"iyuv", "nv12", "yv12", "yuv444", "p010", "yuv444p16", "bgra", "argb10", "ayuv", "abgr", "abgr10"
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
				if (++i == argc)
				{
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
			if (!_stricmp(argv[i], "-thread"))
			{
				if (++i == argc)
				{
					ShowHelpAndExit("-thread");
				}
				nThread = atoi(argv[i]);
				continue;
			}
			if (!_stricmp(argv[i], "-splitframe"))
			{
				if (++i == argc)
				{
					ShowHelpAndExit("-splitframe");
				}
				nWaySplitFrame = atoi(argv[i]);
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
}

uint64_t getFileSize(const char *szFileName)
{
	struct _stat64 st;
	if (_stat64(szFileName, &st) != 0)
	{
		return 0;
	}
	return st.st_size;
}

uint64_t getNumberOfFrames(const char *szFileName, uint32_t width, uint32_t height, uint64_t frameSize)
{
	struct _stat64 st;
	if (_stat64(szFileName, &st) != 0)
	{
		return 0;
	}
	return (uint64_t)(st.st_size / frameSize);
}

NV_ENC_SPLIT_ENCODE_MODE getSplitFrameFlag(int nWaySplitFrame)
{
	switch (nWaySplitFrame)
	{
	case 0:
		return NV_ENC_SPLIT_DISABLE_MODE;
		break;
	case 1:
		return NV_ENC_SPLIT_AUTO_MODE;
		break;
	case 2:
		return NV_ENC_SPLIT_TWO_FORCED_MODE;
		break;
	default:
		return NV_ENC_SPLIT_THREE_FORCED_MODE;
		break;
	}
}

int main(int argc, char **argv)
{
	char szInFilePath[256] = "",
		szOutFilePath[256] = "";
	uint32_t nWidth = 0, nHeight = 0;
	NV_ENC_BUFFER_FORMAT eFormat = NV_ENC_BUFFER_FORMAT_IYUV;
	int iGpu = 0;
	int nThread = 2;
	int nWaySplitFrame = 0;
	uint64_t numFramesPerVideoPortion = 120;
	try
	{
		StopWatch globalTime;
		globalTime.Start();

		NvEncoderInitParam encodeCLIOptions;
		ParseCommandLine(argc, argv, numFramesPerVideoPortion, szInFilePath, szOutFilePath, nWidth, nHeight, eFormat,
			iGpu, nThread, nWaySplitFrame, encodeCLIOptions);
		if (numFramesPerVideoPortion == 0) { // number of video frames per video portion cannot be 0
			std::cout << "numFramesPerVideoPortion (-nf) should be greater than 0!" << std::endl;
			return 1;
		}
		CheckInputFile(szInFilePath);
		ValidateResolution(nWidth, nHeight);

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

		std::ofstream fpOut(szOutFilePath, std::ios::out | std::ios::binary);
		if (!fpOut) {
			std::ostringstream err;
			err << "Unable to open output file: " << szOutFilePath << std::endl;
			throw std::invalid_argument(err.str());
		}

		NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
		NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
		initializeParams.encodeConfig = &encodeConfig;

		CUcontext cuContext;
		ck(cuCtxCreate(&(cuContext), CU_CTX_SCHED_BLOCKING_SYNC, cuDevice)); // Create single CUDA context
		// Create and initialize array of data required for each encoding session thread 
		std::vector<ThreadData> vidEncThreads(nThread);
		for (int i = 0; i < nThread; i++) {
			vidEncThreads[i].cuContext = &cuContext; // same CUDA context for every encoding session thread
			vidEncThreads[i].encSession = make_unique<NvEncoderCuda>(cuContext, nWidth, nHeight, eFormat);
			vidEncThreads[i].encSession->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());
			encodeCLIOptions.SetInitParams(&initializeParams, eFormat);
			initializeParams.splitEncodeMode = getSplitFrameFlag(nWaySplitFrame);
			vidEncThreads[i].encSession->CreateEncoder(&initializeParams);
			vidEncThreads[i].cuStream.reset(new NvCUStream(cuContext, 1, vidEncThreads[i].encSession)); // each encoding session thread is going to use one cuda stream
		}

		uint64_t frameSize = vidEncThreads[0].encSession->GetFrameSize(); // calculate frame size
		uint64_t numFramesTotal = getNumberOfFrames(szInFilePath, nWidth, nHeight, frameSize); // calculate total number of frames
		uint64_t nNumVideoPortions = 0;
		if (numFramesPerVideoPortion > numFramesTotal) { // the number of frames per video portion should not be larger than the total number of frames
			numFramesPerVideoPortion = numFramesTotal;
			std::cout << "Warning: Number of frames per video portions should be smaller or equal to total number of frames! Adjusting numFramesPerVideoPortion = " << numFramesPerVideoPortion << std::endl;
		}
		// calculations required for cases where the number of frames per video portions is not a multiple of the total number of frames
		if (nThread == 1) {
			std::cout << "SINGLE ENCODE SESSSION MODE - The video encoding pipeline is processed with no GOP splits, i.e., the input video is not split into video portions." << std::endl;
			numFramesPerVideoPortion = 16;
		}
		nNumVideoPortions = (numFramesTotal / numFramesPerVideoPortion) + ((numFramesTotal % numFramesPerVideoPortion) != 0);
		uint64_t sizePerVideoPortion = numFramesPerVideoPortion * frameSize;
		uint64_t numFramesLastVideoPortion = (numFramesTotal % numFramesPerVideoPortion);
		if (!numFramesLastVideoPortion) // if this is 0 it means the last video portion has the same number of frames as the other video portions
			numFramesLastVideoPortion = numFramesPerVideoPortion;
		uint64_t totalMemoryAllocation = (sizePerVideoPortion + 1) * nThread;
		// calculate and report total memory allocation required for the current settings
		std::cout << "Number of video portions: " << nNumVideoPortions << std::endl;
		std::cout << "Number of frames per video portions: " << numFramesPerVideoPortion << std::endl;
		std::cout << "Size of each video portion: " << sizePerVideoPortion / 1000000 << " MB." << std::endl;
		std::cout << "Number of video encoding threads: " << nThread << std::endl;
		std::cout << "Allocating " << totalMemoryAllocation / 1000000 << " MB of memory." << std::endl;
		// Allocate all the required memory for IO 
		std::vector<IOEncoderMem> ioVideoMem(nThread);
		for (int i = 0; i < nThread; i++) {
			for (int inBuf = 0; inBuf < bufSize; inBuf++){
				ioVideoMem[i].hostInBuf[inBuf].readyToEdit = true;
				ck(cuMemAllocHost((void**)&ioVideoMem[i].hostInBuf[inBuf].data, frameSize)); // Allocate pinned memory for input RAW frame
			}
			ioVideoMem[i].hostOutBuf.readyToEdit = true;
			ck(cuMemAllocHost((void**)&ioVideoMem[i].hostOutBuf.data, sizePerVideoPortion)); // Allocate pinned memory for output compressed video portions
		}
		// Create fwrite and encode work queues
		ConcurrentQueue<fileWriteData> fwriteQueue;
		std::vector<ConcurrentQueue<encodeData>> encodeQueue(nThread);
		std::vector<ConcurrentQueue<fileReadData>> freadQueue(nThread);

		uint64_t nFrame = 0; // frame counter per video portion
		uint32_t videoPortion = 0; // video portion counter
		uint64_t nTotal = 0; // total frame counter
		float totalProcessingTime = 0;
		while (videoPortion < nNumVideoPortions) // go through every video portion
		{
			nFrame = 0; // reset number of frames per video portion
			for (int i = 0; i < nThread && videoPortion + i < nNumVideoPortions; i++) // split video portions across the several available encoding session threads
			{
				// FREAD thread work queue generation
				fileReadData currFreadData;
				currFreadData.offset = (videoPortion + i) * sizePerVideoPortion;
				currFreadData.filePath = szInFilePath;
				currFreadData.numFrames = static_cast<uint32_t>(((videoPortion + i + 1) == nNumVideoPortions) ? numFramesLastVideoPortion : numFramesPerVideoPortion);
				currFreadData.threadData = &vidEncThreads[i];
				currFreadData.vidPortionNum = videoPortion + i;
				currFreadData.vidThreadIdx = i;
				currFreadData.ioVideoMem = &ioVideoMem[i];
				currFreadData.isLast = (videoPortion + i + 1 == nNumVideoPortions); // check if last to end thread
				currFreadData.isSingleThread = (nThread == 1);
				freadQueue[i].push_back(currFreadData); // queue fread work
				// video ENCODING thread work queue generation
				encodeData currEncData;
				currEncData.offset = (videoPortion + i) * sizePerVideoPortion;
				currEncData.filePath = szInFilePath;
				currEncData.numFrames = static_cast<uint32_t>(((videoPortion + i + 1) == nNumVideoPortions) ? numFramesLastVideoPortion : numFramesPerVideoPortion);
				currEncData.threadData = &vidEncThreads[i];
				currEncData.vidPortionNum = videoPortion + i;
				currEncData.vidThreadIdx = i;
				currEncData.ioVideoMem = &ioVideoMem[i];
				currEncData.isLast = (videoPortion + i + 1 == nNumVideoPortions); // check if last to end thread
				currEncData.isSingleThread = (nThread == 1);
				encodeQueue[i].push_back(currEncData); // queue encode work
				// FWRITE thread work queue generation            
				fileWriteData currFwriteData;
				currFwriteData.vidPortionNum = videoPortion + i;
				currFwriteData.fpOut = &fpOut;
				currFwriteData.vidThreadIdx = i;
				currFwriteData.ioVideoMem = &ioVideoMem[i];
                currFwriteData.isFirst = (videoPortion+i == i); // check if first
				currFwriteData.isLast = (videoPortion + i + 1 == nNumVideoPortions); // check if last to end thread
				currFwriteData.outPath = szOutFilePath;
                currFwriteData.isAV1 = encodeCLIOptions.IsCodecAV1();
				fwriteQueue.push_back(currFwriteData); // queue fwrite work
				nFrame += currEncData.numFrames; // increment number of frames
			}
			videoPortion += nThread;
			nTotal += nFrame;
		}

		// Launch fwite and encoding threads 
		std::atomic<bool> fwriteWorking(true);
		std::thread fwriteThread = std::thread(&asyncFwrite, std::ref(fwriteQueue), std::ref(fwriteWorking));
		std::atomic<bool> encoderWorking(true);
		std::atomic<bool> freadWorking(true);
		std::vector<std::thread> encodeThread(nThread);
		std::vector<std::thread> freadThread(nThread);
		for (int i = 0; i < nThread; i++){
			encodeThread[i] = std::thread(&asyncEncode, std::ref(encodeQueue[i]), std::ref(encoderWorking));
			freadThread[i] = std::thread(&asyncFread, std::ref(freadQueue[i]), std::ref(freadWorking));
		}

		StopWatch processingTime;
		processingTime.Start();

		for (int i = 0; i < nThread; i++){
			freadThread[i].join();
			encodeThread[i].join();
		}
		fwriteThread.join();

		double gT = globalTime.Stop();
		double pT = processingTime.Stop();
		std::cout << "Total time = " << gT << " seconds, FPS=" << nTotal / gT << " (#frames=" << nTotal << ")" << std::endl;
		std::cout << "Total processing time [fread + H->D memcpy + Encode time + D->H memcpy + fwrite] = " << pT << " seconds, FPS=" << nTotal / pT << " (#frames=" << nTotal << ")" << std::endl;

		for (int i = 0; i < nThread; i++)
			vidEncThreads[i].encSession->DestroyEncoder();
		ck(cuCtxDestroy(cuContext));
	}
	catch (const std::exception &ex)
	{
		std::cout << ex.what();
		return 1;
	}

	return 0;
}
