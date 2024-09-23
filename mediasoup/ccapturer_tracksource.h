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
#ifndef _C_CAPTURER_TRACKSOURCE_H_
#define _C_CAPTURER_TRACKSOURCE_H_
 
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "desktop_capture_source.h"
#include "desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"
#include "csend_transport.h"
#include "clog.h"
#include "sdptransform.hpp"
#include "sdp/Utils.hpp"
#include "ortc.hpp"
#include "csession_description.h"
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <rtc_base/ssl_adapter.h>
#include "sdptransform.hpp"
#include "sdp/Utils.hpp"
#include "ortc.hpp"
#include "cproducer.h"
#include <memory>
#include "cclient.h"
 
#include "cdataconsumer.h"
#include "csession_description.h"
#include "pc/video_track_source.h"
#include "third_party/libyuv/include/libyuv.h"
#include <thread>
#include <atomic>
#include "ccfg.h"
#include "NvCodec/nvenc.h"
#include "cinput_device.h"
namespace chen {
	 

	class ProxyVideoTrackSource : public webrtc::VideoTrackSource {
	public:
		static rtc::scoped_refptr<ProxyVideoTrackSource> Create()
		{ 
			std::unique_ptr<  VideoCaptureSource> video_source_ptr(VideoCaptureSource::Create( ));
			if (video_source_ptr)
			{
				 
				return new rtc::RefCountedObject<ProxyVideoTrackSource>(std::move(video_source_ptr));
			}
			return nullptr;
		}
		bool OnFrameTexture(void * texture, uint32 fmt, int32_t width, int32_t height)
		{
			if (!texture)
			{
				WARNING_EX_LOG("osg OnFrame texture nullptr !!!");
				return false;
			}
			if (!video_source_ptr)
			{
				WARNING_EX_LOG("capturer_ == nullptr ");
				return false;
			}
			if (width < 1 || height < 1)
			{
				WARNING_EX_LOG("osg  width = %d, height = %d ", width, height);
				return false;
			}

			if (!i420_buffer_.get() ||
				i420_buffer_->width() * i420_buffer_->height() < width * height)
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}
			else if (i420_buffer_->width() * i420_buffer_->height() < width * height)
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}
			//g_pid = fmt;
			i420_buffer_->set_texture(texture);
			g_dxgi_format = static_cast<DXGI_FORMAT>(fmt); // static_cast<DXGI_FORMAT>(fmt);
			//NORMAL_EX_LOG("[fmt = %u][width = %u][height = %u]", fmt, width, height);
			//::memcpy(i420_buffer_->MutableDataY(), rgba, width * height * 4);
			//libyuv::ConvertToI420(rgba, 0, i420_buffer_->MutableDataY(),
			//	i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
			//	i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
			//	i420_buffer_->StrideV(), 0, 0, width, height, width,
			//	height, libyuv::kRotate0, libyuv::FOURCC_ARGB); // GL_BGRA，  FOURCC_BGRA 、、GL_BGR


																//chen::draw_font_func(i420_buffer_->MutableDataY(), i420_buffer_->MutableDataU(), i420_buffer_->MutableDataV(), "A", g_width, g_height, 3, 1, width , height);
			//NORMAL_EX_LOG("");
																// seting 马流的信息 
			{
				webrtc::VideoFrame captureFrame =
					webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(i420_buffer_)
					.set_timestamp_rtp(0)
					.set_timestamp_ms(rtc::TimeMillis())
					.build();
				captureFrame.set_ntp_time_ms(0);

				video_source_ptr->VideoOnFrame(captureFrame);
			}

