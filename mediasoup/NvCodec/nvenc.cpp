/***********************************************************************************************
created: 		2020-2-13

author:			chensong

purpose:		nvenc
输赢不重要，答案对你们有什么意义才重要。

光阴者，百代之过客也，唯有奋力奔跑，方能生风起时，是时代造英雄，英雄存在于时代。或许世人道你轻狂，可你本就年少啊。 看护好，自己的理想和激情。


我可能会遇到很多的人，听他们讲好2多的故事，我来写成故事或编成歌，用我学来的各种乐器演奏它。
然后还可能在一个国家遇到一个心仪我的姑娘，她可能会被我帅气的外表捕获，又会被我深邃的内涵吸引，在某个下雨的夜晚，她会全身淋透然后要在我狭小的住处换身上的湿衣服。
3小时候后她告诉我她其实是这个国家的公主，她愿意向父皇求婚。我不得已告诉她我是穿越而来的男主角，我始终要回到自己的世界。
然后我的身影慢慢消失，我看到她眼里的泪水，心里却没有任何痛苦，我才知道，原来我的心被丢掉了，我游历全世界的原因，就是要找回自己的本心。
于是我开始有意寻找各种各样失去心的人，我变成一块砖头，一颗树，一滴水，一朵白云，去听大家为什么会失去自己的本心。
我发现，刚出生的宝宝，本心还在，慢慢的，他们的本心就会消失，收到了各种黑暗之光的侵蚀。
从一次争论，到嫉妒和悲愤，还有委屈和痛苦，我看到一只只无形的手，把他们的本心扯碎，蒙蔽，偷走，再也回不到主人都身边。
我叫他本心猎手。他可能是和宇宙同在的级别 但是我并不害怕，我仔细回忆自己平淡的一生 寻找本心猎手的痕迹。
沿着自己的回忆，一个个的场景忽闪而过，最后发现，我的本心，在我写代码的时候，会回来。
安静，淡然，代码就是我的一切，写代码就是我本心回归的最好方式，我还没找到本心猎手，但我相信，顺着这个线索，我一定能顺藤摸瓜，把他揪出来。
************************************************************************************************/
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "nvenc.h"
#include <cstdint>
#include <string>
#include <dxgi.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "../clog.h"
#include "../ccfg.h"
//#include "Utils/AppEncUtils.h"
//#include "Utils/AppEncUtils.h"
namespace chen {
	uint32 g_gpu_index;
	DXGI_FORMAT g_dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
	static const std::unordered_map<uint32, GUID> g_encoder_preset = {
	{1, NV_ENC_PRESET_P1_GUID},
	{2, NV_ENC_PRESET_P2_GUID},
	{3, NV_ENC_PRESET_P3_GUID},
	{4, NV_ENC_PRESET_P4_GUID},
	{5, NV_ENC_PRESET_P5_GUID},
	{6, NV_ENC_PRESET_P6_GUID},
	{7, NV_ENC_PRESET_P7_GUID}
	};

	static const std::unordered_map<uint32, NV_ENC_TUNING_INFO> g_enc_tuning_info = {
		{1, NV_ENC_TUNING_INFO_HIGH_QUALITY},
	{2, NV_ENC_TUNING_INFO_LOW_LATENCY },
	{3, NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY},
	{4, NV_ENC_TUNING_INFO_LOSSLESS}
	};


	GUID get_encoder_preset_guid(uint32 level)
	{
		std::unordered_map<uint32, GUID>::const_iterator iter = g_encoder_preset.find(level);
		if (iter != g_encoder_preset.end())
		{
			return iter->second;
		}
		return NV_ENC_PRESET_P4_GUID;
	}

	NV_ENC_TUNING_INFO get_enc_tuning(uint32 level)
	{
		std::unordered_map<uint32, NV_ENC_TUNING_INFO>::const_iterator iter = g_enc_tuning_info.find(level);
		if (iter != g_enc_tuning_info.end())
		{
			return iter->second;
		}
		return NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
	}
struct nvenc_data
{
	ID3D11Device*        d3d11_device  = nullptr;
	ID3D11DeviceContext* d3d11_context = nullptr;
	ID3D11Texture2D*     copy_texture  = nullptr;
	IDXGIAdapter*        adapter       = nullptr;
	IDXGIFactory1*       factory       = nullptr;

