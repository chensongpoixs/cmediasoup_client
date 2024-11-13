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
#include "cmediasoup_mgr.h"
#include "cclient.h"
#include "clog.h"
#include "cinput_device.h"
namespace chen
{
 
	cmediasoup_mgr::cmediasoup_mgr()
		: m_init(false)
		, m_webrtc_pause(false)
	{
	}
	cmediasoup_mgr::~cmediasoup_mgr()
	{
	}
	bool cmediasoup_mgr::init(uint32_t gpu_index)
	{
		//if (!m_stoped)
		m_init = s_client.init(gpu_index);
		return m_init;
	}
	void cmediasoup_mgr::startup(const char* mediasoupIp, uint16_t port
		, const char* roomName, const char* clientName
		, uint32_t reconnect_waittime)
	{
		m_mediasoup_ip = mediasoupIp;
		m_mediasoup_port = port;
		m_room_name = roomName;
		m_client_name = clientName;
		m_reconnect_wait = reconnect_waittime;
		m_thread = std::thread(&cmediasoup_mgr::_mediasoup_thread, this);
	}
	void cmediasoup_mgr::destroy()
	{
		s_client.stop();
		if (m_thread.joinable())
		{
			m_thread.join();
		}
		s_client.Destory();
		m_init  = false;
	}

	void cmediasoup_mgr::webrtc_video(unsigned char * rgba_ptr, uint32_t fmt, int width, int height)
	{
		if (!m_init)
		{
			
			//WARNING_EX_LOG("mediasoup_mgr  not init !!!");
			return;
		}
		if (m_webrtc_pause)
		{
			return;
		}
		s_client.webrtc_video(rgba_ptr, fmt, width, height);
	}
	void cmediasoup_mgr::webrtc_video(unsigned char * y_ptr, unsigned char * uv_ptr, uint32_t fmt, int width, int height)
	{
		if (!m_init)
		{

			//WARNING_EX_LOG("mediasoup_mgr  not init !!!");
			return;
		}
		if (m_webrtc_pause)
		{
			return;
		}
		NORMAL_EX_LOG("");
		s_client.webrtc_video(y_ptr, uv_ptr, fmt, width, height);
	}
	void cmediasoup_mgr::webrtc_texture(void * texture, uint32_t fmt, int width, int height)
	{
		if (!m_init)
		{

			//WARNING_EX_LOG("mediasoup_mgr  not init !!!");
			return;
		}
		if (m_webrtc_pause)
		{
			return;
		}
		//NORMAL_EX_LOG("");
		//webrtc::VideoFrame frame;
		s_client.webrtc_texture(texture, fmt, width, height);
	}
	void cmediasoup_mgr::webrtc_pause()
	{
		m_webrtc_pause = true;
	}
	void cmediasoup_mgr::webrtc_resume()
	{
		m_webrtc_pause = false;
	}
	void cmediasoup_mgr::set_mediasoup_status_callback(mediasoup_status_update_cb callback)
	{
		s_client.set_mediasoup_status_callback(callback);
	}
	void cmediasoup_mgr::set_cinput_device_event_callback(cinput_device_event_cb callback)
	{
		s_client.set_input_device_event_callback(callback);
	}
	bool cmediasoup_mgr::mediasoup_run()
	{
		if (!m_init)
		{
			 
			WARNING_EX_LOG("mediasoup_mgr  not init !!!");
			return false;
		}

		return s_client.webrtc_run();
		//return true;
	}

	 


#if defined(_MSC_VER)



	void cmediasoup_mgr::set_main_window(HWND win)
	{
		//s_input_device;
	}
//	void set_main_window(HWND win);
#endif // #if defined(_MSC_VER)
	void cmediasoup_mgr::_mediasoup_thread()
	{
		SYSTEM_LOG("[info]mediasoupip = %s, port = %u, roomname = %s, client_name = %s, reconnectwiat = %u\n", m_mediasoup_ip.c_str(),
			m_mediasoup_port, m_room_name.c_str(), m_client_name.c_str(), m_reconnect_wait);
		s_client.Loop(m_mediasoup_ip, m_mediasoup_port, m_room_name, m_client_name, m_reconnect_wait);
		SYSTEM_LOG("[%s][%d] mediasoup_thread exit !!! \n", __FUNCTION__, __LINE__);
	}
}