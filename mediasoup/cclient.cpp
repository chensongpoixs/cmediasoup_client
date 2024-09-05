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
#include "cclient.h"
#include "mediasoupclient.hpp"
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
#include "rtc_base/logging.h"
namespace chen {

	///////////////////////////////////////mediasoup///////////////////////////////////////////////////////
	static const char * MEDIASOUP_REQUEST_METHOD_GETROUTERRTPCAPABILITIES = "getRouterRtpCapabilities";
	static const char * MEDIASOUP_REQUEST_METHOD_CREATEWEBRTCTRANSPORT = "createWebRtcTransport";
	static const char * MEDIASOUP_REQUEST_METHOD_CONNECTWEBRTCTRANSPORT = "connectWebRtcTransport";
	static const char * MEDIASOUP_REQUEST_METHOD_PRODUCE = "produce";
	static const char * MEDIASOUP_REQUEST_METHOD_JOIN = "join";
	
	
	////////////////////////////////////////websocket protoo /////////////////////////////////////////////////////////////////
	
	static const char * WEBSOCKET_PROTOO_METHOD = "method";
	static const char * WEBSOCKET_PROTOO_ID = "id";
	static const char * WEBSOCKET_PROTOO_DATA = "data";
	static const char * WEBSOCKET_PROTOO_REQUEST = "request";
	static const char * WEBSOCKET_PROTOO_RESPONSE = "response";
	static const char * WEBSOCKET_PROTOO_NOTIFICATION = "notification";
	static const char * WEBSOCKET_PROTOO_OK = "ok";







	/////////////////////////////////////////webrtc////////////////////////////////////////////////////////////
	static const char * WEBRTC_SERVER = "server";
	static const char * WEBRTC_CLIENT = "client";




	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	static const  int32 WEBRTC_FRAMES = 3;