	HANDLE           input_handle  = nullptr;
	ID3D11Texture2D* input_texture = nullptr;;
	IDXGIKeyedMutex* keyed_mutex   = nullptr;

	std::mutex mutex;
	uint32_t width     = 0;
	uint32_t height    = 0;
	uint32_t framerate = 0;
	uint32_t bitrate   = 0;
	uint32_t gop       =  0/*NVENC_INFINITE_GOPLENGTH*/;
	std::string codec;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	NvEncoderD3D11 *nvenc = nullptr;
};

static bool is_supported(void)
{
	static HMODULE hModule = NULL;
	using namespace chen;
	if (hModule == NULL) {
#if defined(_WIN64)
		hModule = LoadLibrary(TEXT("nvEncodeAPI64.dll"));
#elif defined(_WIN32)
		hModule = LoadLibrary(TEXT("nvEncodeAPI.dll"));
#endif
	}
	
	if (hModule == NULL) 
	{
		//printf("[nvenc] Error: NVENC library file is not found. Please ensure NV driver is installed. \n");
		ERROR_EX_LOG("[nvenc] Error: NVENC library file is not found. Please ensure NV driver is installed. ");
		return false;
	}

	typedef NVENCSTATUS(NVENCAPI *NvEncodeAPIGetMaxSupportedVersion_Type)(uint32_t*);
#if defined(_WIN32)
	NvEncodeAPIGetMaxSupportedVersion_Type NvEncodeAPIGetMaxSupportedVersion = (NvEncodeAPIGetMaxSupportedVersion_Type)GetProcAddress(hModule, "NvEncodeAPIGetMaxSupportedVersion");
#else
	NvEncodeAPIGetMaxSupportedVersion_Type NvEncodeAPIGetMaxSupportedVersion = (NvEncodeAPIGetMaxSupportedVersion_Type)dlsym(hModule, "NvEncodeAPIGetMaxSupportedVersion");
#endif

	uint32_t version = 0;
	uint32_t currentVersion = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
	NVENC_API_CALL(NvEncodeAPIGetMaxSupportedVersion(&version));
	if (currentVersion > version) 
	{
		//printf("[nvenc] Error: Current Driver Version does not support this NvEncodeAPI version, please upgrade driver");
		ERROR_EX_LOG("[nvenc] Error: Current Driver Version does not support this NvEncodeAPI version, please upgrade driver");
		return false;
	}

	return true;
}

static void* nvenc_create()
{
	using namespace chen;
	//ERROR_EX_LOG("");
	if (!is_supported()) {
		return nullptr;
	}
	int32 index = 0;
	struct nvenc_data* enc = new nvenc_data;
	HRESULT hr = S_OK;

	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&enc->factory);
	if (FAILED(hr)) {
		return nullptr;
	}
	
	SYSTEM_LOG(" start  gpu info [g_gpu_index = %u] ....", g_gpu_index);


