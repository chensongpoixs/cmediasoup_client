/***********************************************************************************************
created: 		2022-01-20

author:			chensong

purpose:		assertion macros

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
#include "external_video_encoder_factory.h"
//extern "C"
//{
#include "absl/strings/match.h"
#include "absl/memory/memory.h"
//}

#include "media/engine/internal_decoder_factory.h"
#include "rtc_base/logging.h"

#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"

#include "nv_encoder.h"

namespace chen {

class ExternalEncoderFactory : public webrtc::VideoEncoderFactory {
public:
	std::vector<webrtc::SdpVideoFormat> GetSupportedFormats()
		const override {
		std::vector<webrtc::SdpVideoFormat> video_formats;
		for (const webrtc::SdpVideoFormat& h264_format : webrtc::SupportedH264Codecs())
			video_formats.push_back(h264_format);
		return video_formats;
	}

	VideoEncoderFactory::CodecInfo QueryVideoEncoder(
		const webrtc::SdpVideoFormat& format) const override {
		CodecInfo codec_info = { false, false };
		codec_info.is_hardware_accelerated = true;
		codec_info.has_internal_source = false;
		return codec_info;
	}

	std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
		const webrtc::SdpVideoFormat& format) override {
		if (format.name == cricket::kH264CodecName/*absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName)*/) {
			if (webrtc::H264Encoder::IsSupported()) {
				if (nvenc_info.is_supported()) {
					return absl::make_unique<webrtc::NvEncoder>(cricket::VideoCodec(format));
				}
				else {
					return webrtc::H264Encoder::Create(cricket::VideoCodec(format));
				}
			}			
		}
		else
		{
			WARNING_EX_LOG("not find codec = %s", format.name.c_str());
		}

		return nullptr;
	}
};

std::unique_ptr<webrtc::VideoEncoderFactory> CreateBuiltinExternalVideoEncoderFactory() {
	return absl::make_unique<ExternalEncoderFactory>();
}

}


