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
#include "nv_encoder.h"

#include <limits>
#include <string>

#include "third_party/openh264/src/codec/api/svc/codec_api.h"
#include "third_party/openh264/src/codec/api/svc/codec_app_def.h"
#include "third_party/openh264/src/codec/api/svc/codec_def.h"
#include "third_party/openh264/src/codec/api/svc/codec_ver.h"
#include "absl/strings/match.h"
#include "common_video/h264/h264_common.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "modules/video_coding/utility/simulcast_rate_allocator.h"
#include "modules/video_coding/utility/simulcast_utility.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/time_utils.h"
#include "system_wrappers/include/metrics.h"
#include "third_party/libyuv/include/libyuv/convert.h"
#include "third_party/libyuv/include/libyuv/scale.h"
#include "third_party/libyuv/include/libyuv.h"
#include "../ccfg.h"
#include "../clog.h"
namespace webrtc {

namespace {
	using namespace chen;
const bool kOpenH264EncoderDetailedLogging = false;

// QP scaling thresholds.
static const int kLowH264QpThreshold = 24;
static const int kHighH264QpThreshold = 37;

// Used by histograms. Values of entries should not be changed.
enum NvVideoEncoderEvent
{
	kH264EncoderEventInit = 0,
	kH264EncoderEventError = 1,
	kH264EncoderEventMax = 16,
};

int NumberOfThreads(int width, int height, int number_of_cores) 
{
	// TODO(hbos): In Chromium, multiple threads do not work with sandbox on Mac,
	// see crbug.com/583348. Until further investigated, only use one thread.
	//  if (width * height >= 1920 * 1080 && number_of_cores > 8) {
	//    return 8;  // 8 threads for 1080p on high perf machines.
	//  } else if (width * height > 1280 * 960 && number_of_cores >= 6) {
	//    return 3;  // 3 threads for 1080p.
	//  } else if (width * height > 640 * 480 && number_of_cores >= 3) {
	//    return 2;  // 2 threads for qHD/HD.
	//  } else {
	//    return 1;  // 1 thread for VGA or less.
	//  }
	// TODO(sprang): Also check sSliceArgument.uiSliceNum om GetEncoderPrams(),
	//               before enabling multithreading here.
  return 1;
}

VideoFrameType ConvertToVideoFrameType(EVideoFrameType type) {
	switch (type) {
	case videoFrameTypeIDR:
		return VideoFrameType::kVideoFrameKey;
	case videoFrameTypeSkip:
	case videoFrameTypeI:
	case videoFrameTypeP:
	case videoFrameTypeIPMixed:
		return VideoFrameType::kVideoFrameDelta;
	case videoFrameTypeInvalid:
		break;
	}
	RTC_NOTREACHED() << "Unexpected/invalid frame type: " << type;
	return VideoFrameType::kEmptyFrame;
}

}  // namespace

// Helper method used by NvVideoEncoder::Encode.
// Copies the encoded bytes from |info| to |encoded_image| and updates the
// fragmentation information of |frag_header|. The |encoded_image->_buffer| may
// be deleted and reallocated if a bigger buffer is required.
//
// After OpenH264 encoding, the encoded bytes are stored in |info| spread out
// over a number of layers and "NAL units". Each NAL unit is a fragment starting
// with the four-byte start code {0,0,0,1}. All of this data (including the
// start codes) is copied to the |encoded_image->_buffer| and the |frag_header|
// is updated to point to each fragment, with offsets and lengths set as to
// exclude the start codes.


static void RtpFragmentize(EncodedImage* encoded_image,
	const VideoFrameBuffer& frame_buffer, cnv_frame_packet&  frame_packet
	/*std::vector<uint8_t>& frame_packet*/,
	RTPFragmentationHeader* frag_header)
{
	
	size_t required_capacity = 0;
	encoded_image->set_size(0);

	required_capacity = frame_packet.use_size;
	//encoded_image->Allocate(required_capacity);

	// TODO(nisse): Use a cache or buffer pool to avoid allocation?
	encoded_image->Allocate(required_capacity);


	///////////////////////////////////////H264 NAL///////////////////////////////////////////////////////

	memcpy(encoded_image->data() + encoded_image->size(), &frame_packet.frame.get()[0], frame_packet.use_size);
	encoded_image->set_size(encoded_image->size() + frame_packet.use_size);
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	
	std::vector<webrtc::H264::NaluIndex> nalus = webrtc::H264::FindNaluIndices(
		encoded_image->data(), encoded_image->size());
	size_t fragments_count = nalus.size();
	// const uint8_t start_code[4] = {0, 0, 0, 1};
	frag_header->VerifyAndAllocateFragmentationHeader(fragments_count);
	for (size_t i = 0; i < nalus.size(); i++) {
		frag_header->fragmentationOffset[i] = nalus[i].payload_start_offset;
		frag_header->fragmentationLength[i] = nalus[i].payload_size;
	}
}




static void RtpFragmentize_webrtc(EncodedImage* encoded_image,
	const VideoFrameBuffer& frame_buffer,
	SFrameBSInfo* info,
	RTPFragmentationHeader* frag_header) {
	// Calculate minimum buffer size required to hold encoded data.
	size_t required_capacity = 0;
	size_t fragments_count = 0;
	for (int layer = 0; layer < info->iLayerNum; ++layer) {
		const SLayerBSInfo& layerInfo = info->sLayerInfo[layer];
		for (int nal = 0; nal < layerInfo.iNalCount; ++nal, ++fragments_count) {
			RTC_CHECK_GE(layerInfo.pNalLengthInByte[nal], 0);
			// Ensure |required_capacity| will not overflow.
			RTC_CHECK_LE(layerInfo.pNalLengthInByte[nal],
				std::numeric_limits<size_t>::max() - required_capacity);
			required_capacity += layerInfo.pNalLengthInByte[nal];
		}
	}
	if (encoded_image->capacity() < required_capacity) {
		// Increase buffer size. Allocate enough to hold an unencoded image, this
		// should be more than enough to hold any encoded data of future frames of
		// the same size (avoiding possible future reallocation due to variations in
		// required size).
		size_t new_capacity = CalcBufferSize(VideoType::kI420, frame_buffer.width(),
			frame_buffer.height());
		if (new_capacity < required_capacity) {
			// Encoded data > unencoded data. Allocate required bytes.
			RTC_LOG(LS_WARNING)
				<< "Encoding produced more bytes than the original image "
				<< "data! Original bytes: " << new_capacity
				<< ", encoded bytes: " << required_capacity << ".";
			new_capacity = required_capacity;
		}
		encoded_image->Allocate(new_capacity);
	}

	// Iterate layers and NAL units, note each NAL unit as a fragment and copy
	// the data to |encoded_image->_buffer|.
	const uint8_t start_code[4] = { 0, 0, 0, 1 };
	frag_header->VerifyAndAllocateFragmentationHeader(fragments_count);
	size_t frag = 0;
	encoded_image->set_size(0);
	for (int layer = 0; layer < info->iLayerNum; ++layer) {
		const SLayerBSInfo& layerInfo = info->sLayerInfo[layer];
		// Iterate NAL units making up this layer, noting fragments.
		size_t layer_len = 0;
		for (int nal = 0; nal < layerInfo.iNalCount; ++nal, ++frag) {
			// Because the sum of all layer lengths, |required_capacity|, fits in a
			// |size_t|, we know that any indices in-between will not overflow.
			RTC_DCHECK_GE(layerInfo.pNalLengthInByte[nal], 4);
			RTC_DCHECK_EQ(layerInfo.pBsBuf[layer_len + 0], start_code[0]);
			RTC_DCHECK_EQ(layerInfo.pBsBuf[layer_len + 1], start_code[1]);
			RTC_DCHECK_EQ(layerInfo.pBsBuf[layer_len + 2], start_code[2]);
			RTC_DCHECK_EQ(layerInfo.pBsBuf[layer_len + 3], start_code[3]);
			frag_header->fragmentationOffset[frag] =
				encoded_image->size() + layer_len + sizeof(start_code);
			frag_header->fragmentationLength[frag] =
				layerInfo.pNalLengthInByte[nal] - sizeof(start_code);
			layer_len += layerInfo.pNalLengthInByte[nal];
		}
		// Copy the entire layer's data (including start codes).
		memcpy(encoded_image->data() + encoded_image->size(), layerInfo.pBsBuf,
			layer_len);
		encoded_image->set_size(encoded_image->size() + layer_len);
	}
}

NvEncoder::NvEncoder(const cricket::VideoCodec& codec)
    : packetization_mode_(H264PacketizationMode::SingleNalUnit),
      max_payload_size_(0),
      number_of_cores_(0),
      encoded_image_callback_(nullptr),
      has_reported_init_(false),
      has_reported_error_(false),
      num_temporal_layers_(1),
      tl0sync_limit_(0)
	, m_key_frame_count(0)
{
	//RTC_CHECK(/*absl::EqualsIgnoreCase(codec.name, cricket::kH264CodecName)*/ codec.name == cricket::kH264CodecName);
	std::string packetization_mode_string;
	if (codec.GetParam(cricket::kH264FmtpPacketizationMode, &packetization_mode_string) 
		&& packetization_mode_string == "1") {
		packetization_mode_ = H264PacketizationMode::NonInterleaved;
	}

	encoded_images_.reserve(kMaxSimulcastStreams);
	nv_encoders_.reserve(kMaxSimulcastStreams);
	configurations_.reserve(kMaxSimulcastStreams);
	image_buffer_ = nullptr;
}

NvEncoder::~NvEncoder() 
{
	Release();
}

int32_t NvEncoder::InitEncode(const VideoCodec* inst,
	int32_t number_of_cores,
	size_t max_payload_size)
{
	using namespace chen;
	ReportInit();
	if (!inst || inst->codecType != kVideoCodecH264) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (inst->maxFramerate == 0) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}
	if (inst->width < 1 || inst->height < 1) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
	}

	int32_t release_ret = Release();
	if (release_ret != WEBRTC_VIDEO_CODEC_OK) {
		ReportError();
		return release_ret;
	}

	int number_of_streams = SimulcastUtility::NumberOfSimulcastStreams(*inst);
	bool doing_simulcast = (number_of_streams > 1);

	if (doing_simulcast && !SimulcastUtility::ValidSimulcastParameters(*inst, number_of_streams)) {
		return WEBRTC_VIDEO_CODEC_ERR_SIMULCAST_PARAMETERS_NOT_SUPPORTED;
	}

	assert(number_of_streams == 1);

	encoded_images_.resize(number_of_streams);
	nv_encoders_.resize(number_of_streams);
	configurations_.resize(number_of_streams);

	number_of_cores_ = number_of_cores;
	max_payload_size_ = max_payload_size;
	codec_ = *inst;

	// Code expects simulcastStream resolutions to be correct, make sure they are
	// filled even when there are no simulcast layers.
	if (codec_.numberOfSimulcastStreams == 0) {
		codec_.simulcastStream[0].width = codec_.width;
		codec_.simulcastStream[0].height = codec_.height;
	}

	num_temporal_layers_ = codec_.H264()->numberOfTemporalLayers;

	for (int i = 0, idx = number_of_streams - 1; i < number_of_streams; ++i, --idx) {
		// Store nvidia encoder.
		nv_encoders_[i] = nvenc_info.create();

		// Set internal settings from codec_settings
		configurations_[i].simulcast_idx = idx;
		configurations_[i].sending = false;
		configurations_[i].width = codec_.simulcastStream[idx].width;
		configurations_[i].height = codec_.simulcastStream[idx].height;
		configurations_[i].max_frame_rate = static_cast<float>(codec_.maxFramerate);
		configurations_[i].frame_dropping_on = codec_.H264()->frameDroppingOn;
		configurations_[i].key_frame_interval = codec_.H264()->keyFrameInterval;

		// Codec_settings uses kbits/second; encoder uses bits/second.
		configurations_[i].max_bps = codec_.maxBitrate * 1000;
		configurations_[i].target_bps = codec_.startBitrate * 1000;

		encoder_config nvenc_config;
		nvenc_config.codec = "h264";
		// ÉèÖÃÏñËØ¸ñÊ½
		nvenc_config.format =  g_dxgi_format;  //DXGI_FORMAT_B8G8R8A8_UNORM;
		NORMAL_EX_LOG("[format = %u]", g_dxgi_format);
		nvenc_config.width = configurations_[i].width;
		nvenc_config.height = configurations_[i].height;
		nvenc_config.framerate = (uint32_t)configurations_[i].max_frame_rate;
		//TODO@chensong 2022-03-18 ÉèÖÃGOP µÄ´óÐ¡ ¹þ ^_^ ²»ÖªµÀÎªÊ²Ã´ webrtcÖÐÉèÖÃgop key_frame_interval ×Ö¶ÎÊÇÓÐÐ§µÄ¡¢
		//¸ù¾Ý³ÌÐò
		using namespace chen;
		nvenc_config.gop = NVENC_INFINITE_GOPLENGTH; // g_cfg.get_uint32(ECI_EncoderVideoGop);// configurations_[i].key_frame_interval;
		nvenc_config.bitrate = 100000;
		if (!nvenc_info.init(nv_encoders_[i], &nvenc_config)) {
			Release();
			ReportError();
			return WEBRTC_VIDEO_CODEC_ERROR;
		}

		image_buffer_.reset(new uint8_t[configurations_[i].width * configurations_[i].height * 10]);

		// TODO(pbos): Base init params on these values before submitting.
		video_format_ = EVideoFormatType::videoFormatI420;

		// Initialize encoded image. Default buffer size: size of unencoded data.
		const size_t new_capacity = CalcBufferSize(VideoType::kI420,
			codec_.simulcastStream[idx].width, codec_.simulcastStream[idx].height);
		encoded_images_[i].Allocate((new_capacity));
		encoded_images_[i]._completeFrame = true;
		encoded_images_[i]._encodedWidth = codec_.simulcastStream[idx].width;
		encoded_images_[i]._encodedHeight = codec_.simulcastStream[idx].height;
		encoded_images_[i].set_size(0);
	}

	SimulcastRateAllocator init_allocator(codec_);
	/*VideoBitrateAllocation allocation = init_allocator.GetAllocation(
		codec_.startBitrate * 1000, codec_.maxFramerate);
	SetRates(RateControlParameters(allocation, codec_.maxFramerate));
	return WEBRTC_VIDEO_CODEC_OK;*/
	VideoBitrateAllocation allocation = init_allocator.GetAllocation(
		codec_.startBitrate * 1000, codec_.maxFramerate);
	return SetRateAllocation(allocation, codec_.maxFramerate);
}