	hr = enc->factory->EnumAdapters(g_gpu_index, &enc->adapter);
	if (FAILED(hr)) 
	{
		WARNING_EX_LOG("[g_gpu_index = %u] enumadapters failed !!! ", g_gpu_index);
		goto gpuadapter;
	}
	else
	{
		char desc[128] = { 0 };
		DXGI_ADAPTER_DESC adapterDesc;
		enc->adapter->GetDesc(&adapterDesc);
		wcstombs(desc, adapterDesc.Description, sizeof(desc));
		if (strstr(desc, "NVIDIA") != NULL) 
		{
			hr = D3D11CreateDevice(enc->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
				&enc->d3d11_device, nullptr, &enc->d3d11_context);
			enc->adapter->Release();
			enc->adapter = nullptr;
			if (SUCCEEDED(hr)) 
			{
				index = g_gpu_index;
				SYSTEM_LOG("use gpu info g_gpu_index = %u ok ", g_gpu_index);
				goto findok;
			}
		}
		else
		{
			WARNING_EX_LOG("[g_gpu_index = %u]  not NVIDIA  [desc = %s] failed !!! ", g_gpu_index, desc);
		}
	}
gpuadapter:
	NORMAL_EX_LOG("gpuadapter");
	for (int gpuIndex = 0; gpuIndex <= 5; gpuIndex++) 
	{
		NORMAL_EX_LOG("gpuadapter [gpuIndex = %u]", gpuIndex);
		hr = enc->factory->EnumAdapters(gpuIndex, &enc->adapter);
		if (FAILED(hr)) {
			goto failed;
		}
		else {
			char desc[128] = { 0 };
			DXGI_ADAPTER_DESC adapterDesc;
			enc->adapter->GetDesc(&adapterDesc);
			wcstombs(desc, adapterDesc.Description, sizeof(desc));
			if (strstr(desc, "NVIDIA") == NULL) 
			{
				continue;
			}
		}
		index = gpuIndex;
		hr = D3D11CreateDevice(enc->adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
							   &enc->d3d11_device, nullptr, &enc->d3d11_context);
		enc->adapter->Release();
		enc->adapter = nullptr;
		if (SUCCEEDED(hr)) {
			break;
		}
	}
findok:
	NORMAL_EX_LOG("used gpu index = %u", index);
	if (enc->factory) {
		enc->factory->Release();
		enc->factory = nullptr;
	}

	if (enc->d3d11_device == nullptr) {
		printf("[nvenc] Error: Failed to create d3d11 device. \n");
		ERROR_EX_LOG("[nvenc] Error: Failed to create d3d11 device.");
		goto failed;
	}

	return enc;

failed:
	if (enc->factory) {
		enc->factory->Release();
		enc->factory = nullptr;
	}

	if (enc != nullptr)  {
		delete enc;
	}
	return nullptr;
}

static void nvenc_destroy(void **nvenc_data)
{
	using namespace chen;
	//ERROR_EX_LOG("");
	struct nvenc_data *enc = (struct nvenc_data *)(*nvenc_data);

	enc->mutex.lock();

	if (enc->nvenc != nullptr) {
		enc->nvenc->DestroyEncoder();
		delete enc->nvenc;
		enc->nvenc = nullptr;
	}

	if (enc->d3d11_device) {
		enc->d3d11_device->Release();
		enc->d3d11_device = nullptr;
	}

	if (enc->d3d11_context) {
		enc->d3d11_context->Release();
		enc->d3d11_context = nullptr;
	}

	if (enc->adapter) {
		enc->adapter->Release();
		enc->adapter = nullptr;
	}

	if (enc->factory) {
		enc->factory->Release();
		enc->factory = nullptr;
	}

	if (enc->copy_texture) {
		enc->copy_texture->Release();
		enc->copy_texture = nullptr;
	}

	if (enc->input_texture) {
		enc->input_texture->Release();
		enc->input_texture = nullptr;
	}

	if (enc->keyed_mutex) {
		enc->keyed_mutex->Release();
		enc->keyed_mutex = nullptr;
	}

	enc->mutex.unlock();

	delete *nvenc_data;
	*nvenc_data = nullptr;
}

