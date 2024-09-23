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
#include "cwindow_capture_wgc.h"
#include "third_party/libyuv/include/libyuv.h"
#include "cclient.h"
#include "cinput_device.h"
//#include <Unknwn.h>
//#include <inspectable.h>
//#define WIN32_LEAN_AND_MEAN
// WinRT
//#include <winrt/Windows.Foundation.h>
//#include <winrt/Windows.System.h>
//#include <winrt/Windows.Graphics.DirectX.h>
//#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
//#include <winrt/Windows.Graphics.Capture.h>
//#include <Windows.Graphics.Capture.Interop.h>
// D3D
//#include <d3d11_4.h>
//#include <dxgi1_6.h>
//#include <d2d1_3.h>
//#include <wincodec.h>
//#include <DispatcherQueue.h>
#include "cwindow_util.h"

namespace chen {
	/*WindowCapture* WindowCapture::init(size_t target_fps, size_t capture_screen_index)
	{
		return nullptr;
	}*/
	/*WindowCapture::~WindowCapture()
	{
	}*/
	void WindowCapture::StartCapture()
	{
		session_->start();
		m_thread_encode = std::thread(&WindowCapture::_video_pthread, this);
	}
	void WindowCapture::_video_pthread()
	{
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time;
		std::chrono::steady_clock::duration dur;
		std::chrono::microseconds ms;
		uint32_t elapse = 0;
		static const uint32 frame_s = (1000 * 1000) / 60;

		  auto timestamp =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch())
			.count();
		  size_t cnt = 0;
	
		while (true)
		{
			pre_time = std::chrono::steady_clock::now();
			 
			if (shared_ptr)
			{
				//ShowWindow(m_window_wnd, SW_HIDE);
				i420_buffer_->set_texture((void*)shared_ptr);
				//memcpy(i420_buffer_->MutableDataY(), frame.data, frame.width * frame.height * 4);
				/*libyuv::ConvertToI420(frame.data, 0, i420_buffer_->MutableDataY(),
					i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
					i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
					i420_buffer_->StrideV(), 0, 0, frame.width, frame.height, frame.width,
					frame.height, libyuv::kRotate0, libyuv::FOURCC_ARGB);*/

					//static uint64 frame_count = 0;
					//printf("[%s][%u][frame_count = %u][width = %u][height = %u]\n", __FUNCTION__, __LINE__, ++frame_count, frame.width, frame.height);
					// seting ÂíÁ÷µÄÐÅÏ¢

				webrtc::VideoFrame captureFrame =
					webrtc::VideoFrame::Builder()
					.set_video_frame_buffer(i420_buffer_)
					.set_timestamp_rtp(0)
					.set_timestamp_ms(rtc::TimeMillis())
					.set_rotation(webrtc::kVideoRotation_0)
					.build();
				// captureFrame.set_ntp_time_ms(0);
				s_client.webrtc_video(captureFrame);
				cnt++;
				auto timestamp_curr = std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::system_clock::now().time_since_epoch())
					.count();
				if (timestamp_curr - timestamp > 1000) {
					//RTC_LOG(LS_INFO) <<   << cnt;
					NORMAL_EX_LOG("===============>> send --> FPS: %u ", cnt);
					cnt = 0;
					timestamp = timestamp_curr;
				}
			 }
			{
				cur_time_ms = std::chrono::steady_clock::now();
				dur = cur_time_ms - pre_time;
				ms = std::chrono::duration_cast<std::chrono::microseconds>(dur);
				elapse = static_cast<uint32_t>(ms.count());
				
				if (elapse < frame_s)
				{
					
					std::this_thread::sleep_for(std::chrono::microseconds(frame_s - elapse));
				}
				else
				{
					NORMAL_EX_LOG("send frame  = %u msc [frame_s = %u]", elapse, frame_s);
				}
			}

			
		}
	}
	/*void WindowCapture::StopCapture()
	{
	}*/
	void WindowCapture::Destory()
	{
	}

	//auto WindowCapture::create_d3d11_device()
	//{
	//	/*winrt::com_ptr<ID3D11Device> device;
	//	::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0,
	//		D3D11_SDK_VERSION, device.put(), nullptr,
	//		nullptr)*/

	//}
	
	bool WindowCapture::Init(size_t target_fps, size_t capture_screen_index)
	{
		int error = 0;
		fps_ = target_fps;
		do {
			if (!module_.is_supported()) {
				//error = AE_UNSUPPORT;
				break;
			}

			session_ = module_.create_session();
			if (!session_) {
			//	error = AE_WGC_CREATE_CAPTURER_FAILED;
				break;
			}

			session_->register_observer(this);
			  m_window_wnd = FindMainWindow(g_pid);;
			  //::ShowWindow(m_window_wnd, SW_SHOW);
			error = session_->initialize(m_window_wnd);

			
			::SetActiveWindow(m_window_wnd);
			//_inited = true;
		} while (0);

		//if (error != AE_NO) {
			//al_debug("%s,last error:%s", err2str(error),
			//	system_error::error2str(GetLastError()).c_str());
		//}

		//return error;

		return true;
	}
	void WindowCapture::on_frame(const am::wgc_session::wgc_session_frame& frame)
	{
		/*int width = frame->size().width();
		int height = frame->size().height();
		 int half_width = (width + 1) / 2;*/

		/*static auto timestamp =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch())
			.count();
		static size_t cnt = 0;

		cnt++;
		auto timestamp_curr = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch())
			.count();
		if (timestamp_curr - timestamp > 1000) {
			RTC_LOG(LS_INFO) << "FPS: " << cnt;
			printf("fps: %u\n", cnt);
			cnt = 0;
			timestamp = timestamp_curr;
		}*/
		//return;



		 

		/*static uint32 cccc = 0;
		if (++cccc < 19)
		{
			return;
		}*/
		
		if (!i420_buffer_.get() ||
			i420_buffer_->width() * i420_buffer_->height() < frame.width * frame.height) {
			i420_buffer_ = webrtc::I420Buffer::Create(frame.width, frame.height);
		}
		shared_ptr =  frame.data;
		::SetActiveWindow(m_window_wnd);
		return;
		i420_buffer_->set_texture((void *)frame.data);
		//memcpy(i420_buffer_->MutableDataY(), frame.data, frame.width * frame.height * 4);
		/*libyuv::ConvertToI420(frame.data, 0, i420_buffer_->MutableDataY(),
			i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
			i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
			i420_buffer_->StrideV(), 0, 0, frame.width, frame.height, frame.width,
			frame.height, libyuv::kRotate0, libyuv::FOURCC_ARGB);*/

		//static uint64 frame_count = 0;
		//printf("[%s][%u][frame_count = %u][width = %u][height = %u]\n", __FUNCTION__, __LINE__, ++frame_count, frame.width, frame.height);
		// seting ÂíÁ÷µÄÐÅÏ¢

		webrtc::VideoFrame captureFrame =
			webrtc::VideoFrame::Builder()
			.set_video_frame_buffer(i420_buffer_)
			.set_timestamp_rtp(0)
			.set_timestamp_ms(rtc::TimeMillis())
			.set_rotation(webrtc::kVideoRotation_0)
			.build();
		// captureFrame.set_ntp_time_ms(0);
		s_client.webrtc_video(captureFrame);
	}
	/*void WindowCapture::OnCaptureResult(webrtc::DesktopCapturer::Result result, std::unique_ptr<webrtc::DesktopFrame> frame)
	{
	}*/
}

 