	static const  int32 OSG_RGBA_WIDTH = 3000;
	static const  int32 OSG_RGBA_HEIGHT = 2000;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define  WEBSOCKET_PROTOO_CHECK_RESPONSE()  if (msg.find(WEBSOCKET_PROTOO_OK) == msg.end())\
	{\
		ERROR_EX_LOG(" [msg = %s] not find 'ok' failed !!!", msg.dump().c_str());\
		return false;\
	}\
	if (msg.find(WEBSOCKET_PROTOO_DATA) == msg.end())\
	{\
		ERROR_EX_LOG("  [msg = %s] not find 'data' failed !!!", msg.dump().c_str());\
		return false;\
	}\
	bool  ok = msg[WEBSOCKET_PROTOO_OK].get<bool>();\
	if (!ok)\
	{\
		ERROR_EX_LOG("  not ok  [msg = %s] !!!", msg.dump().c_str());\
		return false;\
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////

	const uint32_t TICK_TIME = 200;

	cclient::cclient()
		:m_id(100000)
		,m_loaded(false)
		, m_stoped(false)
		, m_status(EMediasoup_None)
		, m_produce_consumer(true)
		, m_ui_type(EUI_None)
		, m_mediasoup_status_callback(nullptr)
		, m_websocket_timer(0)
		, m_send_produce_video_msg(false)
		, m_p2p_connect_failed(0)
		, m_window_capture_wgc_ptr(NULL){}
	cclient::~cclient(){}

	static void show_work_dir()
	{
		WCHAR czFileName[1024] = { 0 };
		 
		GetModuleFileName(NULL, czFileName, _countof(czFileName) - 1);
		//	std::to_string(czFileName);
			 //第一次调用确认转换后单字节字符串的长度，用于开辟空间
		int pSize = WideCharToMultiByte(CP_OEMCP, 0, czFileName, wcslen(czFileName), NULL, 0, NULL, NULL);
		char* pCStrKey = new char[pSize + 1];
		if (!pCStrKey)
		{
			ERROR_EX_LOG("alloc failed !!!");
			return;
		}
		//第二次调用将双字节字符串转换成单字节字符串
		WideCharToMultiByte(CP_OEMCP, 0, czFileName, wcslen(czFileName), pCStrKey, pSize, NULL, NULL);

		//std::string path(czFileName);
		NORMAL_EX_LOG("work path = %s", pCStrKey);
		if (pCStrKey)
		{
			delete[] pCStrKey;
			pCStrKey = NULL;
		}
	}
	
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


	static void rtc_log_file(const char * data, size_t len)
	{
		SYSTEM_LOG("[rtc]%s", data);
	}
	bool cclient::init(uint32 gpu_index)
	{
		
		printf("Log init ...\n");
		if (!LOG::init(ELogStorageScreenFile))
		{
			std::cerr << " log init failed !!!";
			return false;
		}
		 
		
		show_work_dir();
		 
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
		LOG::set_level( static_cast<ELogLevelType>(g_cfg.get_uint32(ECI_LogLevel)));

		SYSTEM_LOG("set level = %u", g_cfg.get_uint32(ECI_LogLevel));
		

		if (g_cfg.get_uint32(ECI_EnableRtcLog))
		{
			//rtc::register_log_callback(&rtc_log_file);
		}
		if (!s_input_device.init())
		{
			ERROR_EX_LOG("init input_device mouble !!!  ");
			return false;
		}
		//一上来就推流
		m_produce_consumer = true;
		SYSTEM_LOG("input device  init ok !!!");
		m_server_protoo_msg_call.insert(std::make_pair("newDataConsumer", &cclient::server_request_new_dataconsumer));
		m_server_notification_protoo_msg_call.insert(std::make_pair("peerClosed", &cclient::_notification_peer_closed));
		SYSTEM_LOG("client init ok !!!");
		m_webrtc_connect = false;
		/*uint32 osg_webrtc_frame = g_cfg.get_uint32(ECI_OsgWebrtcFrame);
		if (osg_webrtc_frame == 0)
		{
			osg_webrtc_frame = WEBRTC_FRAMES;
		}
		SYSTEM_LOG("osg webrtc frames = %u", osg_webrtc_frame);
		m_frame_rgba_vec.reserve(osg_webrtc_frame);
		for (uint32 i = 0; i < osg_webrtc_frame; ++i)
		{

			m_frame_rgba_vec[i].m_rgba_ptr = new unsigned char[OSG_RGBA_WIDTH * OSG_RGBA_HEIGHT * 4];
			if (!m_frame_rgba_vec[i].m_rgba_ptr)
			{
				ERROR_EX_LOG("alloc osg rgab failed !!!");
				return false;
			}
		}
		m_osg_frame = 0;
		m_webrtc_frame = 0;
		m_osg_copy_thread = std::thread(&cclient::_osg_copy_rgba_thread, this);*/
		//m_osg_work_thread = std::thread(&cclient::_osg_thread, this);
		//SYSTEM_LOG("osg video capturer thread ok !!!");
		if (g_cfg.get_int32(ECI_DesktopCapture))
		{
			m_desktop_capture_ptr = DesktopCapture::Create(60, 0);
			SYSTEM_LOG("desktop create fps ok !!!");
		}
		else if (g_cfg.get_int32(ECI_WindowCapture))
		{
			m_window_capture_wgc_ptr = new WindowCapture();
			m_window_capture_wgc_ptr->Init(60, 0);
		}
		else
		{
			m_desktop_capture_ptr = nullptr;
		}
		
		
		mediasoupclient::Initialize();
		return true;
	}
	void cclient::stop()
	{
		SYSTEM_LOG("client sop OK !!!");
		m_stoped = true;
		m_webrtc_connect = false;
	}
	void cclient::Loop(const std::string & mediasoupIp, uint16_t port, const std::string & roomName, const std::string & clientName
	,	uint32_t websocket_reconnect_waittime)
	{
		if (m_desktop_capture_ptr)
		{
			m_desktop_capture_ptr->StartCapture();
		}
		if (m_window_capture_wgc_ptr)
		{
			m_window_capture_wgc_ptr->StartCapture();
		}
		if (g_cfg.get_uint32(ECI_SharedGpuHandleAddressPid))
		{
			m_shared_gpu_handle_address_pthread = std::thread(&cclient::_shared_gpu_handle_address, this);
		}
		// mediasoup_ip, mediasoup_port ;
		// room_name , client_id;
		//  Reconnect_waittime, 
		std::string ws_url = "ws://" + mediasoupIp + ":" + std::to_string(port) + "/?roomId=" + roomName + "&peerId=" + clientName;//ws://127.0.0.1:8888/?roomId=chensong&peerId=xiqhlyrn", "http://127.0.0.1:8888")
		//std::string origin = "http://" + mediasoupIp + ":" + std::to_string(port);
		std::list<std::string> msgs;
		time_t cur_time = ::time(NULL);
		NORMAL_EX_LOG("ws_url = %s", ws_url.c_str());
		m_room_name = roomName;
		m_client_name = clientName;
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration dur;
		std::chrono::milliseconds ms;
		uint32_t elapse = 0;
		while (!m_stoped)
		{
			pre_time = std::chrono::steady_clock::now();
			switch (m_status)
			{
			case EMediasoup_WebSocket_Init: // None
			case EMediasoup_None:
			{  // websocket connect 
				// 1. connect suucer status -> 1
				//g_websocket_mgr.destroy();
				if (!m_websocket_mgr.init(ws_url ))
				{
					//RTC_LOG(LS_ERROR) << "weboscket connect failed !!! url = " << ws_url;
					WARNING_EX_LOG("weboscket connect url = %s failed !!!   ", ws_url.c_str());
					m_status = EMediasoup_Reset;
					++m_websocket_timer;
					if (m_websocket_timer > g_cfg.get_uint32(ECI_WebSocketTimers))
					{
						_mediasoup_status_callback(EMediasoup_WebSocket_Init, 1);
					}
					continue;;
				}
				 
				if (!m_websocket_mgr.startup())
				{
					m_status = EMediasoup_Reset;
					continue;
				}
				if (!_send_router_rtpcapabilities())
				{
					m_status = EMediasoup_Reset;
					continue;
				}
				m_websocket_timer = 0;
				_mediasoup_status_callback(EMediasoup_WebSocket_Init, 0);
				m_status = EMediasoup_WebSocket;
				// 1.1 获取服务器的处理能力
				// 2. connect failed wait 5s..
				break;
			}
			case EMediasoup_Request_Create_Send_Webrtc_Transport: // None
			case EMediasoup_Request_Create_Recv_Webrtc_Transport:
			{
				
				// websocket connect success -> 
				// 1. wait  server 处理能力
				// 1.1 load -> 
				if (!_send_create_webrtc_transport())
				{
					m_status = EMediasoup_Reset;
					continue;
				}
				// 1.2 创建Send WebRTC transport id -> 
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Connect_Webrtc_Transport: //暂时该状态没有使用 TODO@chensong  - 20220215 
			{
				// 1.  wait server call transport dtls info
				// 1.1 Send WebRTC Connect -> 
				m_send_transport->webrtc_connect_transport_offer( );
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Produce_Webrtc_Transport:
			{
				// 1. wait Send WebRTC info ->
				// 1.1 Send WebRTC Producer 
			//	m_send_transport->webrtc_transport_produce();
				m_send_transport->webrtc_connect_transport_setup_connect_server_call();
				_send_produce_webrtc_transport();
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Consumer_Webrtc_Transport:
			{
				m_recv_transport->webrtc_connect_recv_setup_call();
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Send_Connect_Set:
			{
				m_send_transport->webrtc_connect_transport_setup_connect(WEBRTC_SERVER);
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Recv_Connect_Set:
			{
				m_recv_transport->webrtc_connect_transport_setup_connect(WEBRTC_CLIENT);
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_Request_Join_Room: 
			{
				// 1. wait Send WebRTC Producer 
				// 1. 1 Send Join Room 
				_send_join_room();
				m_status = EMediasoup_WebSocket;
				break;
			}
			case EMediasoup_WebSocket: // wait server msg 
			{
				
				m_websocket_mgr.presssmsg(msgs);
				
				
				if (!msgs.empty() && m_websocket_mgr.get_status() == CWEBSOCKET_MESSAGE)
				{
					_presssmsg(msgs);
				}

				if (m_websocket_mgr.get_status() != CWEBSOCKET_MESSAGE)
				{
					m_status = EMediasoup_Reset;
					msgs.clear();
				}
				/*if (m_reconnect_wait && g_cfg.get_int32(ECI_ReconnectWait) > 0)
				{
					if (m_reconnect_wait < ::time(NULL))
					{
						m_status = EMediasoup_Reset;
						m_reconnect_wait = 0;
					}
				}*/
				/*if (m_status == EMediasoup_WebSocket && g_cfg.get_int32(ECI_ProduceVideo) > 0 && m_webrtc_connect)
				{
					if (m_produce_video < ::time(NULL))
					{
						static bool video_produce = true;
						m_produce_video = ::time(NULL) + g_cfg.get_int32(ECI_ProduceVideo);
						if (video_produce)
						{
							m_send_transport->Pause();
							video_produce = false;
						}
						else
						{
							m_send_transport->Resume();
							video_produce = true;
						}
						
					}
				}*/
				break;
			}
			case EMediasoup_WebSocket_Close:
			{
				// 1.  send_transport destroy --> 
				// 2. recv_transport destroy --> 
				// 3. status -> reconnect wait --> 
				m_status = EMediasoup_WebSocket_Wait;
				break;
			}
			case EMediasoup_WebSocket_Wait:
			{
				// 10 sleep 
				// TODO@chensong 20220208 ---> 增加时间
				std::this_thread::sleep_for(std::chrono::milliseconds(websocket_reconnect_waittime));
				m_status = EMediasoup_WebSocket_Init;
				break;
			}
			case EMediasoup_Reset:
			case EMediasoup_Destory:
			{
				// destory all websocket, webrtc destroy --> ok !!
				if (m_recv_transport)
				{
					m_recv_transport->Destroy();
					m_recv_transport = nullptr;
				}
				if (m_send_transport)
				{
					m_send_transport->Destroy();
					m_send_transport = nullptr;
				}
				m_websocket_mgr.destroy();
				_clear_register();
				m_produce_consumer = true;
				m_peer_map.clear();

				// 有人的时候推流 
				//m_produce_consumer = false;
				//m_ui_type = EUI_None;
				cur_time = ::time(NULL) + websocket_reconnect_waittime;
				NORMAL_EX_LOG("reconnect  wait [%u s]  ...", websocket_reconnect_waittime);
				m_status = EMediasoup_Wait;
				break;
			}
			case  EMediasoup_Wait:
			{
				if (cur_time < ::time(NULL))
				{
					m_status = EMediasoup_WebSocket_Init;
				}
				break;
			}
			case EMediasoup_Exit:
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
					std::this_thread::sleep_for(std::chrono::milliseconds(TICK_TIME- elapse));
				}
				
			}
			
			
		}
		NORMAL_EX_LOG("mediasoup Loop exit !!!");
	}
	void cclient::_presssmsg(std::list<std::string> & msgs)
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
			
			 
			if (response.find(WEBSOCKET_PROTOO_NOTIFICATION) != response.end())
			{
				//NORMAL_EX_LOG("notification --> msg = %s", msg);
				auto method_iter = response.find(WEBSOCKET_PROTOO_METHOD);
				if (method_iter == response.end())
				{
					WARNING_EX_LOG("notification websocket protoo not find method name  msg = %s", response.dump().c_str());
					continue;
				}
				std::string method = response[WEBSOCKET_PROTOO_METHOD];
				std::map<std::string, server_protoo_msg>::iterator iter = m_server_notification_protoo_msg_call.find(method);
				if (iter != m_server_notification_protoo_msg_call.end())
				{

					(this->*(iter->second))(response);
					//server_request_new_dataconsumer(response);
				}
				else
				{
					//_default_replay(response);
					//WARNING_EX_LOG("server request client not find method  response = %s", response.dump().c_str());
				}

			}
			else if (response.find(WEBSOCKET_PROTOO_RESPONSE) != response.end())//response
			{
				auto id_iter = response.find(WEBSOCKET_PROTOO_ID);
				if (id_iter == response.end())
				{
					WARNING_EX_LOG("websocket protoo response not find 'id' [response = %s] filed !!! ", response.dump().c_str());
					continue;
				}
				uint64 id = response["id"].get<uint64>();
				std::map<uint64, client_protoo_msg>::const_iterator iter = m_client_protoo_msg_call.find(id);
				if (iter == m_client_protoo_msg_call.end())
				{
					ERROR_EX_LOG("not find id = %u, msg = %s", id, msg.c_str());
				}
				else
				{
					if (!(this->*(iter->second))(response))
					{
						m_status = EMediasoup_Reset;
						return;
					}
					m_client_protoo_msg_call.erase(iter);
				}
			}
			else if (response.find(WEBSOCKET_PROTOO_REQUEST) != response.end())
			{
				//服务器请求客户端响应的请求
				auto method_iter = response.find(WEBSOCKET_PROTOO_METHOD);
				if (method_iter == response.end())
				{
					WARNING_EX_LOG("websocket protoo not find method name  msg = %s", response.dump().c_str());
					continue;
				}
				std::string method = response[WEBSOCKET_PROTOO_METHOD];
				std::map<std::string, server_protoo_msg>::iterator iter =  m_server_protoo_msg_call.find(method);
				if (iter != m_server_protoo_msg_call.end())
				{
					
					(this->*(iter->second))(response);
					//server_request_new_dataconsumer(response);
				}
				else
				{
					_default_replay(response);
					//WARNING_EX_LOG("server request client not find method  response = %s", response.dump().c_str());
				}
			}
			else
			{
				ERROR_EX_LOG(" not find msg type !!! msg = %s", msg.c_str());
			}
		}
	}
	void cclient::startup_ui()
	{
		if (!m_recv_transport)
		{
			WARNING_EX_LOG(" startup ui m_recv_transport == nullptr !!!");
			return;
		}
		if (!m_send_transport)
		{
			WARNING_EX_LOG(" startup ui m_send_transport == nullptr !!!");
			return;
		}
		if (m_ui_type == EUI_Starting || m_ui_type == EUI_StartEnd)
		{
			WARNING_EX_LOG("startup ui ... ui_type = %d", m_ui_type);
			return;
		}
		m_status = EMediasoup_Request_Connect_Webrtc_Transport;
		m_ui_type = EUI_Starting;
		m_produce_consumer = true;
	}
	void cclient::Destory()
	{
		SYSTEM_LOG("mediasoup clinet destroy ...");
		m_mediasoup_status_callback = nullptr;
		if (m_desktop_capture_ptr)
		{
			m_desktop_capture_ptr->StopCapture();
			m_desktop_capture_ptr = nullptr;
		 }
		if (m_window_capture_wgc_ptr)
		{
			m_window_capture_wgc_ptr->StartCapture();
			m_window_capture_wgc_ptr = NULL;
		}
		if (m_shared_gpu_handle_address_pthread.joinable())
		{
			m_shared_gpu_handle_address_pthread.join();
		}
		SYSTEM_LOG("osg copy thread destroy ...");
		if (m_osg_copy_thread.joinable())
		{
			m_osg_copy_thread.join();
		}
		if (m_osg_work_thread.joinable())
		{
			m_osg_work_thread.join();
		}
		SYSTEM_LOG("osg copy thread destroy OK !!!");
		//m_stoped = true;
		if (m_send_transport)
		{
			m_send_transport->Destroy();
			m_send_transport = nullptr;
		}
		SYSTEM_LOG("m_send_transport destroy ok !!!");
		if (m_recv_transport)
		{
			m_recv_transport->Destroy();
			m_recv_transport = nullptr;
		}
		SYSTEM_LOG("m_recv_transport destroy ok !!!");
		for (int32 i = 0; m_frame_rgba_vec.size(); ++i)
		{
			if (m_frame_rgba_vec[i].m_rgba_ptr)
			{
				delete[] m_frame_rgba_vec[i].m_rgba_ptr;
				m_frame_rgba_vec[i].m_rgba_ptr = NULL;
			}
		}
		m_websocket_mgr.destroy();
		SYSTEM_LOG("g_websocket_mgr destroy ok !!!");
		m_peer_map.clear();
		_clear_register();
		m_produce_consumer = true;
		mediasoupclient::Cleanup();
		SYSTEM_LOG("mediasoup destroy ok !!!");
		LOG::destroy();
	  
	}
	bool cclient::_load(nlohmann::json routerRtpCapabilities)
	{
		if (m_loaded)
		{
			return true;
		}
		nlohmann::json nativeRtpCapabilities;
		if (!deivce::GetNativeRtpCapabilities(nativeRtpCapabilities))
		{
			ERROR_EX_LOG("GetNativeRtpCapabilities failed !!!");
		
			return false;
		}
		try {

		
			if (!mediasoupclient::ortc::getExtendedRtpCapabilities(m_extendedRtpCapabilities, nativeRtpCapabilities, routerRtpCapabilities))
			{
				ERROR_EX_LOG("getExtendedRtpCapabilities failed !!!");
				return false;
			}
		
			// Generate our receiving RTP capabilities for receiving media.
			m_recvRtpCapabilities = mediasoupclient::ortc::getRecvRtpCapabilities(m_extendedRtpCapabilities);


			m_sctpCapabilities = deivce::GetNativeSctpCapabilities();
		}
		catch (...)
		{
			ERROR_EX_LOG("_load [routerRtpCapabilities = %s ]failed !!!", routerRtpCapabilities.dump().c_str());
			return false;
		}
		m_loaded = true;
		return true;
	}

	void cclient::transportofferasner(bool send, bool success)
	{
		if (success)
		{
			if (send)
			{
				m_status = EMediasoup_Request_Send_Connect_Set;
			}
			else
			{
				m_status = EMediasoup_Request_Recv_Connect_Set;;
			}
		}
		else
		{
			m_status = EMediasoup_Reset;
			ERROR_EX_LOG("- EMediasoup_Reset --recv send = %d failed !!!", send);
			++m_websocket_timer;
			if (m_websocket_timer > g_cfg.get_uint32(ECI_WebSocketTimers))
			{
				_mediasoup_status_callback(EMediasoup_WebSocket_Init, 1);
			}
		}
	}
	void cclient::webrtc_connect_failed_callback()
	{
		WARNING_EX_LOG("webrtc connect failed callback wait ------> reconnect_times = %d", m_websocket_timer);
		m_status = EMediasoup_Reset;
		++m_p2p_connect_failed;
		//++m_websocket_timer;
		if (m_p2p_connect_failed > g_cfg.get_uint32(ECI_WebSocketTimers))
		{
			_mediasoup_status_callback(EMediasoup_WebSocket_Init, 1);
		}
		

	}
	bool cclient::_send_router_rtpcapabilities()
	{
		if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_GETROUTERRTPCAPABILITIES, nlohmann::json::object()))
		{
			m_status = EMediasoup_Reset;
			WARNING_EX_LOG("send request router rtcpcapabilities failed !!!");
			return false;
		}
	
		m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_RouterRtpCapabilities));
		

		return true;
	}


