/***********************************************************************************************
created: 		2022-01-19

author:			chensong

purpose:		input_device
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

#ifndef _C_INPUT_DEVICE_H_
#define _C_INPUT_DEVICE_H_
//#include <winnt.h>
#include "cnet_types.h"
#include "api/data_channel_interface.h"
#include <map>
#include "cprotocol.h"
#include "cint_point.h"
#include "csingleton.h"
#include <set>
#include "json.hpp"
#include <unordered_map>
#include <list>
#include <mutex>
extern uint64_t g_render_window;
namespace chen {
	 

	struct cmouse_info 
	{
		uint32  m_mouse_key;
		bool	m_key;
		cmouse_info()
			: m_mouse_key(0)
			, m_key(false) {}
	};

	
	class cinput_device
	{
	private:
		typedef bool(cinput_device::*input_device_handler_type)(const uint8*& Data, uint32 size);
		typedef bool(cinput_device::*rtc_input_device_handler_type)(const nlohmann::json & data);
		typedef std::unordered_map<EToStreamMsg, input_device_handler_type>			M_INPUT_DEVICE_MAP;
		typedef std::unordered_map<EToStreamMsg, rtc_input_device_handler_type>			M_RTC_INPUT_DEVICE_MAP;
	public:

		 cinput_device();
		~cinput_device();



	public:
		bool init();

		bool set_point(int64 x, int64 y);
		void Destroy();
		void startup();
		void update();
	public:

		bool OnMessage(const std::string & consumer_id, const webrtc::DataBuffer& Buffer);
		bool OnMessage(const nlohmann::json & datachannel);
		void insert_message(const webrtc::DataBuffer& Buffer);
	public:
		
		/*
		* 控制信息
		*/
		bool OnRequestQualityControl(const uint8*& Data,   uint32 size);
		/**
		* 输入字符
		*/
		bool OnKeyChar( const uint8*& Data,   uint32 size);
		bool OnRtcKeyChar(const nlohmann::json & data );

		/**
		* 按下键
		*/
		 bool OnKeyDown(const uint8*& Data,   uint32 size);
		 bool OnRtcKeyDown(const nlohmann::json & data);
		/**
		* 松开键
		*/
		bool OnKeyUp(const uint8* &Data,   uint32 size);
		bool OnRtcKeyUp(const nlohmann::json & data);

		/**
		*keydown：按下键盘键
		*	keypress：紧接着keydown事件触发（只有按下字符键时触发）
		*	keyup：释放键盘键
		*/
		bool OnKeyPress(const uint8*& Data,   uint32 size);
		bool OnRtcKeyPress(const nlohmann::json & data);

		/**
		*鼠标进入控制范围 ？
		*/
		bool OnMouseEnter(const uint8*& Data,   uint32 size);
		bool OnRtcMouseEnter(const nlohmann::json & data);

		/** 
		*鼠标离开控制范围？
		*/
		bool OnMouseLeave(const uint8*& Data,   uint32 size);
		bool OnRtcMouseLeave(const nlohmann::json & data);
		/**
		* 鼠标按下  onclick事件
		* right <-> left   
		*/
		bool OnMouseDown(const uint8*& Data,   uint32 size);
		bool OnRtcMouseDown(const nlohmann::json & data);
		/** 
		*鼠标移动
		*/
		bool OnMouseMove(const uint8* &Data,   uint32 size);
		bool OnRtcMouseMove(const nlohmann::json & data);
		/** 
		*松开鼠标
		* 
		*/
		bool OnMouseUp(const uint8*& Data,   uint32 size);
		bool OnRtcMouseUp(const nlohmann::json & data);
		/** 
		* 鼠标双击
		*/
		bool OnMouseDoubleClick(const uint8*& Data,   uint32 size);
		bool OnRtcMouseDoubleClick(const nlohmann::json & data);
		/** 
		* 鼠标的滚轮滚动 
		*/
		bool OnMouseWheel(const uint8*& Data,   uint32 size);
		bool OnRtcMouseWheel(const nlohmann::json & data);
	private:
		void _UnquantizeAndDenormalize(uint16& InOutX, uint16& InOutY);

		void _UnquantizeAndDenormalize(int16& InOutX, int16& InOutY);
	private:
		void _work_pthread();
	private:
		cinput_device(const cinput_device&);
		cinput_device& operator =(const cinput_device&);
		
	private:
		bool								m_stoped;
		M_INPUT_DEVICE_MAP					m_input_device;
		M_RTC_INPUT_DEVICE_MAP				m_rtc_input_device;
		FIntPoint							m_int_point;
		std::map<std::string, std::map<uint32, cmouse_info>>	m_all_consumer; 
		std::string							m_mouse_id; //当前操作的id
		std::list< webrtc::DataBuffer>		m_input_list;
		std::mutex							m_input_mutex;

		std::thread							m_thread;
		std::atomic_bool					m_init;
#if defined(_MSC_VER)
		HWND								m_main_win;
#endif // #if defined(_MSC_VER)
		uint32								m_app_events;
		time_t								m_app_event_time;


		
	};
	//extern cinput_device   g_input_device_mgr;
	#define 	s_input_device chen::csingleton<chen::cinput_device>::get_instance()
}

#endif // _C_INPUT_DEVICE_H_