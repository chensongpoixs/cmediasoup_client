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
#include "crtc_client.h"

#include "ortc.hpp"
#include "clog.h"
#include "ccfg.h"
#include "cwebsocket_mgr.h"
#include <iostream>
#include <io.h>
#define _CRT_SECURE_NO_WARNINGS
#include "cdevice.h"
#include "pc/video_track_source.h"
#include "crecv_transport.h"
#include "csend_transport.h"
#include "cinput_device.h"
#include "NvCodec/nvenc.h"
#include "build_version.h"
namespace chen {

	static const uint32_t TICK_TIME = 200;


	


	static void check_file(const char* file_name)
	{

		if (::_access(file_name, 0) != 0)
		{
			FILE* fp = ::fopen(file_name, "wb+");
			if (!fp)
			{
				// return;
				return;
			}
			::fclose(fp);
			fp = NULL;
		}
	}
	 	
	crtc_client::~crtc_client()
	{
	}
	bool crtc_client::init(uint32 gpu_index)
	{
		printf("Log init ...\n");
		if (!LOG::init(ELogStorageScreenFile))
		{
			std::cerr << " log init failed !!!";
			return false;
		}
	//	show_work_dir();

		SYSTEM_LOG("Log init ...\n");
		g_gpu_index = gpu_index;
		SYSTEM_LOG("gpu index = %u", g_gpu_index);
		static const   char* config_file = "client.cfg";
		check_file(config_file);
		bool init = g_cfg.init(config_file);
		if (!init)
		{
			//	//RTC_LOG(LS_ERROR) << "config init failed !!!" << config_name;
			ERROR_EX_LOG("config init failed !!! config_name = %s", config_file);
			return false;
		}
		SYSTEM_LOG("config init ok !!!");
		// set log level
		LOG::set_level(static_cast<ELogLevelType>(g_cfg.get_uint32(ECI_LogLevel)));

		SYSTEM_LOG("set level = %u", g_cfg.get_uint32(ECI_LogLevel));

		if (!s_input_device.init())
		{
			ERROR_EX_LOG("init input_device mouble !!!  ");
			return false;
		} 
		SYSTEM_LOG("input device  init ok !!!");
		 
		 
		 
		return true;
	}
	void crtc_client::Loop(const std::string& rtc_ip, uint16_t rtc_port, const std::string& roomName, const std::string& clientName
		, uint32_t reconnect_waittime)
	{
		std::string ws_url = "ws://" + rtc_ip + ":" + std::to_string(rtc_port) + "/?roomId=" + roomName + "&peerId=" + clientName;//ws://127.0.0.1:8888/?roomId=chensong&peerId=xiqhlyrn", "http://127.0.0.1:8888")
		//std::string origin = "http://" + mediasoupIp + ":" + std::to_string(port);
		std::list<std::string> msgs;
		time_t cur_time = ::time(NULL);
		NORMAL_EX_LOG("ws_url = %s", ws_url.c_str());
		m_room_name = roomName;
		m_user_name = clientName;
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration dur;
		std::chrono::milliseconds ms;
		uint32_t elapse = 0;
		 
		while (!m_stoped)
		{
			pre_time = std::chrono::steady_clock::now();
			s_input_device.update();
			switch (m_status)
			{
			case ERtc_WebSocket_Init: // None
			case ERtc_None:
			{  // websocket connect 
				// 1. connect suucer status -> 1
				//g_websocket_mgr.destroy();
				if (!m_websocket_mgr.init(ws_url))
				{
					//RTC_LOG(LS_ERROR) << "weboscket connect failed !!! url = " << ws_url;
					WARNING_EX_LOG("weboscket connect url = %s [count = %u]failed !!!   ", ws_url.c_str(), ++m_websocket_timer);
					m_status = ERtc_Reset;
					 
					 if (m_websocket_timer > g_cfg.get_uint32(ECI_WebSocketTimers))
					{
						 abort();
						 assert(1);
						//_mediasoup_status_callback(EMediasoup_WebSocket_Init, 1);
					} 

					continue;;
				}

				if (!m_websocket_mgr.startup())
				{
					m_status = ERtc_Reset;
					continue;
				}
				m_status = ERtc_WebSocket;
				m_rtc_publisher = new rtc::RefCountedObject<chen::crtc_publisher>(this);
				m_rtc_publisher->create_offer();
				m_websocket_timer = 0;
				// 1.1 获取服务器的处理能力
				// 2. connect failed wait 5s..
				break;
			} 
			case ERtc_WebSocket: // wait server msg 
			{

				m_websocket_mgr.presssmsg(msgs);


				if (!msgs.empty() && m_websocket_mgr.get_status() == CWEBSOCKET_MESSAGE)
				{
					_presssmsg(msgs);
				}

				if (m_websocket_mgr.get_status() != CWEBSOCKET_MESSAGE)
				{
					m_status = ERtc_Reset;
					msgs.clear();
				}
			 
				break;
			}
			case ERtc_WebSocket_Close:
			{
				// 1.  send_transport destroy --> 
				// 2. recv_transport destroy --> 
				// 3. status -> reconnect wait --> 
				m_status = ERtc_WebSocket_Wait;
				break;
			}
			case ERtc_WebSocket_Wait:
			{
				// 10 sleep 
				// TODO@chensong 20220208 ---> 增加时间
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
				m_status = ERtc_WebSocket_Init;
				break;
			}
			case ERtc_Reset:
			case ERtc_Destory:
			{
				 
				m_websocket_mgr.destroy();
				 
				cur_time = ::time(NULL) + 5;
				NORMAL_EX_LOG("reconnect  wait [%u s]  ...", 5);
				m_status = ERtc_Wait;
				break;
			}
			case  ERtc_Wait:
			{
				if (cur_time < ::time(NULL))
				{
					m_status = ERtc_WebSocket_Init;
				}
				break;
			}
			case ERtc_Exit:
			{

				break;
			}
			default:
			{
				ERROR_EX_LOG("client not find status = %u", m_status);
			}
			break;
			}
			if (!m_stoped)
			{
				cur_time_ms = std::chrono::steady_clock::now();
				dur = cur_time_ms - pre_time;
				ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
				elapse = static_cast<uint32_t>(ms.count());
				if (elapse < TICK_TIME)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(TICK_TIME - elapse));
				}

			}


		}
		NORMAL_EX_LOG("rtc Loop exit !!!");
	}
	void crtc_client::destroy()
	{
	}
	void crtc_client::stop()
	{
	}
	bool crtc_client::rtc_texture(void * texture, uint32 fmt, int32_t width, int32_t height)
	{
		if (m_rtc_publisher)
		{
			m_rtc_publisher->rtc_texture(texture, fmt, width, height);
			return true;
		}
		return false;
	}
	void crtc_client::send_create_offer_sdp(const std::string & sdp, bool create )
	{
		if (m_websocket_mgr.get_status() != CWEBSOCKET_MESSAGE)
		{
			WARNING_EX_LOG("websocket mgr status = %d !!!", m_websocket_mgr.get_status());
			return ;
		}

		nlohmann::json data = {
			{ "offer", sdp},
			{"roomname", m_room_name},
			{"peerid",  m_user_name}
		};

		nlohmann::json request_data =
		{
			{"msg_id"  ,  1072},
			{"data" , data }
		};
		m_websocket_mgr.send(request_data.dump());
	}

	void crtc_client::connect_rtc_failed()
	{
		m_status = ERtc_Reset;
	}

	void crtc_client::_presssmsg(std::list<std::string> & msgs)
	{
		nlohmann::json response;
		while (!msgs.empty() && m_websocket_mgr.get_status() == CWEBSOCKET_MESSAGE)
		{
			std::string msg = std::move(msgs.front());
			msgs.pop_front();
			NORMAL_EX_LOG("server ====> msg = %s", msg.c_str());

			try
			{
				response = nlohmann::json::parse(msg);
			}
			catch (const std::exception&)
			{
				ERROR_EX_LOG("websocket protoo [msg = %s] json parse failed !!!", msg.c_str());
				continue;
			}


			if (response.find("msg_id") != response.end())
			{
				//NORMAL_EX_LOG("notification --> msg = %s", msg);
				auto result = response.find("result");
				if (result == response.end())
				{
					WARNING_EX_LOG(" result websocket   not find   result  msg = %s", response.dump().c_str());
					continue;
				}
			//	std::string data = response["data"];
				uint32 msg_id = response["msg_id"].get<uint32>();
				if (msg_id == 1073)
				{
					if (m_rtc_publisher)
					{
						m_rtc_publisher->set_remoter_description(response["data"]["sdp"]);
					}
				}
				else if (msg_id == 212)
				{
					if (response.find("data") != response.end())
					{
						 
						 

						s_input_device.OnMessage(response["data"]);
					}
				}
			 

			} 
			else
			{
				ERROR_EX_LOG(" not find msg type !!! msg = %s", msg.c_str());
			}
		}
	}

}