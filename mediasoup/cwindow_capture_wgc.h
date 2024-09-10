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
#ifndef _C_WINDOW_CAPTURE_WGC_H_
#define _C_WINDOW_CAPTURE_WGC_H_
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"
#include "desktop_capture_source.h"
#include "modules/desktop_capture/desktop_capturer.h"
#include "modules/desktop_capture/desktop_frame.h"
#include "api/video/i420_buffer.h"
//#include "wgc_session_impl.h"
#include "WGC/export.h"
#include <thread>
#include <atomic>
namespace chen {

    class WindowCapture : 
        public am::wgc_session::wgc_session_observer
    {
    public:
        class wgc_session_module {
            using func_type_is_supported = bool (*)();
            using func_type_create_session = am::wgc_session * (*)();

        public:
            wgc_session_module() {
                module_ = ::LoadLibraryA("ccapture.dll");
                if (module_) {
                    func_is_supported_ = (func_type_is_supported)::GetProcAddress(
                        module_, "wgc_is_supported");
                    func_create_session_ = (func_type_create_session)::GetProcAddress(
                        module_, "wgc_create_session");
                }
            }
            ~wgc_session_module() {
                if (module_)
                    ::FreeModule(module_);
            }

            bool is_supported() const {
                return func_create_session_ && func_is_supported_();
            }

            am::wgc_session* create_session() const {
                if (!func_create_session_)
                    return nullptr;

                return func_create_session_();
            }

        private:
            HMODULE module_ = nullptr;
            func_type_is_supported func_is_supported_ = nullptr;
            func_type_create_session func_create_session_ = nullptr;
        };
     
    public:
        WindowCapture() {}

        ~WindowCapture()   {}

    public:

        // WindowCapture  init(size_t target_fps, size_t capture_screen_index);
        bool Init(size_t target_fps, size_t capture_screen_index)  ;

      

        std::string GetWindowTitle() const { return window_title_; }

    public:

        void StartCapture()  ;
        //void StopCapture();



         void on_frame(const am::wgc_session::wgc_session_frame& frame) override;
    private:
        
        void _video_pthread();
        void Destory();
    private:
      //  auto create_d3d11_device();
        
        
       

      //  std::unique_ptr<webrtc::DesktopCapturer> dc_;

        size_t fps_;
        std::string window_title_;

        std::unique_ptr<std::thread> capture_thread_;
        std::atomic_bool start_flag_;

        rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;
        am::wgc_session* session_ = nullptr;
        wgc_session_module module_;
        void*  shared_ptr = NULL;

        HWND                m_window_wnd = NULL;
        std::thread         m_thread_encode;
    };
}

#endif // _C_WINDOW_CAPTURE_WGC_H_