static bool nvenc_init(void *nvenc_data, void *encoder_config)
{
	using namespace chen;
	//ERROR_EX_LOG("");
	if (nvenc_data == nullptr) {
		return false;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;
	struct encoder_config* config = (struct encoder_config*)encoder_config;

	std::lock_guard<std::mutex> locker(enc->mutex);
	if (enc->nvenc != nullptr) {
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
	desc.Width = config->width;
	desc.Height = config->height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = config->format;
	NORMAL_EX_LOG("[format = %u]", config->format);
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = enc->d3d11_device->CreateTexture2D(&desc, nullptr, &enc->copy_texture);
	if (FAILED(hr)) {
		//printf("[nvenc] Error: Failed to create texture. \n");
		ERROR_EX_LOG("[nvenc] Error: Failed to create texture.");
		return false;
	}

	enc->width = config->width;
	enc->height = config->height;
	enc->framerate = config->framerate;
	enc->format = config->format;
	enc->codec = config->codec;
	enc->gop = NVENC_INFINITE_GOPLENGTH; // config->gop;
	enc->bitrate = config->bitrate;

	NV_ENC_BUFFER_FORMAT eBufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
	if (enc->format == DXGI_FORMAT_NV12) {
		eBufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
	}
	else if (enc->format == DXGI_FORMAT_B8G8R8A8_UNORM) {
		eBufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;
	}
	else if (enc->format == DXGI_FORMAT_R8G8B8A8_UNORM)
	{
		eBufferFormat = NV_ENC_BUFFER_FORMAT_ARGB; // error 
	}
	else if (enc->format == DXGI_FORMAT_R10G10B10A2_UNORM)
	{
		eBufferFormat = NV_ENC_BUFFER_FORMAT_ABGR10;
	}
	else {
		//printf("[nvenc] Error: Unsupported dxgi format. \n");
		ERROR_EX_LOG("[nvenc] Error: Unsupported dxgi format = %u. ", enc->format);
		return false;
	}

	GUID codecId = NV_ENC_CODEC_H264_GUID;
	if (enc->codec == "h264") {
		codecId = NV_ENC_CODEC_H264_GUID;
	}
	else if (enc->codec == "hevc") {
		codecId = NV_ENC_CODEC_HEVC_GUID;
	}
	else {
		//printf("[nvenc] Error: Unsupported codec. \n");
		ERROR_EX_LOG("[nvenc] Error: Unsupported codec.");
		return false;
	}

	enc->nvenc = new NvEncoderD3D11(enc->d3d11_device, enc->width, enc->height, eBufferFormat);

	NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
	initializeParams.encodeConfig = &encodeConfig;	
	enc->nvenc->CreateDefaultEncoderParams(&initializeParams, codecId, get_encoder_preset_guid(g_cfg.get_int32(ECI_EncoderPreset)),   get_enc_tuning(g_cfg.get_uint32(ECI_EncoderLowLatency)));
	// chensong@20220506 -- .\AppEncD3D11.exe -i C:\Work\video\test3.rgb -s 1920x1080 -gpu 1  -codec h264  -preset p4 -fps 60 -bf 10    -o p4.h264
	
	initializeParams.encodeWidth = enc->width;
	initializeParams.encodeHeight = enc->height;
	initializeParams.version = NV_ENC_INITIALIZE_PARAMS_VER;
	initializeParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
	initializeParams.presetGUID = NV_ENC_PRESET_P4_GUID;
	 
	initializeParams.frameRateDen = 1;
	initializeParams.enablePTD = 1;
	initializeParams.reportSliceOffsets = 0;
	initializeParams.enableSubFrameWrite = 0;
	
	initializeParams.tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
	initializeParams.frameRateNum = g_cfg.get_int32(ECI_RtcFrames);


	///////////////
	// H.264 specific settings
	///
	
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableIntraRefresh = 1;
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshPeriod = 180;
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.intraRefreshCnt = 180;
	//initializeParams.encodeConfig->encodeCodecConfig.h264Config.idrPeriod = 
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.repeatSPSPPS = 1;
	/*
	* Slice mode - set the slice mode to "entire frame as a single slice" because WebRTC implementation doesn't work well with slicing. The default slicing mode
	* produces (rarely, but especially under packet loss) grey full screen or just top half of it.
	*/
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceMode = 0;
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.sliceModeData = 0;
	// `outputPictureTimingSEI` is used in CBR mode to fill video frame with data to match the requested bitrate.
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.outputPictureTimingSEI = 1;
	initializeParams.encodeConfig->encodeCodecConfig.h264Config.enableFillerDataInsertion = 1;
	//initializeParams.encodeConfig->gopLength =  g_cfg.get_uint32(ECI_EncoderVideoGop);//NVENC_INFINITE_GOPLENGTH;//
	//initializeParams.encodeConfig->rcParams.averageBitRate = g_cfg.get_uint32(ECI_RtcAvgRate) * 1000 ;
	//initializeParams.encodeConfig->rcParams.maxBitRate = g_cfg.get_uint32(ECI_RtcMaxRate) * 1000;
	//initializeParams.encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;// NV_ENC_PARAMS_RC_VBR_HQ;// NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ;
	//initializeParams.encodeConfig->rcParams.qpMapMode = NV_ENC_QP_MAP_DELTA;
	NV_ENC_RC_PARAMS& RateControlParams = initializeParams.encodeConfig->rcParams;
#define DEFAULT_BITRATE (1000000u)
	uint32_t const MinQP = static_cast<uint32_t>(1);
	uint32_t const MaxQP = static_cast<uint32_t>(51);
	RateControlParams.rateControlMode = g_cfg.get_uint32(ECI_EnableEncoderCbr) > 0 ?  NV_ENC_PARAMS_RC_CBR: NV_ENC_PARAMS_RC_VBR; // NV_ENC_PARAMS_RC_VBR; // NV_ENC_PARAMS_RC_CBR_LOWDELAY_HQ
	RateControlParams.averageBitRate = g_cfg.get_uint32(ECI_RtcAvgRate) * 1000;// DEFAULT_BITRATE;
	RateControlParams.maxBitRate = g_cfg.get_uint32(ECI_RtcMaxRate) * 1000;// DEFAULT_BITRATE; // Not used for CBR
	RateControlParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
	if (g_cfg.get_uint32(ECI_EnableEncoderCbr) <= 0)
	{
		RateControlParams.minQP = { MinQP, MinQP, MinQP };
		RateControlParams.maxQP = { MaxQP, MaxQP, MaxQP };
		RateControlParams.enableMinQP = 1;
		RateControlParams.enableMaxQP = 1;
	}

	// If we have QP ranges turned on use the last encoded QP to guide the max QP for an i-frame, so the i-frame doesn't look too blocky
	// Note: this does nothing if we have i-frames turned off.
	/*if (RateControlParams.enableMaxQP && LastEncodedQP > 0 && CVarKeyframeQPUseLastQP.GetValueOnAnyThread())
	{
		RateControlParams.maxQP.qpIntra = LastEncodedQP;
	}*/
	
	enc->nvenc->CreateEncoder(&initializeParams);
	NORMAL_EX_LOG("------------->encoder create ok !!!");
	return true;
}

int nvenc_encode_texture(void *nvenc_data, ID3D11Texture2D *texture,int * ready, uint8_t* out_buf, uint32_t out_buf_size)
{
	using namespace chen;
	//NORMAL_EX_LOG("");
	uint32_t packet_size = 0;
	if (nvenc_data == nullptr) 
	{
		ERROR_EX_LOG("nvenc_data == nullptr");
		return -1;
	}
	//NORMAL_EX_LOG("");
	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;	

	std::lock_guard<std::mutex> locker(enc->mutex);
	//NORMAL_EX_LOG("");
	if (enc->nvenc == nullptr)
	{
		ERROR_EX_LOG("evenc ");
		return -1;
	}
	//NORMAL_EX_LOG("");
	std::vector<std::vector<uint8_t>> packet;
	const NvEncInputFrame* input_frame = enc->nvenc->GetNextInputFrame();
	//NORMAL_EX_LOG("");
	ID3D11Texture2D *encoder_texture = reinterpret_cast<ID3D11Texture2D*>(input_frame->inputPtr);
	//*ready = 1;
	//NORMAL_EX_LOG("");
	enc->d3d11_context->CopyResource(encoder_texture, texture);
	//*ready = 0;
	//NORMAL_EX_LOG("");
	
	try
	{
		enc->nvenc->EncodeFrame(out_buf, &packet_size);
	}
	catch (...)
	{
		ERROR_EX_LOG("nvenc encode frame failed !!! packet ");
		return -2;
	}
	

	/*int frame_size = 0;
	for (std::vector<uint8_t> &packet : packet) {
		if (frame_size + packet.size() < out_buf_size) {
			memcpy(out_buf + frame_size, packet.data(), packet.size());
			frame_size += (int)packet.size();
		}
		else {
			break;
		}
	}*/

	return packet_size;
}



int nvenc_encode_texture_unlock_lock(void *nvenc_data, ID3D11Texture2D *texture , uint8_t* out_buf, uint32_t out_buf_size, int lock_key, int unlock_key, IDXGIKeyedMutex* keyed_mutex)
{
	using namespace chen;
	uint32_t packet_size = 0;
	//NORMAL_EX_LOG("");
	if (nvenc_data == nullptr)
	{
		ERROR_EX_LOG("nvenc_data == nullptr");
		return -1;
	}
	//NORMAL_EX_LOG("");
	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);
	//NORMAL_EX_LOG("");
	if (enc->nvenc == nullptr)
	{
		ERROR_EX_LOG("evenc ");
		return -1;
	}
	//NORMAL_EX_LOG("");
	std::vector<std::vector<uint8_t>> packet;
	const NvEncInputFrame* input_frame = enc->nvenc->GetNextInputFrame();
	//NORMAL_EX_LOG("");
	ID3D11Texture2D *encoder_texture = reinterpret_cast<ID3D11Texture2D*>(input_frame->inputPtr);
	//*ready = 1;
	//NORMAL_EX_LOG("");
	if (lock_key >= 0 && unlock_key >= 0 && keyed_mutex)
	{
		HRESULT hr = keyed_mutex->AcquireSync(lock_key, 3);
		if (hr != S_OK)
		{
			NORMAL_EX_LOG("AcquireSync time out !!!");
			return -1;
		}
	}
	enc->d3d11_context->CopyResource(encoder_texture, texture);
	if (lock_key >= 0 && unlock_key >= 0 && keyed_mutex)
	{
		keyed_mutex->ReleaseSync(unlock_key);
	}
	//*ready = 0;
	//NORMAL_EX_LOG("");
	try
	{
		enc->nvenc->EncodeFrame(out_buf, &packet_size);
	}
	catch (...)
	{
		ERROR_EX_LOG("nvenc encode frame failed !!! packet ");
		return -2;
	}


	/*int frame_size = 0;
	for (std::vector<uint8_t> &packet : packet) {
		if (frame_size + packet.size() < out_buf_size) {
			memcpy(out_buf + frame_size, packet.data(), packet.size());
			frame_size += (int)packet.size();
		}
		else {
			break;
		}
	}*/

	return packet_size;
}

int nvenc_encode_handle(void *nvenc_data, HANDLE handle, int lock_key, int unlock_key, 
	uint8_t* out_buf, uint32_t out_buf_size)
{
	using namespace chen;
	//NORMAL_EX_LOG("");
	struct video_data {
		int ready;
		void * handler;
	};
	// 
	//video_data * video_data_ptr = (video_data*)handle;
	//NORMAL_EX_LOG("");
	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;
	if (nvenc_data == nullptr || handle == nullptr ) {
		return 0;
	}
	
	ID3D11Texture2D* input_texture = enc->input_texture;
	IDXGIKeyedMutex* keyed_mutex = enc->keyed_mutex;
	int frame_size = 0;
	//NORMAL_EX_LOG("[enc->input_handle = %p][handle = %p]", enc->input_handle, handle);
	if (enc->input_handle != handle) 
	{
		WARNING_EX_LOG("[enc->input_handle = %p][handle = %p][handle = %u]", enc->input_handle, handle, handle);
		if (enc->input_texture) {
			enc->input_texture->Release();
			enc->input_texture = nullptr;
		}
		//NORMAL_EX_LOG("");
		if (enc->keyed_mutex) {
			enc->keyed_mutex->Release();
			enc->keyed_mutex = nullptr;
		}
		//NORMAL_EX_LOG("");
		HRESULT hr = enc->d3d11_device->OpenSharedResource((HANDLE)(uintptr_t)handle, __uuidof(ID3D11Texture2D),
			reinterpret_cast<void **>(&enc->input_texture));
		if (FAILED(hr)) 
		{
			WARNING_EX_LOG("[handle = %p]OpenSharedResource  failed !!!", handle);
			return -1;
		}
	//	NORMAL_EX_LOG("");
		input_texture = enc->input_texture;
		//NORMAL_EX_LOG("");
		//if (g_cfg.get_uint32(ECI_GpuVideoLock) > 0)
		{
			//if (lock_key >= 0 && unlock_key >= 0)
			//{
			//	hr = input_texture->QueryInterface(_uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&enc->keyed_mutex));
			//	//NORMAL_EX_LOG("hr = %u", hr);
			//	if (FAILED(hr))
			//	{
			//		enc->input_texture->Release();
			//		enc->input_texture = nullptr;
			//		return -1;
			//	}
			//	//NORMAL_EX_LOG("");
			//	keyed_mutex = enc->keyed_mutex;
			//}
		}
		
		//NORMAL_EX_LOG("");
		enc->input_handle = handle;
	}
	//NORMAL_EX_LOG("");
	if (input_texture != nullptr)
	{
		/*if (g_cfg.get_uint32(ECI_GpuVideoLock) > 0)
		{
			if (lock_key >= 0 && unlock_key >= 0 && keyed_mutex) {
				HRESULT hr = keyed_mutex->AcquireSync(lock_key, 3);
				if (hr != S_OK)
				{
					NORMAL_EX_LOG("AcquireSync time out !!!");
					return -1;
				}
			}
		}*/
		
	//	NORMAL_EX_LOG("");
		//video_data_ptr->ready = 1;
		//int ready = 0;
		frame_size = nvenc_encode_texture_unlock_lock(enc, input_texture , out_buf, out_buf_size, lock_key, unlock_key, keyed_mutex);
		//video_data_ptr->ready = 0;
	//	NORMAL_EX_LOG("");
		/*if (g_cfg.get_uint32(ECI_GpuVideoLock) > 0)
		{
			if (lock_key >= 0 && unlock_key >= 0 && keyed_mutex) {
				keyed_mutex->ReleaseSync(unlock_key);
			}
		}*/
		
	}
	//NORMAL_EX_LOG("");
	return frame_size;
}

int nvenc_set_bitrate(void *nvenc_data, uint32_t bitrate_bps)
{
	if (nvenc_data == nullptr) 
	{
		return 0;
	}
	using namespace chen;
	NORMAL_EX_LOG("------bitrate_bps = %u----->", bitrate_bps);
	/*if (g_cfg.get_uint32(ECI_EnableEncoderCbr) > 0)
	{ 
		return 0;
	}
	if ((bitrate_bps / 1000) > g_cfg.get_uint32(ECI_RtcMaxRate))
	{
		NORMAL_EX_LOG("[bitrate_bps = %u ]too big [defalut max bitrate = %u]", bitrate_bps/ 1000, g_cfg.get_uint32(ECI_RtcMaxRate));
		bitrate_bps = g_cfg.get_uint32(ECI_RtcMaxRate) * 1000;
	}
	else if ((bitrate_bps / 1000) < g_cfg.get_uint32(ECI_RtcAvgRate))
	{
		WARNING_EX_LOG("[bitrate_bps = %u ]too big [defalut avg bitrate = %u]", bitrate_bps / 1000, g_cfg.get_uint32(ECI_RtcAvgRate));
		bitrate_bps = g_cfg.get_uint32(ECI_RtcAvgRate) * 1000;
	}
	 */
	/*if ((bitrate_bps / 1000) > g_cfg.get_uint32(ECI_RtcAvgRate))
	{
		WARNING_EX_LOG("[bitrate_bps = %u ]too big [defalut bitrate = %u]", bitrate_bps / 1000, g_cfg.get_uint32(ECI_RtcAvgRate));
		bitrate_bps = g_cfg.get_uint32(ECI_RtcAvgRate) * 1000;
	}*/
	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);

	if (enc->nvenc != nullptr)
	{
		NV_ENC_RECONFIGURE_PARAMS reconfigureParams = {0};
		reconfigureParams.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		reconfigureParams.forceIDR = 1;
		reconfigureParams.resetEncoder = 1;

		NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
		reconfigureParams.reInitEncodeParams.encodeConfig = &encodeConfig;
		enc->nvenc->GetInitializeParams(&reconfigureParams.reInitEncodeParams);
		reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.averageBitRate = bitrate_bps;// 6000 * 1000;//bitrate_bps ; // bitrate_bps;
		reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.maxBitRate = g_cfg.get_uint32(ECI_RtcMaxRate) * 1000; // bitrate_bps;
		enc->nvenc->Reconfigure(&reconfigureParams);
	}

	return 0;
}