	bool cclient::_send_create_webrtc_transport()
	{
		
		
		if (EMediasoup_Request_Create_Send_Webrtc_Transport == m_status)
		{
			nlohmann::json data =
			{
				{"forceTcp",false},
			{"producing",true},
			{"consuming" , false},
			{	"sctpCapabilities", m_sctpCapabilities }

			};
			if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_CREATEWEBRTCTRANSPORT, data))
			{
				WARNING_EX_LOG("send request create send webrtc transport  failed !!!");
				return false;
			}
			 
		}
		else
		{
			nlohmann::json data =
			{
				{"forceTcp",false},
				{"producing",false},
				{"consuming" , true},
				{	"sctpCapabilities", m_sctpCapabilities }

			};

			if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_CREATEWEBRTCTRANSPORT, data))
			{
				WARNING_EX_LOG("send request create recv webrtc transport  failed !!!");
				return false;
			}
			 
		}
		
		m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_create_webrtc_transport));
		return true;
	}
	bool cclient::_send_connect_webrtc_transport(nlohmann::json dtlsparameters)
	{
		std::string str = dtlsparameters["role"];
		NORMAL_EX_LOG("connect webrtc -> role = %s", str.c_str());
		std::string transportId;
		if (str == WEBRTC_SERVER)
		{
			transportId = m_send_transport->get_transportId();
		}
		else
		{
			transportId = m_recv_transport->get_transportId();
		}
		nlohmann::json data = 
		{
			{"transportId", transportId},
		{"dtlsParameters", dtlsparameters}
		};
		 
	 
		
		if (str == WEBRTC_SERVER)
		{
			if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_CONNECTWEBRTCTRANSPORT, data))
			{
				m_status = EMediasoup_Reset;
				WARNING_EX_LOG("send request connect recv webrtc transport  failed !!!");
				return false;
			}
			m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_connect_webrtc_transport));
		}
		else
		{
			if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_CONNECTWEBRTCTRANSPORT, data))
			{
				m_status = EMediasoup_Reset;
				WARNING_EX_LOG("send request connect recv webrtc transport  failed !!!");
				return false;
			}
			m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_recv_connect_webrtc_transport));
		}
		
		//m_status = EMediasoup_WebSocket;
		return true;
	}

	bool cclient::_send_produce_webrtc_transport()
	{
		nlohmann::json data = nlohmann::json::object();
		
		
		std::string transportId = m_send_transport->get_transportId();
		std::string kind = m_send_transport->get_kind();
		nlohmann::json rtp = m_send_transport->get_sending_rtpParameters();
		data =
		{
			{"transportId", transportId},
			{"kind", kind},
			{"rtpParameters", rtp},
			{"appData", nlohmann::json::object()}
		};
		NORMAL_EX_LOG("data = %s", data.dump().c_str());
		 
		if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_PRODUCE, data))
		{
			m_status = EMediasoup_Reset;
			WARNING_EX_LOG("send request connect recv webrtc transport  failed !!!");
			return false;
		}
		 
		 
		m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_produce_webrtc_transport));
		//m_status = EMediasoup_WebSocket;
		return true;
	}
	bool cclient::_send_join_room()
	{
		nlohmann::json device = 
		{
			{"flag","native"},
		{"name" , "win"},
		{"version" , "1.0"}
		};
		nlohmann::json rtpParameters =
		{
			{"displayName", m_client_name},
		{"device", device},
		{"rtpCapabilities", m_recvRtpCapabilities},
		{"sctpCapabilities", m_sctpCapabilities.dump()}
		};

		if (!_send_request_mediasoup(MEDIASOUP_REQUEST_METHOD_JOIN, rtpParameters))
		{
			m_status = EMediasoup_Reset;
			WARNING_EX_LOG("send request join room   failed !!!");
			return false;
		}
		 
		m_client_protoo_msg_call.insert(std::make_pair(m_id, &cclient::_server_join_room));
		//m_status = EMediasoup_WebSocket;
		return true;
	}
	bool cclient::webrtc_video(unsigned char * rgba, uint32 fmt, int32_t width, int32_t height)
	{
		if (!m_webrtc_connect)
		{
			//WARNING_EX_LOG("not connect webrtc video wait !!!");
			return false;
		}
		if (!m_send_transport)
		{
			//WARNING_EX_LOG("m_send_transport == nullptr !!!");
			return false;
		}

		if (!rgba || width <= 0 || height <= 0)
		{//
			//WARNING_EX_LOG(" osg copy param ? !!!");
			return false;
		}

	/*	memcpy(m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_rgba_ptr, rgba, (width * height * 4));
		m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_height = height;
		m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_width = width;

		++m_osg_frame;*/
		if (!m_send_produce_video_msg)
		{
			m_send_produce_video_msg = true;
			_mediasoup_status_callback(EMediasoup_Request_Produce_Webrtc_Transport, 0);
		}
		return m_send_transport->webrtc_video(rgba, fmt, width, height);
		 
		return true;
	}
	bool cclient::webrtc_texture(void * texture, uint32 fmt, int32_t width, int32_t height)
	{
		//NORMAL_EX_LOG("[fmt = %u][width = %u][height = %u]", fmt, width, height);
		if (!m_webrtc_connect)
		{
			//WARNING_EX_LOG("not connect webrtc video wait !!!");
			return false;
		}
		if (!m_send_transport)
		{
			//WARNING_EX_LOG("m_send_transport == nullptr !!!");
			return false;
		}

		if (!texture || width <= 0 || height <= 0)
		{//
			//WARNING_EX_LOG(" osg copy param ? !!!");
			return false;
		}

		/*	memcpy(m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_rgba_ptr, rgba, (width * height * 4));
			m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_height = height;
			m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_width = width;

			++m_osg_frame;*/
		if (!m_send_produce_video_msg)
		{
			m_send_produce_video_msg = true;
			_mediasoup_status_callback(EMediasoup_Request_Produce_Webrtc_Transport, 0);
		}
		return m_send_transport->webrtc_texture(texture, fmt, width, height);
	}
	bool cclient::webrtc_video(unsigned char * y_ptr, unsigned char * uv_ptr, uint32 fmt, int32_t width, int32_t height)
	{
		if (!m_webrtc_connect)
		{
			WARNING_EX_LOG("not connect webrtc video wait !!!");
			return false;
		}
		if (!m_send_transport)
		{
			WARNING_EX_LOG("m_send_transport == nullptr !!!");
			return false;
		}

		if (!y_ptr || width <= 0 || height <= 0)
		{//
			WARNING_EX_LOG(" osg copy param ? !!!");
			return false;
		}

		/*	memcpy(m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_rgba_ptr, rgba, (width * height * 4));
			m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_height = height;
			m_frame_rgba_vec[m_osg_frame % m_frame_rgba_vec.size()].m_width = width;

			++m_osg_frame;*/
		NORMAL_EX_LOG("");
		if (!m_send_produce_video_msg)
		{
			m_send_produce_video_msg = true;
			_mediasoup_status_callback(EMediasoup_Request_Produce_Webrtc_Transport, 0);
		}
		return m_send_transport->webrtc_video(y_ptr, uv_ptr, fmt, width, height);

		return true;
	}
	bool cclient::webrtc_video(const webrtc::VideoFrame& frame)
	{
		if (!m_webrtc_connect)
		{
			//WARNING_EX_LOG("not connect webrtc video wait !!!");
			return false;
		}
		if (!m_send_transport)
		{
			//WARNING_EX_LOG("m_send_transport == nullptr !!!");
			return false;
		}

		if (!m_send_produce_video_msg)
		{
			m_send_produce_video_msg = true;
			_mediasoup_status_callback(EMediasoup_Request_Produce_Webrtc_Transport, 0);
		}
		return m_send_transport->webrtc_video(frame);
	}
	bool cclient::webrtc_run()
	{
		if (!m_webrtc_connect)
		{
			WARNING_EX_LOG("not connect webrtc video wait !!!");
			return false;
		}
		return true;
	}
	bool cclient::_server_RouterRtpCapabilities(const  nlohmann::json & msg)
	{
		WEBSOCKET_PROTOO_CHECK_RESPONSE();
		
		nlohmann::json data_rtp = msg[WEBSOCKET_PROTOO_DATA];
		if (!mediasoupclient:: ortc::validateRtpCapabilities(data_rtp))
		{
			WARNING_EX_LOG("RouterRtpCapabilities validate rtp capabilities [data = %s] failed !!!", data_rtp.dump().c_str());
			m_status = EMediasoup_Reset;
			return false;
		}
		if (!_load(data_rtp))
		{
			WARNING_EX_LOG("RouterRtpCapabilities _load[data_rtp = %s] failed !!!", data_rtp.dump().c_str());
			m_status = EMediasoup_Reset;
			return false;
		}
		//m_status = EMediasoup_Request_Create_Recv_Webrtc_Transport;
		m_status = EMediasoup_Request_Create_Send_Webrtc_Transport; // 
		return true;
	}

	bool cclient::_server_create_webrtc_transport(const  nlohmann::json & msg)
	{
		//NORMAL_EX_LOG(" server create webrtc --> ");
		WEBSOCKET_PROTOO_CHECK_RESPONSE();
		  
		nlohmann::json new_data = msg[WEBSOCKET_PROTOO_DATA];
		if (new_data.find("id") == new_data.end())
		{
			ERROR_EX_LOG("_server_create_webrtc_transport [msg = %s] not find 'data.id' failed !!!", msg.dump().c_str());

			return false;
		}
		auto iceParameters = new_data.find("iceParameters");
		auto iceCandidates = new_data.find("iceCandidates");
		auto dtlsParameters = new_data.find("dtlsParameters");
		auto sctpParameters = new_data.find("sctpParameters");

		if (iceParameters == new_data.end())
		{
			ERROR_EX_LOG("_server_create_webrtc_transport [msg = %s] not find 'data.iceParameters' failed !!!", msg.dump().c_str());

			return false;
		}
		std::string  transport_id = new_data["id"];
		
		nlohmann::json json_iceParameters = iceParameters.value();
		nlohmann::json json_iceCandidates = iceCandidates.value();
		nlohmann::json json_dtlsParameters = dtlsParameters.value();
		nlohmann::json json_sctpParameters = sctpParameters.value();

			//NORMAL_EX_LOG("data.value().dump().c_str() = %s\n json_iceParameters = %s\n dtlsParameters = %s", new_data.dump().c_str(), json_iceParameters.dump().c_str(), dtlsParameters.value().dump().c_str());
	 
		if (!m_send_transport)
		{
			m_send_transport = new rtc::RefCountedObject<csend_transport>(transport_id, this);
			m_send_transport->init(transport_id, m_extendedRtpCapabilities,
				json_iceParameters,
				json_iceCandidates,
				json_dtlsParameters,
				json_sctpParameters);
			m_status = EMediasoup_Request_Create_Recv_Webrtc_Transport;
		}
		else if (!m_recv_transport)
		{

			m_recv_transport = new rtc::RefCountedObject<crecv_transport>(transport_id, this);
			m_recv_transport->init(transport_id, m_extendedRtpCapabilities,
				json_iceParameters,
				json_iceCandidates,
				json_dtlsParameters,
				json_sctpParameters);
			m_status = EMediasoup_Request_Join_Room;
		}
		else
		{
			ERROR_EX_LOG(" not find  transport id = %s", transport_id.c_str());
		}
		
		
				 
		
		 
		return true;
	}
	bool cclient::_server_connect_webrtc_transport(const  nlohmann::json & msg)
	{
		WEBSOCKET_PROTOO_CHECK_RESPONSE();
		m_status = EMediasoup_Request_Produce_Webrtc_Transport  ;
		return true;
	}
	bool cclient::_server_recv_connect_webrtc_transport(const  nlohmann::json & msg)
	{
		WEBSOCKET_PROTOO_CHECK_RESPONSE();
		m_status = EMediasoup_Request_Consumer_Webrtc_Transport;
		return true;
	}
	bool cclient::_server_produce_webrtc_transport(const  nlohmann::json & msg)
	{
		WEBSOCKET_PROTOO_CHECK_RESPONSE();

		nlohmann::json data = msg[WEBSOCKET_PROTOO_DATA];
		if (data.find("id") == data.end())
		{
			ERROR_EX_LOG("[msg = %s] not find  data.id failed !!!", msg.dump().c_str());
			return false;
		}
		std::string produce_id = data["id"]; 
		m_send_transport->webrtc_transport_produce(produce_id);
		
		m_status = EMediasoup_WebSocket;
		m_produce_consumer = false;
		m_recv_transport->webrtc_create_all_wait_consumer();
		m_produce_video = ::time(NULL) +8;
		m_webrtc_connect = true;
		//_mediasoup_status_callback(EMediasoup_Request_Produce_Webrtc_Transport, 0);
 		return true;
	}

	bool cclient::_server_join_room(const  nlohmann::json & msg)
	{
		WEBSOCKET_PROTOO_CHECK_RESPONSE();
		m_send_transport->webrtc_connect_transport_offer( );
		//m_produce_consumer = false;
		/*if (g_cfg.get_int32(ECI_ReconnectWait) > 0)
		{
			m_reconnect_wait = ::time(NULL) + g_cfg.get_int32(ECI_ReconnectWait);
		}*/
		//m_status = EMediasoup_WebSocket; 
		_mediasoup_status_callback(EMediasoup_Request_Join_Room, 0);
		return true;
	}



	bool cclient::server_request_new_dataconsumer(const  nlohmann::json & msg)
	{
		auto data_iter = msg.find(WEBSOCKET_PROTOO_DATA);
		if (data_iter == msg.end())
		{
			WARNING_EX_LOG("not find 'data'  ");
			return false;
		}
		auto id_iter = data_iter.value().find(WEBSOCKET_PROTOO_ID);
		if (id_iter ==  data_iter.value().end())
		{
			WARNING_EX_LOG("not find   'id' ");
			return false;
		}
		std::string transport_id =  msg["data"]["id"];
		uint64 id = msg["id"].get<uint64>();
	
		/*if (transport_id == m_send_transport->get_transportId())
		{
			WARNING_EX_LOG("transport_id = %s ", transport_id.c_str());
			server_reply_new_dataconsumer(id);
			return false;
		}*/

		nlohmann::json data = msg[WEBSOCKET_PROTOO_DATA];
		std::string peerId;
		auto peerIditer = data.find("peerId");
		if (peerIditer != data.end() && !peerIditer.value().is_null())
		{
			peerId = data["peerId"].get<std::string>();
			m_peer_map.insert(peerId);
		}
		
		std::string dataProducerId = msg["data"]["dataProducerId"];
		if (!peerId.empty())
		{
			if (!m_peerid_dataconsumer.insert(std::make_pair(peerId, transport_id)).second)
			{
				WARNING_EX_LOG("insert peerId = data failed !!!", peerId.c_str());
			}
		}
		uint16_t streamId = msg["data"]["sctpStreamParameters"]["streamId"].get<uint16_t>();
		if (m_produce_consumer)
		{
			m_recv_transport->webrtc_consumer_wait(transport_id, dataProducerId, std::to_string(streamId));
		}
		else
		{
			m_recv_transport->webrtc_create_consumer(transport_id, dataProducerId, std::to_string(streamId));
		}
		server_reply_new_dataconsumer(id);
		return true;
	}

	bool cclient::_notification_peer_closed(const nlohmann::json & msg)
	{

		auto data_iter = msg.find(WEBSOCKET_PROTOO_DATA);
		if (data_iter == msg.end())
		{
			WARNING_EX_LOG("not find 'data'  ");
			return false;
		}
		auto id_iter = data_iter.value().find("peerId");
		if (id_iter == data_iter.value().end())
		{
			WARNING_EX_LOG("not find   'peerId' ");
			return false;
		}
		std::string peerId = msg[WEBSOCKET_PROTOO_DATA]["peerId"];
		 
		
		m_peer_map.erase(peerId);
		NORMAL_EX_LOG("peerId = %s exit ", peerId.c_str());
		/*std::map < std::string, std::string>::iterator iter =  m_peerid_dataconsumer.find(peerId);
		if (iter != m_peerid_dataconsumer.end())
		{
			m_recv_transport->close_dataconsumer(iter->second);
			m_peerid_dataconsumer.erase(iter->first);
		}
		else
		{
			WARNING_EX_LOG("not find peerId = %s failed !!!", peerId.c_str());
		}*/
		return true;
	}
	bool cclient::server_reply_new_dataconsumer(uint64 id)
	{
		 
		if (!_reply_server(id))
		{
			WARNING_EX_LOG("reply new dataconsumer failed !!!");
		}
		return true;
	}
	bool cclient:: async_produce()
	{
		//m_send_transport->webrtc_connect_transport_offer(nullptr);
		//m_async_data_consumer_t = ::time(NULL) + 50;
		return true;
	}



	bool cclient::_send_request_mediasoup(  const std::string& method, const nlohmann::json & data)
	{
		if (m_websocket_mgr.get_status() != CWEBSOCKET_MESSAGE)
		{
			WARNING_EX_LOG("websocket mgr status = %d !!!", m_websocket_mgr.get_status());
			return false;
		}
		nlohmann::json request_data =
		{
			{WEBSOCKET_PROTOO_REQUEST  ,true},
			{WEBSOCKET_PROTOO_ID , ++m_id}, //  
			{WEBSOCKET_PROTOO_METHOD , method}, //方法
			{WEBSOCKET_PROTOO_DATA , data}
		};
		m_websocket_mgr.send(request_data.dump());
		return true;
	}


	bool cclient::_reply_server(uint64 id)
	{
		if (m_websocket_mgr.get_status() != CWEBSOCKET_MESSAGE)
		{
			WARNING_EX_LOG("websocket mgr status = %d !!!", m_websocket_mgr.get_status());
			return false;
		}
		nlohmann::json  reply =
		{
			{WEBSOCKET_PROTOO_RESPONSE,true},
			{WEBSOCKET_PROTOO_ID,id},
			{WEBSOCKET_PROTOO_OK , true},
			{WEBSOCKET_PROTOO_DATA, nlohmann::json::object()}
		};
		m_websocket_mgr.send(reply.dump());
		return true;
	}
	void cclient::_clear_register()
	{
		m_client_protoo_msg_call.clear();
	}

	void cclient::_osg_thread()
	{
		 
	}
	void cclient::_osg_copy_rgba_thread()
	{
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time  ;
		std::chrono::steady_clock::duration dur;
		std::chrono::milliseconds ms;
		uint32_t elapse = 0;
		
			
	/*	while (!m_stoped)
		{
			pre_time = std::chrono::steady_clock::now();
			if  (m_osg_frame > m_webrtc_frame)
			{
				if ((!m_webrtc_connect || !m_send_transport) &&  (m_osg_frame - m_webrtc_frame > 5))
				{
					m_osg_frame = 0;
					m_webrtc_frame = 0;
				}
				else
				{ 
						m_send_transport->webrtc_video(m_frame_rgba_vec[m_webrtc_frame % m_frame_rgba_vec.size()].m_rgba_ptr, m_frame_rgba_vec[m_webrtc_frame % m_frame_rgba_vec.size()].m_width, m_frame_rgba_vec[m_webrtc_frame % m_frame_rgba_vec.size()].m_height);

						++m_webrtc_frame;
				}
			}
			else
			{
				m_osg_frame = 0;
				m_webrtc_frame = 0;
			}
			if (m_osg_frame > m_webrtc_frame)
			{
				cur_time_ms = std::chrono::steady_clock::now();
				dur = cur_time_ms - pre_time;
				ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
				elapse = static_cast<uint32_t>(ms.count());
				if (elapse < 50)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(50 - elapse));
				}
			}
			 
		}*/
	}
	void cclient::_shared_gpu_handle_address()
	{

		struct d3d8_offsets {
			uint32_t present;
		};

		struct d3d9_offsets {
			uint32_t present;
			uint32_t present_ex;
			uint32_t present_swap;
			uint32_t d3d9_clsoff;
			uint32_t is_d3d9ex_clsoff;
		};

		struct d3d12_offsets {
			uint32_t execute_command_lists;
		};

		struct dxgi_offsets {
			uint32_t present;
			uint32_t resize;

			uint32_t present1;
		};

		struct dxgi_offsets2 {
			uint32_t release;
		};

		struct ddraw_offsets {
			uint32_t surface_create;
			uint32_t surface_restore;
			uint32_t surface_release;
			uint32_t surface_unlock;
			uint32_t surface_blt;
			uint32_t surface_flip;
			uint32_t surface_set_palette;
			uint32_t palette_set_entries;
		};

		struct shmem_data {
			volatile int last_tex;
			uint32_t tex1_offset;
			uint32_t tex2_offset;
		};

		struct shtex_data {
			uint32_t tex_handle;
		};

		enum capture_type {
			CAPTURE_TYPE_MEMORY,
			CAPTURE_TYPE_TEXTURE,
		};

		struct graphics_offsets {
			struct d3d8_offsets d3d8;
			struct d3d9_offsets d3d9;
			struct dxgi_offsets dxgi;
			struct ddraw_offsets ddraw;
			struct dxgi_offsets2 dxgi2;
			struct d3d12_offsets d3d12;
		};

		struct hook_info {
			/* hook version */
			uint32_t hook_ver_major;
			uint32_t hook_ver_minor;

			/* capture info */
			enum capture_type type;
			uint32_t window;
			uint32_t format;
			uint32_t cx;
			uint32_t cy;
			uint32_t UNUSED_base_cx;
			uint32_t UNUSED_base_cy;
			uint32_t pitch;
			uint32_t map_id;
			uint32_t map_size;
			bool flip;

			/* additional options */
			uint64_t frame_interval;
			bool UNUSED_use_scale;
			bool force_shmem;
			bool capture_overlay;
			bool allow_srgb_alias;

			/* hook addresses */
			struct graphics_offsets offsets;

			uint32_t reserved[126];
		};



		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time;
		std::chrono::steady_clock::duration dur;
		std::chrono::microseconds ms;
		uint32_t elapse = 0;
		static const uint32 frame_s = (1000 ) / 200;

		auto timestamp =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch())
			.count();
		size_t cnt = 0;

		rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer_;


