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
#include "desktop_capture_source.h"

#include "api/video/i420_buffer.h"
#include "api/video/video_rotation.h"
#include "rtc_base/logging.h"
#include <chrono>
#include "third_party/libyuv/include/libyuv.h"
#include "clog.h"

namespace chen {

void VideoCaptureSource::AddOrUpdateSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
    const rtc::VideoSinkWants& wants) {
  broadcaster_.AddOrUpdateSink(sink, wants);
  UpdateVideoAdapter();
}
VideoCaptureSource* VideoCaptureSource::Create()
{
	 std::unique_ptr<VideoCaptureSource> Source_ptr(new VideoCaptureSource());
	 if (Source_ptr)
	 {
		 return Source_ptr.release();
	 }
	 return nullptr;
}
void VideoCaptureSource::RemoveSink(
    rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
  broadcaster_.RemoveSink(sink);
  UpdateVideoAdapter();
}

void VideoCaptureSource::UpdateVideoAdapter() {
  //video_adapter_.OnSinkWants(broadcaster_.wants());
	rtc::VideoSinkWants wants = broadcaster_.wants();
	video_adapter_.OnResolutionFramerateRequest(
		wants.target_pixel_count, wants.max_pixel_count, wants.max_framerate_fps);
}
void VideoCaptureSource::VideoOnFrame(const webrtc::VideoFrame& frame)
{
	OnFrame(frame);
}
void VideoCaptureSource::OnFrame(const webrtc::VideoFrame& frame) 
{
	broadcaster_.OnFrame(frame);
	return;


	int cropped_width = 0;
	int cropped_height = 0;
	int out_width = 0;
	int out_height = 0;

	//if (!video_adapter_.AdaptFrameResolution(
	//	frame.width(), frame.height(), frame.timestamp_us() * 1000,
	//	&cropped_width, &cropped_height, &out_width, &out_height)) {
	//	// Drop frame in order to respect frame rate constraint.
	//	//RTC_LOG(LS_INFO) << "video adapter input failed !!!";
	//	return;
	//}
	// 
	//if (out_height != frame.height() || out_width != frame.width()) { 
	//	// Video adapter has requested a down-scale. Allocate a new buffer and
 //  // return scaled version.
 //  // For simplicity, only scale here without cropping.
	//	rtc::scoped_refptr<webrtc::I420Buffer> scaled_buffer =
	//		webrtc::I420Buffer::Create(out_width, out_height);
	//	scaled_buffer->ScaleFrom(*frame.video_frame_buffer()->ToI420());
	//	webrtc::VideoFrame::Builder new_frame_builder =
	//		webrtc::VideoFrame::Builder()
	//		.set_video_frame_buffer(scaled_buffer)
	//		.set_timestamp_rtp(0)
	//		.set_timestamp_ms(rtc::TimeMillis())
	//		.set_rotation(webrtc::kVideoRotation_0)
	//		.set_timestamp_us(frame.timestamp_us())
	//		.set_id(frame.id());
	//	;

	//	 
	//	broadcaster_.OnFrame(new_frame_builder.build());
	//	
	//} else 
	{
		 
		broadcaster_.OnFrame(frame);
		 
	}
}


}