int32_t NvEncoder::Release() 
{
	while (!nv_encoders_.empty()) 
	{
		void* nv_encoder = nv_encoders_.back();
		if (nv_encoder) {
			nvenc_info.destroy(&nv_encoder);
			nv_encoder = nullptr;
		}
		nv_encoders_.pop_back();
	}

	configurations_.clear();
	encoded_images_.clear();

	return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvEncoder::RegisterEncodeCompleteCallback(EncodedImageCallback* callback) 
{
	encoded_image_callback_ = callback;
	return WEBRTC_VIDEO_CODEC_OK;
}

void NvEncoder::SetRates(const RateControlParameters& parameters)
{
	if (parameters.bitrate.get_sum_bps() == 0) {
		// Encoder paused, turn off all encoding.
		for (size_t i = 0; i < configurations_.size(); ++i)
			configurations_[i].SetStreamState(false);
		return;
	}

	// At this point, bitrate allocation should already match codec settings.
	if (codec_.maxBitrate > 0)
		RTC_DCHECK_LE(parameters.bitrate.get_sum_kbps(), codec_.maxBitrate);
	RTC_DCHECK_GE(parameters.bitrate.get_sum_kbps(), codec_.minBitrate);
	if (codec_.numberOfSimulcastStreams > 0)
		RTC_DCHECK_GE(parameters.bitrate.get_sum_kbps(), codec_.simulcastStream[0].minBitrate);

	codec_.maxFramerate = parameters.framerate_fps;

	size_t stream_idx = nv_encoders_.size() - 1;
	for (size_t i = 0; i < nv_encoders_.size(); ++i, --stream_idx) {
		configurations_[i].target_bps = parameters.bitrate.GetSpatialLayerSum(stream_idx);
		configurations_[i].max_frame_rate = static_cast<float>(parameters.framerate_fps);

		if (configurations_[i].target_bps) {
			configurations_[i].SetStreamState(true);

			if (nv_encoders_[i]) {
				nvenc_info.set_bitrate(nv_encoders_[i], configurations_[i].target_bps);
				nvenc_info.set_framerate(nv_encoders_[i], (uint32_t)configurations_[i].max_frame_rate);
			}
			else {
				configurations_[i].SetStreamState(false);
			}
		} 
	}
}

int32_t NvEncoder::SetRateAllocation(const VideoBitrateAllocation & bitrate, uint32_t framerate)
{
	if (nv_encoders_.empty())
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;

	
	if (bitrate.get_sum_bps() == 0) {
		// Encoder paused, turn off all encoding.
		for (size_t i = 0; i < configurations_.size(); ++i)
			configurations_[i].SetStreamState(false);
		return WEBRTC_VIDEO_CODEC_OK;
	}

	// At this point, bitrate allocation should already match codec settings.
	if (codec_.maxBitrate > 0)
		RTC_DCHECK_LE(bitrate.get_sum_kbps(), codec_.maxBitrate);
	RTC_DCHECK_GE(bitrate.get_sum_kbps(), codec_.minBitrate);
	if (codec_.numberOfSimulcastStreams > 0)
		RTC_DCHECK_GE(bitrate.get_sum_kbps(), codec_.simulcastStream[0].minBitrate);

	codec_.maxFramerate = framerate;

	size_t stream_idx = nv_encoders_.size() - 1;
	for (size_t i = 0; i < nv_encoders_.size(); ++i, --stream_idx) {
		configurations_[i].target_bps = bitrate.GetSpatialLayerSum(stream_idx);
		configurations_[i].max_frame_rate = static_cast<float>(framerate);

		if (configurations_[i].target_bps) {
			configurations_[i].SetStreamState(true);

			if (nv_encoders_[i]) {
				nvenc_info.set_bitrate(nv_encoders_[i], configurations_[i].target_bps);
				nvenc_info.set_framerate(nv_encoders_[i], (uint32_t)configurations_[i].max_frame_rate);
			}
			else {
				configurations_[i].SetStreamState(false);
			}
		}
	}

	return WEBRTC_VIDEO_CODEC_OK;
}
static auto timestamp =
std::chrono::duration_cast<std::chrono::milliseconds>(
	std::chrono::system_clock::now().time_since_epoch())
	.count();
static size_t cnt = 0;
int32_t NvEncoder::Encode(const VideoFrame& input_frame,
						  const std::vector<VideoFrameType>* frame_types)
{



	 
	

	cnt++;
	auto timestamp_curr = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch())
		.count();
	if (timestamp_curr - timestamp > 1000) {
		//RTC_LOG(LS_INFO) <<   << cnt;
		NORMAL_EX_LOG("===============>> encode --> FPS: %u ", cnt);
		cnt = 0;
		timestamp = timestamp_curr;
	}
	std::chrono::steady_clock::time_point cur_time_ms;
	std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
	std::chrono::steady_clock::duration dur;
	std::chrono::milliseconds milliseconds;
	uint32_t elapse = 0;
	NORMAL_EX_LOG("--=====>");
	if (nv_encoders_.empty()) {
		ReportError();
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}
	if (!encoded_image_callback_) {
		RTC_LOG(LS_WARNING)
			<< "InitEncode() has been called, but a callback function "
			<< "has not been set with RegisterEncodeCompleteCallback()";
		ReportError();
		return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
	}

	rtc::scoped_refptr<const I420BufferInterface> frame_buffer = input_frame.video_frame_buffer()->ToI420();

	bool send_key_frame = false;
	for (size_t i = 0; i < configurations_.size(); ++i) {
		if (configurations_[i].key_frame_request && configurations_[i].sending) {
			send_key_frame = true;
			break;
		}
	}
	if (!send_key_frame && frame_types) {
		for (size_t i = 0; i < frame_types->size() && i < configurations_.size(); ++i) {
			if ((*frame_types)[i] == VideoFrameType::kVideoFrameKey && configurations_[i].sending) {
				send_key_frame = true;
				break;
			}
		}
	}
	if (send_key_frame)
	{
		m_key_frame_count = 0;
	}
	RTC_DCHECK_EQ(configurations_[0].width, frame_buffer->width());
	RTC_DCHECK_EQ(configurations_[0].height, frame_buffer->height());

	// Encode image for each layer.
	for (size_t i = 0; i < nv_encoders_.size(); ++i) {
		if (!configurations_[i].sending) {
			continue;
		}

		if (frame_types != nullptr) {
			// Skip frame?
			if ((*frame_types)[i] == VideoFrameType::kEmptyFrame) {
				continue;
			}
		}

		if (send_key_frame) {			
			if (!nv_encoders_.empty() && nv_encoders_[i]) 
			{
				
				nvenc_info.request_idr(nv_encoders_[i]);
			}

			configurations_[i].key_frame_request = false;
		}

		// EncodeFrame output.
		SFrameBSInfo info;
		memset(&info, 0, sizeof(SFrameBSInfo));
		//std::vector<uint8_t> frame_packet;
		cnv_frame_packet  frame_packet;
		++m_key_frame_count;
		if (!EncodeFrame((int)i, input_frame, frame_packet))
		{
			ERROR_EX_LOG("encode frame failed !!!");
			return false;
		}

		if (frame_packet.use_size <= 0) 
		{
			NORMAL_EX_LOG("frame_packet.use_size == 0");
			return WEBRTC_VIDEO_CODEC_OK;
		}
		else {
			NORMAL_EX_LOG("frame size = %u", frame_packet.use_size);
			if (frame_packet.frame.get()[4] == 0x67)
			{
				NORMAL_EX_LOG(" I frame  = %u", m_key_frame_count);
				m_key_frame_count = 0;
				info.eFrameType = videoFrameTypeIDR;
			}
			else if (frame_packet.frame.get()[4] == 0x61)
			{
				NORMAL_EX_LOG(" P frame ");
				info.eFrameType = videoFrameTypeP;
			}
			else {
				NORMAL_EX_LOG(" B frame ");
				info.eFrameType = videoFrameTypeP;
			}
		}

		 static FILE* out_test_h264_ptr = ::fopen("chensong_h.h264", "wb+");
		if (out_test_h264_ptr)
		{
			fwrite(&frame_packet.frame.get()[0], 1, frame_packet.use_size, out_test_h264_ptr);
			fflush(out_test_h264_ptr);
		} 
		encoded_images_[i]._encodedWidth = configurations_[i].width;
		encoded_images_[i]._encodedHeight = configurations_[i].height;
		encoded_images_[i].SetTimestamp(input_frame.timestamp());
		encoded_images_[i].ntp_time_ms_ = input_frame.ntp_time_ms();
		encoded_images_[i].capture_time_ms_ = input_frame.render_time_ms();
		encoded_images_[i].rotation_ = input_frame.rotation();
		encoded_images_[i].SetColorSpace(input_frame.color_space());
		encoded_images_[i].content_type_ =
			(codec_.mode == VideoCodecMode::kScreensharing) ? VideoContentType::SCREENSHARE : VideoContentType::UNSPECIFIED;
		encoded_images_[i].timing_.flags = VideoSendTiming::kInvalid;
		encoded_images_[i]._frameType = ConvertToVideoFrameType(info.eFrameType);
		encoded_images_[i].SetSpatialIndex(configurations_[i].simulcast_idx);

		// Split encoded image up into fragments. This also updates
		// |encoded_image_|.
		RTPFragmentationHeader frag_header;
		RtpFragmentize(&encoded_images_[i], *frame_buffer, frame_packet , &frag_header);
		//RtpFragmentize_webrtc(&encoded_images_[i], *frame_buffer, &info, &frag_header);

		// Encoder can skip frames to save bandwidth in which case
		// |encoded_images_[i]._length| == 0.
		if (encoded_images_[i].size() > 0) {
			// Parse QP.
			h264_bitstream_parser_.ParseBitstream(encoded_images_[i].data(), encoded_images_[i].size());
			h264_bitstream_parser_.GetLastSliceQp(&encoded_images_[i].qp_);
			//encoded_images_[i].qp_ = 10;

			// Deliver encoded image.
			CodecSpecificInfo codec_specific;
			codec_specific.codecType = kVideoCodecH264;
			codec_specific.codecSpecific.H264.packetization_mode = packetization_mode_;
			codec_specific.codecSpecific.H264.temporal_idx = kNoTemporalIdx;
			codec_specific.codecSpecific.H264.idr_frame = info.eFrameType == videoFrameTypeIDR;
			codec_specific.codecSpecific.H264.base_layer_sync = true;

			// if (info.eFrameType == videoFrameTypeIDR &&
			//    encoded_images_[i]._frameType == kVideoFrameKey) {
			//  RTC_LOG(LS_ERROR) << "send idr frame - " << encoded_images_[i].size();
			//}

			encoded_image_callback_->OnEncodedImage(encoded_images_[i], &codec_specific, &frag_header);
		}
	}
	cur_time_ms = std::chrono::steady_clock::now();
	dur = cur_time_ms - pre_time;
	milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
	elapse = static_cast<uint32_t>(milliseconds.count());
	if (elapse > 3)
	{
		WARNING_EX_LOG("encoder video  frame  milliseconds = %lu", elapse);
	}
	return WEBRTC_VIDEO_CODEC_OK;
}