#define SHMEM_HOOK_INFO L"CaptureHook_HookInfo"
#define SHMEM_TEXTURE L"CaptureHook_Texture"
#define GC_MAPPING_FLAGS (FILE_MAP_READ | FILE_MAP_WRITE)
		wchar_t new_name[64] = L"CaptureHook_Texture_12918194_7";
		//swprintf(new_name, 64, L"%s%lu", SHMEM_TEXTURE, g_cfg.get_uint32(ECI_SharedGpuHandleAddressPid));

		//debug("map id: %S", new_name);

		 
		HANDLE shader_ptr  =   OpenFileMappingW(GC_MAPPING_FLAGS, false, new_name);

		void * global_hook_info_ptr = NULL;

		global_hook_info_ptr =(struct hook_info*) MapViewOfFile(shader_ptr,
			FILE_MAP_ALL_ACCESS, 0, 0,
			4);
		i420_buffer_ = webrtc::I420Buffer::Create(1920, 1040);
		while (true)
		{
			pre_time = std::chrono::steady_clock::now();
			//shtex_data* ddd_ptr = (shtex_data *)global_hook_info_ptr;
			if (true)
			{
				// webgpu Rte2.exe 932b9c2e0250b666_C2 eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJhdWQiOiJhZG1pbiJ9.uPchyky8YSNmaJjxTHPIM_CnS9X8hcjzLp9H55aohag 192.168.1.175 192.168.1.244 8500 9871 6379 syz@8015 192.168.2.52  9905 0 0 1920 1040 1 1 192.168.0.191 8300 192.168.0.191 8301 1 1 192.168.1.244 9871
				// http://localhost:8080/getHandle
				uint64 address = 0XC0003942;
				//ShowWindow(m_window_wnd, SW_HIDE);
				i420_buffer_->set_texture((void*)address);
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
				ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
				elapse = static_cast<uint32_t>(ms.count());

				if (elapse < frame_s)
				{

					std::this_thread::sleep_for(std::chrono::milliseconds(frame_s - elapse));
				}
				else
				{
					NORMAL_EX_LOG("send frame  = %u msc [frame_s = %u]", elapse, frame_s);
				}
			}


		}
	}
	void cclient::_mediasoup_status_callback(uint32 status, uint32 error)
	{
		NORMAL_EX_LOG("status = %u, error = %u", status, error);
		if (m_mediasoup_status_callback)
		{

			m_mediasoup_status_callback(status, error);
		}
		else
		{
			WARNING_EX_LOG("not find mediasoup status callback !!!");
		}
	}
	bool cclient::_default_replay(const nlohmann::json & reply)
	{
		auto iter =  reply.find(WEBSOCKET_PROTOO_ID);
		if (iter != reply.end())
		{
			uint64 id = reply[WEBSOCKET_PROTOO_ID].get<uint64>();
			if (!_reply_server(id))
			{
				WARNING_EX_LOG("reply send failed  msg = %s", reply.dump().c_str());
				return false;
			}
			return true;
		}
		WARNING_EX_LOG("reply not find 'id' failed  msg = %s", reply.dump().c_str());
		return false;
	}
}