int nvenc_set_framerate(void *nvenc_data, uint32_t framerate) 
{
	if (nvenc_data == nullptr) {
		return 0;
	}
	using namespace chen;
	
	//if (framerate < g_cfg.get_uint32(ECI_RtcFrames))
	{
		NORMAL_EX_LOG("framerate = %u", framerate);
	}
	if (g_cfg.get_uint32(ECI_EnableEncoderCbr) > 0)
	{
		return 0;
	}
	return 0;
 	//NORMAL_EX_LOG("----------->");
	//return 0;
	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);

	if (enc->nvenc != nullptr) {
		NV_ENC_RECONFIGURE_PARAMS reconfigureParams;
		 
		reconfigureParams.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		reconfigureParams.resetEncoder = 1;
		reconfigureParams.forceIDR = 1;
		 
		NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };
		reconfigureParams.reInitEncodeParams.encodeConfig = &encodeConfig;
		enc->nvenc->GetInitializeParams(&reconfigureParams.reInitEncodeParams);
		reconfigureParams.reInitEncodeParams.frameRateNum = framerate;
		enc->nvenc->Reconfigure(&reconfigureParams);
	}

	return 0;
}

int nvenc_request_idr(void *nvenc_data)
{
	using namespace chen;
	//NORMAL_EX_LOG("");
	if (nvenc_data == nullptr)
	{
		return 0;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);

	if (enc->nvenc != nullptr) 
	{
		NORMAL_EX_LOG("request Keyframe");
		enc->nvenc->ForceIDR();
	}
	else
	{
		WARNING_EX_LOG("not request Keyframe");
	}

	return 0;
}

