/***********************************************************************************************
				created: 		2019-05-01

				author:			chensong

				purpose:		ccfg
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
#include "ccfg.h"
#include "clog.h"
namespace chen {
	ccfg g_cfg;
	
	ccfg::ccfg()
	{
	}


	ccfg::~ccfg()
	{
	}
	bool ccfg::init(const char * file_name)
	{
		if (!cconfig::init(ECI_Max, file_name))
		{
			return false;
		}
		// é»˜è®¤å€¼
		set_string(ECI_MediaSoup_Host, "mediasoup_ip", "127.0.0.1");
		set_uint32(ECI_MediaSoup_Http_Port, "mediasoup_http_port", 8888);
		set_uint32(ECI_MediaSoup_WebSocket_Port, "mediasoup_websocket_port", 8888);
		set_string(ECI_Room_Name, "room_name", "chensong");
		set_string(ECI_Client_Name, "client_name", "test");
		set_uint32(ECI_Video_Fps, "video_fps", 30);
		set_uint32(ECI_WebSocket_Reconnect, "reconnect_timer", 10);
		set_int32(ECI_ReconnectWait, "reconnect_wait", 0);
		set_int32(ECI_ProduceVideo, "produce_video", 0);
		set_uint32(ECI_OsgWebrtcFrame, "osg_webrtc_frame", 3);
		set_uint32(ECI_DesktopCapture, "desktop_capture", 0);
		set_uint32(ECI_WindowCapture, "window_capture", 0);
		set_uint32(ECI_WebSocketTimers, "websocket_timer", 3);
		set_uint32(ECI_RtcAvgRate, "rtc_avg_rate", 8000);
		set_uint32(ECI_RtcMaxRate, "rtc_max_rate", 12000);
		set_uint32(ECI_RtcFrames, "rtc_frame", 50);
		
		set_uint32(ECI_EncoderArraySize, "encoder_array_size", 3);

		set_uint32(ECI_LogLevel, "log_level", ELogLevel_Info);

		set_uint32(ECI_EncoderPreset, "nvenc_preset", 5);
		set_uint32(ECI_EncoderPFrameCount, "nvenc_pframe_count", 10);
		set_uint32(ECI_EncoderVideoGop, "nvenc_video_gop", 180);
		set_uint32(ECI_EncoderLowLatency, "nvenc_low_latency", 3);
		set_uint32(ECI_GpuVideoLock, "gpu_video_lock", 1);
		set_uint32(ECI_EnableEncoderCbr, "enable_encoder_cbr", 1);
		set_uint32(ECI_EnableRtcLog, "enable_rtc_log", 0);
		set_uint32(ECI_EncodeQpMin, "encode_qp_min", 0);
		set_uint32(ECI_EncodeQpMax, "encode_qp_max", 51);
		set_uint32(ECI_SharedGpuHandleAddressPid, "shard_gpu_handle_address_pid", 0);
		set_uint32(ECI_BrowserAppIdleTime, "browser_app_idle_time", 60 * 3);
#if _DEBUG
		set_uint32(ECI_LogLevel, "log_level", ELogLevel_Num);
#endif // _DEBUG

		
		/*set_int32(ECI_Time_Adjust, "time_adjust", 0);
		set_string(ECI_Log_DB_Host, "log_db_ip", "127.0.0.1");
		set_int32(ECI_Log_DB_Port, "log_db_port", 3306);
		set_string(ECI_Log_DB_User, "log_db_user", "root");
		set_string(ECI_Log_DB_Passwd, "log_db_passwd", "20190813");
		set_string(ECI_Log_DB_Name, "log_db_name", "global_log");
		set_string(ECI_ResPath, "res_path", "../res/");*/
		//set_uint32(ECI_LogPoolSize, "log_pool_size", 10);
		//é…ç½®è¡¨çš„å€¼
		load_cfg_file();

		return true;
	}
	void ccfg::destroy()
	{
		cconfig::destroy();
	}
}//namespace chen 
