/***********************************************************************************************
						created: 		2018-10-02

						author:			chensong

						purpose:		async_log
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

#include "casync_log.h"
//#include "ctime_api.h"
#include <cassert>
#include <thread>
#include <iostream>
#include <mutex>
#include "clog.h"
#include <sstream>
#include <string>
#include "ctime_api.h"
//#include "httplib.h"
#define LOG_WRITE_FILE (0)
#ifdef _MSC_VER
#include <io.h>
#include <direct.h>
#elif defined(__linux__) ||defined(__APPLE__)
#include <unistd.h>
#include <dirent.h>
#else
// 其他不支持的编译器需要自己实现这个方法
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
#endif
#include <vector>
#include "cutil.h"
#include <boost/filesystem.hpp>


namespace chen {

#pragma pack(push,1)
	struct clog_item
	{
		int				timestamp;
		signed char		level;
		std::string		data;
	};
#pragma pack(pop)
//	static const unsigned int LOG_ITEM_SIZE = sizeof(clog_item);
	static const unsigned int LOG_BUF_MAX_SIZE = 1024 * 50; // default 1024 * 1024 statck win too small

	enum ELogColorType
	{
		ELCT_White = 0,
		ELCT_Red,
		ELCT_Pink,
		ELCT_Yellow,
		ELCT_Blue,
		ELCT_Green,
		ELCT_Max
	};
	struct clog_name_color
	{
		const char* name;
		ELogColorType color;
	}
	static const g_log_name_colors[] =
	{
		{ "",		ELCT_White },
		{ "system", ELCT_Blue },
		{ "fatal",  ELCT_Red },
		{ "error",  ELCT_Pink },
		{ "warn",	ELCT_Yellow },
		{ "info",	ELCT_White },
		{ "debug",	ELCT_Green },
	};

	static void gen_log_file_name(char* p, const std::string& path, const std::string& name
		, const std::string& ext, ELogNameType name_type, int& day_stamp)
	{
		p += sprintf(p, "%s", path.c_str());
		char dateTime[ASCII_DATETIME_LEN] = { 0 };
		if (name_type == ELogName_Date)
		{
			ctime_base_api::datetime_format(dateTime, 0, -1, -1);
			 
		}
		else if (name_type == ELogName_DateTime)
		{
			ctime_base_api::datetime_format(dateTime, 0, 0, 0);
			 
		}
		else if (name_type == ELogName_AutoDate)
		{
			if (day_stamp == 0)
			{
				day_stamp = ctime_base_api::get_today_stamp_time64(0);
			}
			ctime_base_api::datetime_format(dateTime, 0, 0, 0);
			 
			day_stamp += ETC_Day;
		}
		sprintf(p, "%s_%s%s", dateTime, name.c_str(), ext.c_str());
	}
	 
	casync_log::casync_log()
		: m_host("127.0.0.1")
		, m_port(80)  
		, m_level_log(ELogLevel_Num)
		, m_storage_type(ELogStorageScreenFilePost)
		, m_stoped(false)
		, m_date_time(0)
		, m_path("./log")
		, m_expired_log_day(3)
	{ 
	}

	casync_log::~casync_log()
	{
		
	}
	bool casync_log::init(ELogStorageType storagetype, const   std::string  & host, uint32 port, uint32 day)
	{ 
		//NORMAL_EX_LOG("host = %s, port = %d, show_screen = %d", host.c_str(), port, show_screen);


#ifdef _MSC_VER
		if (::_access(m_path.c_str(), 0) == -1)
		{

			::_mkdir(m_path.c_str());
		}
#elif defined(__GNUC__) ||defined(__APPLE__)
		if (::access(m_path.c_str(), 0) == -1)
		{
			::mkdir(m_path.c_str(), 0777);
		}
#else
		// 其他不支持的编译器需要自己实现这个方法
#error unexpected c complier (msc/gcc), Need to implement this method for demangle
		return false;
#endif
		if (storagetype & ELogStorageFile)
		{
			char log_name[1024] = {0};
			gen_log_file_name(log_name, m_path + "/", "rtc_native", ".log", ELogName_AutoDate, m_date_time);
			m_fd.open(log_name, std::ios::out | std::ios::trunc);
			if (!m_fd.is_open())
			{
				std::cout << "not open log file dest url = " << log_name << std::endl;
				return false;
			}
		}
		m_host = host;
		m_port = port;
		m_storage_type = storagetype;
		m_expired_log_day = day > 0 ? day : 1;
		std::thread td(&casync_log::_work_pthread, this);
		m_thread.swap(td);
		//std::move(m_thread, td);
		return true;
	}
	
	void casync_log::append_var(ELogLevelType level, const char * format, va_list ap)
	{
		
		if (level < ELogLevel_None || level > ELogLevel_Num  || level >= m_level_log)
		{
			return;
		}

		if (m_stoped)
		{
			return;
		}
		 
		int32 cnt = -1;
		char buffer[LOG_BUF_MAX_SIZE] = {0};
		cnt = vsnprintf(buffer, LOG_BUF_MAX_SIZE, format, ap);
		if (cnt <= 0)
		{ 
			return;
		}
		{
			
			std::shared_ptr<clog_item>  log_item_ptr = _get_new_buf();
			if (!log_item_ptr)
			{
				return;
			}
			{
				std::lock_guard<std::mutex> lock{ m_lock };

				//得到一个新的log

				log_item_ptr->level = level;
				log_item_ptr->timestamp = static_cast<int>(ctime_base_api::get_gmt());
				log_item_ptr->data = buffer;
				m_log_item.push_back(log_item_ptr);
			}
		}
		
		{
			m_condition.notify_one();
		}
	}
	void casync_log::set_level(ELogLevelType level)
	{
		m_level_log = level;
	}
	bool casync_log::_init_log()
	{
		return true;
	}
	void casync_log::_work_pthread()
	{
		std::shared_ptr<clog_item>  log_item_ptr;
		std::list<std::shared_ptr<clog_item>> log_items;
		while (!m_stoped  )
		{
			//当又有数据就立即写入日志文件中
			if (m_log_item.size() <= 0)
			{
				std::unique_lock<std::mutex> lock(m_lock);
				//有数据或者停止才可以写入日志中
				m_condition.wait(lock, [this]() {return (m_log_item.size() > 0 || m_stoped); });
			}
			if (m_log_item.empty())
            {
                continue;
            }
            {
                std::lock_guard<std::mutex> lock{m_lock};
                log_items = m_log_item;
                m_log_item.clear();
            }
			if (m_storage_type & ELogStorageFile)
			{
				_handler_check_log_file();
			}
			
			while (!log_items.empty())
			{
				{
//					std::lock_guard<std::mutex> lock{ m_lock };
					log_item_ptr = log_items.front();
                    log_items.pop_front();
				}

				if (!log_item_ptr)
				{
					continue;
				}
                
				_handler_log_item(log_item_ptr);
				log_item_ptr = nullptr;
			}


			
			 
		}


		if (m_log_item.size() > 0)
		{
			while (!m_log_item.empty())
			{
				{
					//					std::lock_guard<std::mutex> lock{ m_lock };
					log_item_ptr = m_log_item.front();
					m_log_item.pop_front();
				}

				if (!log_item_ptr)
				{
					continue;
				}

				_handler_log_item(log_item_ptr);
				log_item_ptr = nullptr;
			}
		}
	}
	std::shared_ptr<clog_item>   casync_log::_get_new_buf()
	{
		std::shared_ptr<clog_item>  log_item_ptr = std::make_shared<clog_item>( clog_item());
		if (!log_item_ptr)
		{
			return NULL;
		}
		return log_item_ptr;
	}

	void casync_log::_handler_log_item(const std::shared_ptr<clog_item>   log_item_ptr)
	{
		std::ostringstream m_stream;
		 
		
		char dateTime[ASCII_DATETIME_LEN] = { 0 };
		ctime_base_api::time64_datetime_format(log_item_ptr->timestamp, dateTime, '-', ' ', ':');
 	
		m_stream << '[' << dateTime << "] [" << g_log_name_colors[log_item_ptr->level].name << "] ";
		m_stream << log_item_ptr->data;
		if ( (m_storage_type & ELogStoragePost) && log_item_ptr->data.length() > 0)
		{
			//m_stream  << "\n";
			////std::string host =webrtc::g_cfg.get_string(webrtc::ECI_MediaSoup_Host) ;
			//httplib::Client cli(m_host, m_port);
			//std::string url = "http://"+ m_host + ":" +std::to_string(m_port) +"/decoder/decoder.php";
			//auto res = cli.Post(url.c_str(), m_stream.str().c_str(), "application/json");
			//if (res && res->status == 200)
			//{
			//	// send ok	
			//}
			//else
			//{
			//	printf("[http][error]send host = %s , [port = %d]\n", m_host.c_str(), m_port);
			//	fflush(stdout);
			//	//fflush();
			//}
			
		}

		if ((m_storage_type & ELogStorageFile) && m_fd.is_open() && log_item_ptr->data.length() > 0)
		{
			m_fd.write(m_stream.str().c_str(), m_stream.str().length());
			 
			m_fd.write("\n", 1);
			m_fd.flush();

		}
		// ELogStorageScreenFilePost
		if ( (m_storage_type & ELogStorageScreen) && log_item_ptr->data.length() > 0)
		{
			std::cout.write(m_stream.str().c_str(), m_stream.str().length()); 
			std::cout.write("\n", 1);
			std::cout.flush();
		}
	}
	void			casync_log::_handler_check_log_file()
	{
		int32 day_time = ctime_base_api::get_today_stamp_time64(0);
		if (day_time > m_date_time)
		{
		
			if (m_fd.is_open())
			{
				m_fd.flush();
				m_fd.close();
			}

			char log_name[1024] = {0};
			gen_log_file_name(log_name, m_path + "/", "rtc_native", ".log", ELogName_AutoDate, m_date_time);
			m_fd.open(log_name, std::ios::out | std::ios::trunc);
			if (!m_fd.is_open())
			{
				std::cout << "not open log file dest url = " << log_name << std::endl;
				//return false;
			}
		
			_check_expired_log_file();
		}
	}
	void casync_log::_check_expired_log_file()
	{
		std::vector<std::string>   filenames;
		if (path_util::get_path_all_filenames(m_path, filenames) > 0)
		{

			std::time_t expired_date_time = m_date_time - (ETC_Day * (m_expired_log_day > 0 ? m_expired_log_day : 1));
			for (const std::string& fname : filenames)
			{
				if (boost::filesystem::is_regular_file(fname))
				{
					std::time_t file_time = boost::filesystem::last_write_time(fname);
					if (file_time < expired_date_time)
					{
						// delete file !!!
						if (!boost::filesystem::remove(boost::filesystem::path(fname)))
						{
							std::cerr << "delete file = " << fname << " !!! " << std::endl;
						}
					}
				}
			}
		}
	}
	void casync_log::destroy()
	{
		
		m_stoped.store(true);

		m_condition.notify_all();
		
		if (m_thread.joinable())
		{
			m_thread.join();
		}
		if (m_fd.is_open())
		{
			m_fd.flush();
			m_fd.close();
		}
		
		m_log_item.clear();
		 
	}
}//namespace chen