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
#include "cdesktop_capture.h"
#include "modules/desktop_capture/desktop_capture_options.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv.h"
#include "cclient.h"
namespace chen {



    DesktopCapture::DesktopCapture() : dc_(nullptr), start_flag_(false) {}

    DesktopCapture::~DesktopCapture() {
        Destory();
    }

    void DesktopCapture::Destory() {
        StopCapture();

        if (!dc_)
            return;

        dc_.reset(nullptr);
    }

    DesktopCapture* DesktopCapture::Create(size_t target_fps,
        size_t capture_screen_index) {
        std::unique_ptr<DesktopCapture> dc(new DesktopCapture());
        if (!dc->Init(target_fps, capture_screen_index)) {
            RTC_LOG(LS_WARNING) << "Failed to create DesktopCapture(fps = "
                << target_fps << ")";
            return nullptr;
        }
        return dc.release();
    }

    bool DesktopCapture::Init(size_t target_fps, size_t capture_screen_index)
    {
        // ´°¿Ú
        /*dc_ = webrtc::DesktopCapturer::CreateWindowCapturer(
            webrtc::DesktopCaptureOptions::CreateDefault());*/
            //×ÀÃæ
        webrtc::DesktopCaptureOptions result;
        result.set_allow_directx_capturer(true);
        dc_ = webrtc::DesktopCapturer::CreateScreenCapturer(result);

        if (!dc_)
            return false;

        webrtc::DesktopCapturer::SourceList sources;
        dc_->GetSourceList(&sources);
        if (capture_screen_index > sources.size()) {
            RTC_LOG(LS_WARNING) << "The total sources of screen is " << sources.size()
                << ", but require source of index at "
                << capture_screen_index;
            return false;
        }

        RTC_CHECK(dc_->SelectSource(sources[capture_screen_index].id));
        window_title_ = sources[capture_screen_index].title;
        fps_ = target_fps;

        RTC_LOG(LS_INFO) << "Init DesktopCapture finish window_title = " << window_title_ << " , fps = " << fps_ << "";
        // Start new thread to capture
        return true;
    }

    void DesktopCapture::OnCaptureResult(
        webrtc::DesktopCapturer::Result result,
        std::unique_ptr<webrtc::DesktopFrame> frame) {
        //RTC_LOG(LS_INFO) << "new Frame";

        static auto timestamp =
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
            cnt = 0;
            timestamp = timestamp_curr;
        }

        // Convert DesktopFrame to VideoFrame
        if (result != webrtc::DesktopCapturer::Result::SUCCESS) {
          //  RTC_LOG(LS_ERROR) << "Capture frame faiiled, result: " << result;
           
            return;
        }
        int width = frame->size().width() ;
        int height = frame->size().height() ;
        // int half_width = (width + 1) / 2;

        if (!i420_buffer_.get() ||
            i420_buffer_->width() * i420_buffer_->height() < width * height) {
            i420_buffer_ = webrtc::I420Buffer::Create(width, height);
        }
		//i420_buffer_->set_text();
		//memcpy(i420_buffer_->MutableDataY(), frame->data(), width * height * 4);
      libyuv::ConvertToI420(frame->data(), 0, i420_buffer_->MutableDataY(),
            i420_buffer_->StrideY(), i420_buffer_->MutableDataU(),
            i420_buffer_->StrideU(), i420_buffer_->MutableDataV(),
            i420_buffer_->StrideV(), 0, 0, width, height, width,
            height, libyuv::kRotate0, libyuv::FOURCC_ARGB);


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
       // DesktopCaptureSource::OnFrame(captureFrame);
        // rtc media info 
       /* DesktopCaptureSource::OnFrame(
            webrtc::VideoFrame(i420_buffer_, 0, 0, webrtc::kVideoRotation_0));*/
    }

    void DesktopCapture::StartCapture() {
        if (start_flag_) {
            RTC_LOG(WARNING) << "Capture already been running...";
            return;
        }

        start_flag_ = true;

        // Start new thread to capture
        capture_thread_.reset(new std::thread([this]() {
            dc_->Start(this);

            while (start_flag_) {
                dc_->CaptureFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 60));
            }
            }));
    }

    void DesktopCapture::StopCapture() {
        start_flag_ = false;

        if (capture_thread_ && capture_thread_->joinable()) {
            capture_thread_->join();
        }
    }
  
}