/***********************************************************************************************
					created: 		2019-04-27

					author:			chensong

					purpose:		config
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
#include "cconfig.h"
#include <fstream>
#include "rtc_base/logging.h"
//#include "clog.h"
//#include "Util/Logging.hpp"
//using namespace logging;
#include "cnet_types.h"
namespace chen {
	using namespace rtc;
	cconfig::cconfig():m_configs(NULL), m_values_size(0)
	{
	}


	cconfig::~cconfig()
	{
	}

	bool cconfig::init(int64 values_count, const char * file_name)
	{
		if (values_count < 1 || !file_name )
		{
			//ERROR_EX_LOG(" m_config max values_count = %ld, cfg file ptr= NULL ", values_count);
			RTC_LOG(LS_ERROR) << "m_config max values_count = " << values_count << ", cfg file ptr= NULL ";
			return false;
		}

		if (!m_cfg_file.init(file_name))
		{
			return false;
		}
		m_configs = new cnode[values_count];
		if (!m_configs)
		{
			//ERROR_EX_LOG("alloc m_config max values_count = %lu", values_count);
			RTC_LOG(LS_ERROR) << "alloc m_config max values_count = " << values_count;
			return false;
		}
		

		m_values_size = values_count;
		return true;
	}

	void cconfig::destroy()
	{
		
		if (m_configs)
		{
			delete[] m_configs;
			m_configs = NULL;
		}

		m_values_size = 0;
	}

	void cconfig::set_string(int32 index,const std::string key, std::string value)
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("set_string index = %ld, num = %ld, key = %s, value = %s", m_values_size, index, key.c_str(), value.c_str());
			RTC_LOG(LS_ERROR) << "set_string index = " << m_values_size << ", num = " << index << ", key = " << key.c_str() << ", value = " << value.c_str();
			return;
		}
		m_configs[index].init(key, EDataTypeString);
		m_configs[index].m_data = value;
	}

	void cconfig::set_int32(int32 index, const std::string key, int32 value)
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("set_int32 index = %ld, num = %ld, key = %s, value = %d", m_values_size, index, key.c_str(), value);
			RTC_LOG(LS_ERROR) << "set_int32 index = " << m_values_size << ", num = " << index << ", key = " << key.c_str() << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeint);
		m_configs[index].m_int32 = index;
	}

	void cconfig::set_uint32(int32 index, const std::string key, uint32 value)
	{
		if (m_values_size < index)
		{
		//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key.c_str(), value);
			RTC_LOG(LS_ERROR) << "set_uint32 index = " << m_values_size << ", num = " << index << ", key = " << key.c_str() << ", value = " << value;

			return;
		}
		m_configs[index].init(key, EDataTypeuint32);
		m_configs[index].m_uint32 = value;
	}

	void cconfig::set_string(int32 index, int32, const char * key, const char * value)
	{
		if (m_values_size < index)
		{
		//	ERROR_EX_LOG("set_string index = %ld, num = %ld, key = %s, value = %s", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_string index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;

			return;
		}
		m_configs[index].init(key, EDataTypeString);
		m_configs[index].m_data = value;
	}
	void cconfig::show()
	{
		for (int i = 0; i < m_values_size; ++i)
		{
			printf("i = %d, key = %s, value = %s\n", i, m_configs[i].m_name.c_str(), m_configs[i].m_data.c_str());
		}
	}
	void cconfig::set_int32(int32 index, const char * key, int32 value)
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("set_int32 max_num = %ld, num = %ld, key = %s, value = %d", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_int32 index = " << m_values_size << ", num = " << index << ", key = " << key<< ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeint);
		m_configs[index].m_int32 = value;
	}

	void cconfig::set_uint32(int32 index, const char * key, uint32 value)
	{
		if (m_values_size < index)
		{
		//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_uint32 index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeuint32);
		m_configs[index].m_uint32 = value;
	}


	void cconfig::set_int64(int32 index, const std::string key, int64 value)
	{
		if (m_values_size < index)
		{
			//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_int64 index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeint64);
		m_configs[index].m_int64 = value;
	}
	void cconfig::set_uint64(int32 index, const std::string key, uint64 value)
	{
		if (m_values_size < index)
		{
			//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_uint64 index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeuint64);
		m_configs[index].m_uint64 = value;
	}
	void cconfig::set_int64(int32 index, const char* key, int64 value)
	{
		if (m_values_size < index)
		{
			//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_int64 index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeint64);
		m_configs[index].m_int64 = value;
	}
	void cconfig::set_uint64(int32 index, const char* key, uint64 value)
	{
		if (m_values_size < index)
		{
			//	ERROR_EX_LOG("set_uint32 index = %ld, num = %ld, key = %s, value = %ld", m_values_size, index, key, value);
			RTC_LOG(LS_ERROR) << "set_uint64 index = " << m_values_size << ", num = " << index << ", key = " << key << ", value = " << value;
			return;
		}
		m_configs[index].init(key, EDataTypeuint64);
		m_configs[index].m_uint64 = value;
	}
	std::string cconfig::get_string(int64 index)const
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("get_string index = %ld, num = %ld", m_values_size, index);
			RTC_LOG(LS_ERROR) << " get_string index = " << m_values_size << ", num = " << index;
			return "";
		}
		return m_configs[index].m_data;
	}

	int32 cconfig::get_int32(int64 index)const
	{
		if (m_values_size < index)
		{
		//	ERROR_EX_LOG("get_int32 index = %ld, num = %ld", m_values_size, index);
			RTC_LOG(LS_ERROR) << "get_int32 index = " << m_values_size << ", num= " << index;
			return 0;
		}
		return m_configs[index].m_int32;
	}

	uint32 cconfig::get_uint32(int64 index)const
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("get_int32 index = %ld, num = %ld", m_values_size, index);
			RTC_LOG(LS_ERROR) << "get_int32 index = " << m_values_size << ", num= " << index;
			return 0;
		}
		return m_configs[index].m_uint32;
	}
	int64 cconfig::get_int64(int64 index) const
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("get_int32 index = %ld, num = %ld", m_values_size, index);
			RTC_LOG(LS_ERROR) << "get_int64 index = " << m_values_size << ", num= " << index;
			return 0;
		}
		return m_configs[index].m_int64;
	}
	uint64 cconfig::get_uint64(int64 index)const
	{
		if (m_values_size < index)
		{
			//ERROR_EX_LOG("get_int32 index = %ld, num = %ld", m_values_size, index);
			RTC_LOG(LS_ERROR) << "get_uint64 index = " << m_values_size << ", num= " << index;
			return 0;
		}
		return m_configs[index].m_uint64;
	}
	void cconfig::load_cfg_file()
	{
		for (int64 i = 0; i < m_values_size; ++i)
		{
			if (m_cfg_file.is_key(m_configs[i].m_name))
			{
				if (m_configs[i].m_type == EDataTypeNULL)
				{
					//ERROR_EX_LOG(" i = %ld, type = null , name = %s", i, m_configs[i].m_name.c_str());
					RTC_LOG(LS_ERROR) << "i = " << i << ", type = " << m_configs[i].m_name.c_str();
				}
				else if (m_configs[i].m_type == EDataTypeString)
				{
					m_configs[i].m_data = m_cfg_file.get_value(m_configs[i].m_name);
				}
				else if (m_configs[i].m_type == EDataTypeint)
				{
					m_configs[i].m_int32 = std::atoi(m_cfg_file.get_value(m_configs[i].m_name).c_str());
				}
				else if (m_configs[i].m_type == EDataTypeuint32)
				{
					m_configs[i].m_uint32 = std::atol(m_cfg_file.get_value(m_configs[i].m_name).c_str());
				}
				else if (m_configs[i].m_type == EDataTypeuint64)
				{
					m_configs[i].m_uint64 = std::atoll(m_cfg_file.get_value(m_configs[i].m_name).c_str());
				}
				else if (m_configs[i].m_type == EDataTypeint64)
				{
					m_configs[i].m_int64 = std::atoll(m_cfg_file.get_value(m_configs[i].m_name).c_str());
				}
				else
				{
					//ERROR_EX_LOG(" i = %ld, type = is not find , name = %s", i, m_configs[i].m_name.c_str());
					RTC_LOG(LS_ERROR) << "i = " << i << ", type = is not find , name = " << m_configs[i].m_name.c_str();
				}
			}
		}
		m_cfg_file.destroy();
	}

	ccfg_file::ccfg_file()
	{
	}

	ccfg_file::~ccfg_file()
	{
	}

	bool ccfg_file::init(const char * file_name)
	{

		std::ifstream  input(file_name);
		if (!input)
		{
			//ERROR_EX_LOG("open config file fail file = %s", file_name);
			RTC_LOG(LS_ERROR) << "open config file fail file = " << file_name;
			return false;
		}
		std::string line, key, value;
		while (getline(input, line))
		{
			if (_parser_line(line, key, value))
			{
				_insert_data(key, value);
				//m_configs[1].m_data = value; // 保存到map容器中的方法。
			}
		}
		if (input)
		{
			input.close();
		}
		return true;
	}

	void ccfg_file::destroy()
	{
		m_config_map.clear();
	}


	bool ccfg_file::_parser_line(const std::string & line, std::string & key, std::string & value)
	{
		if (line.empty())
		{
			return false;
		}
		std::string::size_type start_pos = 0, end_pos = line.size() - 1, pos;
		if ((pos = line.find('#')) != std::string::npos)
		{
			if (pos == 0)
			{
				return false;// #line---
			}
			end_pos = pos - 1;
		}

		std::string new_line = line.substr(start_pos, end_pos - start_pos + 1); // 删掉后半部分的注释 FIX_ME： 这里应该是减错了吧
																				// 下面pos的赋值时必要的，这样，就可在后面得到Key和value值了。
		if ((pos = new_line.find("=")) == std::string::npos) //说明前面没有 = 号
		{
			return false;
		}
		key = new_line.substr(0, pos); // 获得key
		value = new_line.substr(pos + 1, end_pos + 1 - (pos + 1)); // 获得value
		_trim(key);
		if (key.empty())
		{
			return false;
		}
		_trim(value); // 因为这里的key和value都是引用传递，可以直接被修改，所以不用返回
		return true;
	}


	bool ccfg_file::_is_space(char c)
	{
		if (c == ' ' || c == '\t')
		{
			return true;
		}
		else
		{
			return false;
		}
		return true;
	}

	bool ccfg_file::_iscommentchar(char c)
	{
		if (c == '#')
		{
			return true;
		}
		else
		{
			return false;
		}
		return true;
	}

	void ccfg_file::_trim(std::string & str)
	{
		if (str.empty())
		{
			return;
		}
		size_t i, start_pos, end_pos;
		for (i = 0; i < str.size(); i++)
		{
			if (!_is_space(str[i]))
			{
				break;
			}
		}
		if (i == str.size())
		{
			str = "";
			return;
		}
		start_pos = i; // 获取到非空格的初始位置

		for (i = str.size() - 1; i >= 0; i--)
		{
			if (!_is_space(str[i]))
			{
				break;
			}
		}
		end_pos = i;
		str = str.substr(start_pos, end_pos - start_pos + 1);
	}



	bool ccfg_file::is_key(const std::string & key) const
	{
		CCONFIG_MAP::const_iterator iter = m_config_map.find(key);
		if (iter != m_config_map.end())
		{
			return true;
		}
		return false;
	}

	std::string  ccfg_file::get_value(const std::string & key)
	{
		// TODO: 在此处插入 return 语句
		CCONFIG_MAP::const_iterator iter = m_config_map.find(key);
		if (iter != m_config_map.end())
		{
			return iter->second;
		}
		return NULL;
	}

	void ccfg_file::_insert_data(const std::string & key, const std::string & value)
	{
		if (!m_config_map.insert(std::make_pair(key, value)).second)
		{
			//ERROR_EX_LOG("read cfg insert  key = %s, value = %s", key.c_str(), value.c_str());
			RTC_LOG(LS_ERROR) << "read cfg insert  key = " << key.c_str() << ", value = " << value;
		}
	}
}//namespace chen