void NvEncoder::ReportInit() 
{
	if (has_reported_init_)
		return;
	RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.NvEncoder.Event",
							kH264EncoderEventInit, kH264EncoderEventMax);
	has_reported_init_ = true;
}

void NvEncoder::ReportError() 
{
	if (has_reported_error_)
		return;
	RTC_HISTOGRAM_ENUMERATION("WebRTC.Video.NvEncoder.Event",
							kH264EncoderEventError, kH264EncoderEventMax);
	has_reported_error_ = true;
}

VideoEncoder::EncoderInfo NvEncoder::GetEncoderInfo() const 
{
	EncoderInfo info;
	info.supports_native_handle = false;
	info.implementation_name = "NvEncoder";
	info.scaling_settings = VideoEncoder::ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
	info.is_hardware_accelerated = true;
	info.has_internal_source = false;
	return info;
}

void NvEncoder::LayerConfig::SetStreamState(bool send_stream) 
{
	if (send_stream && !sending) {
		// Need a key frame if we have not sent this stream before.
		key_frame_request = true;
	}
	sending = send_stream;
}
//static FILE *out_file_ptr = ::fopen("test_chensong2.rgb", "wb+");
bool NvEncoder::EncodeFrame(int index, const VideoFrame& input_frame, cnv_frame_packet& frame_packet
							/*std::vector<uint8_t>& frame_packet*/) 
{
	//frame_packet.clear();
	//NORMAL_EX_LOG("");
	if (nv_encoders_.empty() || !nv_encoders_[index])
	{
		ERROR_EX_LOG("nv encoder empty !!!");
		return false;
	}

	/*
	if (video_format_ == EVideoFormatType::videoFormatI420) {
		if (image_buffer_ != nullptr) {
			if (webrtc::ConvertFromI420(input_frame, webrtc::VideoType::kARGB, 0,
										image_buffer_.get()) < 0) {
				return false;
			}
		} 
		else {
			return false;
		}
	}
	*/

	int width = input_frame.width();
	int height = input_frame.height();
	 
	//NORMAL_EX_LOG("gpu_index = %u", g_gpu_index);
	ID3D11Texture2D* texture = nvenc_info.get_texture(nv_encoders_[index]);
	ID3D11DeviceContext* context = nvenc_info.get_context(nv_encoders_[index]);
	 if (texture && context  )
	{
		// NORMAL_EX_LOG("");
		 int max_buffer_size = height * width * 4;
		 //std::shared_ptr<uint8_t> out_buffer(new uint8_t[max_buffer_size]);
		 frame_packet.frame.reset(new uint8_t[max_buffer_size]);
		/* if (g_dxgi_format == DXGI_FORMAT_R10G10B10A2_UNORM)
		 {
			 NORMAL_EX_LOG("");
			 D3D11_MAPPED_SUBRESOURCE dsec = { 0 };
			 HRESULT hr = context->Map(texture, D3D11CalcSubresource(0, 0, 0), D3D11_MAP_WRITE, 0, &dsec);
			 if (SUCCEEDED(hr))
			 {

				 NORMAL_EX_LOG("");
				 libyuv::ARGBCopy(input_frame.video_frame_buffer()->ToI420()->DataY(), width * 4, (uint8_t*)dsec.pData, dsec.RowPitch, width, height);

				 NORMAL_EX_LOG("");
				 context->Unmap(texture, D3D11CalcSubresource(0, 0, 0));
				 NORMAL_EX_LOG("");
				 int frame_size = nvenc_info.encode_texture(nv_encoders_[index], texture, 0, frame_packet.frame.get(), max_buffer_size);
				 NORMAL_EX_LOG("");
				 frame_packet.use_size = frame_size;
				 if (frame_packet.use_size< 1)
				 {
					 ERROR_EX_LOG("encoder texture  frame_size = %d !!!!", frame_size);
					 return false;
				 }

			 }
		 }
		 else*/ 
		
		 if ( 0 != g_gpu_index  )
		 {
			// NORMAL_EX_LOG("");
			 D3D11_MAPPED_SUBRESOURCE dsec = { 0 };
			 HRESULT hr = context->Map(texture, D3D11CalcSubresource(0, 0, 0), D3D11_MAP_WRITE, 0, &dsec);
			 if (SUCCEEDED(hr)) {

				// NORMAL_EX_LOG("");
				 libyuv::ARGBCopy(input_frame.video_frame_buffer()->ToI420()->DataY(), width * 4, (uint8_t*)dsec.pData, dsec.RowPitch, width, height);

				// NORMAL_EX_LOG("");
				 context->Unmap(texture, D3D11CalcSubresource(0, 0, 0));
				// NORMAL_EX_LOG("");
				 int frame_size =   nvenc_info.encode_texture(nv_encoders_[index], texture, 0, frame_packet.frame.get(), max_buffer_size);
				// NORMAL_EX_LOG("");
				 frame_packet.use_size = frame_size;
				 if (frame_packet.use_size < 0) 
				 {
					 ERROR_EX_LOG("encoder texture  frame_size = %d !!!!", frame_size);
					 return false;
				 }

			 }
			 
		 }
		 else if (input_frame.video_frame_buffer()->ToI420()->get_texture())
		 {
			// NORMAL_EX_LOG("");
			 int frame_size = nvenc_info.encode_handle((void*)nv_encoders_[index], (HANDLE)input_frame.video_frame_buffer()->ToI420()->get_texture(), 0, 0, frame_packet.frame.get(), max_buffer_size); ;
			 frame_packet.use_size = frame_size;
			 if (frame_packet.use_size < 1)
			 {
				 ERROR_EX_LOG("encoder texture  frame_size = %d !!!!", frame_size);
				 return false;
			 }
		 }
		 else
		 {
			 WARNING_EX_LOG("encoder texture type error !!!");
		 }
	}
	else
	{
		ERROR_EX_LOG("context failed !!!");
		return false;
	}

	return true;
}

}  // namespace webrtc