			return true;
		}
		bool OnFrame(unsigned char * rgba, uint32 fmt, int32_t width, int32_t height)
		{
			if (!rgba)
			{
				WARNING_EX_LOG("osg OnFrame rgba nullptr !!!");
				return false;
			}
			if (!video_source_ptr)
			{
				WARNING_EX_LOG("capturer_ == nullptr ");
				return false;
			}
			if (width < 1 || height < 1)
			{
				WARNING_EX_LOG("osg  width = %d, height = %d ", width, height);
				return false;
			}

			if (!i420_buffer_.get() ||
				i420_buffer_->width() * i420_buffer_->height() < width * height) 
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}
			else if (i420_buffer_->width() * i420_buffer_->height() < width * height) 
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}

			i420_buffer_->set_texture(NULL);
			g_dxgi_format = static_cast<DXGI_FORMAT>(fmt);
		//	NORMAL_EX_LOG("[fmt = %u][width = %u][height = %u]", fmt, width, height);
			//::memcpy(i420_buffer_->MutableDataY(), rgba, width * height * 4);
			libyuv::ConvertToI420(rgba, 0, i420_buffer_->MutableDataY(),
				i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
				i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
				i420_buffer_->StrideV(), 0, 0, width, height, width,
				height, libyuv::kRotate0, libyuv::FOURCC_ARGB); // GL_BGRA，  FOURCC_BGRA 、、GL_BGR


																//chen::draw_font_func(i420_buffer_->MutableDataY(), i420_buffer_->MutableDataU(), i420_buffer_->MutableDataV(), "A", g_width, g_height, 3, 1, width , height);

																// seting 马流的信息 
			{
				webrtc::VideoFrame captureFrame =
					webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(i420_buffer_)
					.set_timestamp_rtp(0)
					.set_timestamp_ms(rtc::TimeMillis())
					.build();
				captureFrame.set_ntp_time_ms(0);
				
				 video_source_ptr-> VideoOnFrame(captureFrame);
			}

			return true;
		}


		bool OnFrame(unsigned char * y_ptr, uint32 fmt, unsigned char *uv_ptr, int32_t width, int32_t height)
		{
			if (!y_ptr )
			{
				WARNING_EX_LOG("osg OnFrame rgba nullptr !!!");
				return false;
			}
			if (!video_source_ptr)
			{
				WARNING_EX_LOG("capturer_ == nullptr ");
				return false;
			}
			if (width < 1 || height < 1)
			{
				WARNING_EX_LOG("osg  width = %d, height = %d ", width, height);
				return false;
			}

			if (!i420_buffer_.get() ||
				i420_buffer_->width() * i420_buffer_->height() < width * height)
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}
			else if (i420_buffer_->width() * i420_buffer_->height() < width * height)
			{
				i420_buffer_ = webrtc::I420Buffer::Create(width, height);
			}
			g_dxgi_format = static_cast<DXGI_FORMAT>(fmt);
			NORMAL_EX_LOG("[fmt = %u][width = %u][height = %u]", fmt, width, height);
			::memcpy(i420_buffer_->MutableDataY(), y_ptr, width * height );
			::memcpy(i420_buffer_->MutableDataU(), uv_ptr, width * height/ 2);
			//NORMAL_EX_LOG("");
			//libyuv::ConvertToI420(rgba, 0, i420_buffer_->MutableDataY(),
			//	i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
			//	i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
			//	i420_buffer_->StrideV(), 0, 0, width, height, width,
			//	height, libyuv::kRotate0, libyuv::FOURCC_ARGB); // GL_BGRA，  FOURCC_BGRA 、、GL_BGR


																//chen::draw_font_func(i420_buffer_->MutableDataY(), i420_buffer_->MutableDataU(), i420_buffer_->MutableDataV(), "A", g_width, g_height, 3, 1, width , height);

																// seting 马流的信息 
			{
				webrtc::VideoFrame captureFrame =
					webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(i420_buffer_)
					.set_timestamp_rtp(0)
					.set_timestamp_ms(rtc::TimeMillis())
					
					.build();
				captureFrame.set_ntp_time_ms(0);

				video_source_ptr->VideoOnFrame(captureFrame);
			}

			return true;
		}
	
		// YUV 
		bool OnFrame(const webrtc::VideoFrame & frame)
		{
			video_source_ptr->VideoOnFrame(frame);
			return true;
		}
	protected:
		explicit ProxyVideoTrackSource(
			std::unique_ptr< VideoCaptureSource> video_source)
			: VideoTrackSource(/*remote=*/false), video_source_ptr(std::move(video_source)) {}

	private:
		rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
			return video_source_ptr.get();
		}
		std::atomic_bool start_flag_  = false;

		rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
		//std::unique_ptr<webrtc::test::VcmCapturer> capturer_;
		std::unique_ptr< VideoCaptureSource> video_source_ptr;
	};
}


#endif // !_C_CAPTURER_TRACKSOURCE_H_