int nvenc_get_sequence_params(void *nvenc_data, uint8_t* outBuf, uint32_t max_buf_size)
{
	using namespace chen;
	//ERROR_EX_LOG("");
	if (nvenc_data == nullptr) {
		return 0;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);

	if (enc->nvenc != nullptr) {
		std::vector<uint8_t> seq_params;
		enc->nvenc->GetSequenceParams(seq_params);
		if (seq_params.size() > 0 && seq_params.size() < max_buf_size) {
			memcpy(outBuf, seq_params.data(), seq_params.size());
			return (int)seq_params.size();
		}		
	}

	return 0;
}

int nvenc_set_region_of_interest(void* nvenc_data, int x, int y, int width, int height, int delta_qp)
{
	if (nvenc_data == nullptr) {
		return 0;
	}
	using namespace chen;
	//ERROR_EX_LOG("");
	struct nvenc_data* enc = (struct nvenc_data*)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);

	if (enc->nvenc != nullptr){ 
		//enc->nvenc->SetROI(x, y, width, height, delta_qp);
	}

	return 0;
}

static ID3D11Device* get_device(void *nvenc_data)
{	
	using namespace chen;
	//ERROR_EX_LOG("");
	if (nvenc_data == nullptr) {
		return nullptr;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);
	return enc->d3d11_device;
}

static ID3D11Texture2D* get_texture(void *nvenc_data)
{
	using namespace chen;
	//ERROR_EX_LOG("");
	if (nvenc_data == nullptr) {
		return nullptr;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);
	return enc->copy_texture;
}


static ID3D11DeviceContext* get_context(void *nvenc_data)
{
	using namespace chen;
	//ERROR_EX_LOG("");
	if (nvenc_data == nullptr) {
		return nullptr;
	}

	struct nvenc_data *enc = (struct nvenc_data *)nvenc_data;

	std::lock_guard<std::mutex> locker(enc->mutex);
	return enc->d3d11_context;
}

struct encoder_info nvenc_info = {
	is_supported,
	nvenc_create,
	nvenc_destroy,
	nvenc_init,
	nvenc_encode_texture,
	nvenc_encode_handle,
	nvenc_set_bitrate,
	nvenc_set_framerate,
	nvenc_request_idr,
	nvenc_get_sequence_params,
	nvenc_set_region_of_interest,
	get_device,
	get_texture,
	get_context
};

}