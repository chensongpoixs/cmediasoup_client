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
#ifndef _C_CONFIG_H_
#define _C_CONFIG_H_
//#include "cnoncopyable.h"
#include <map>
#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>
#include <iostream>
#include <string>
#include "cnet_types.h"
namespace chen {
	class ccfg_file //: private cnoncopyable
	{
	private:
		typedef std::unordered_map<std::string, std::string>	CCONFIG_MAP;
	public:
		ccfg_file();
		~ccfg_file();
	public:
		bool init(const char * file_name);
		void destroy();
	private:
		bool _parser_line(const std::string &line, std::string& key, std::string& value);
		bool _is_space(char c);
		bool _iscommentchar(char c);
		void _trim(std::string & str);
	public:
		bool   is_key(const std::string &key)const;
		std::string get_value(const std::string& key);
	private:
		ccfg_file(const ccfg_file&);
		ccfg_file& operator=(const ccfg_file&);
	private:
		void _insert_data(const std::string& key, const std::string &value);
	private:
		CCONFIG_MAP								m_config_map;
	};
	class cconfig //: private cnoncopyable
	{
	public:
		explicit cconfig();
		virtual ~cconfig();

	public:
	    bool init(int64 values_count, const char * file_name);
		void destroy();
	public:
		void set_string(int32 index , const std::string key, std::string value);
		void set_int32(int32 index, const std::string key, int32 value);
		void set_uint32(int32 index, const std::string key, uint32 value);
		void set_string(int32 index, int32, const char * key, const char * value);
		void set_int32(int32 index, const char * key, int32 value);
		void set_uint32(int32 index, const char * key, uint32 value);
		void set_int64(int32 index, const std::string key, int64 value);
		void set_uint64(int32 index, const std::string key, uint64 value);
		void set_int64(int32 index, const char* key, int64 value);
		void set_uint64(int32 index, const char* key, uint64 value);
		void show();

	public:
		std::string get_string(int64 index) const ;
		int32 get_int32(int64 index) const;
		uint32 get_uint32(int64 index)const;
		int64 get_int64(int64 index) const;
		uint64 get_uint64(int64 index)const;
	public:
		void load_cfg_file();
	private:
		cconfig(const cconfig&);
		cconfig& operator =(const cconfig&);
	private:
		enum EValueType {
			EDataTypeNULL = 1,
			EDataTypeint,
			EDataTypeuint32,
			EDataTypeint64,
			EDataTypeuint64,
			EDataTypeString,
		};
		typedef struct cnode {
			std::string m_name;
			EValueType	m_type;
			union
			{
				int32	m_int32;
				uint32	m_uint32;
				
			};
			union
			{
				int64   m_int64;
				uint64  m_uint64;
			};
			std::string m_data;

			cnode()
			{
				m_name.clear(); m_data.clear(); m_type = EDataTypeNULL;
			}
			~cnode()
			{
				m_name.clear(); m_data.clear(); m_type = EDataTypeNULL;

			}
			void init(const std::string& name, EValueType type)
			{
				m_name = name;
				m_type = type;
			}
		}cnode;

		cnode*			m_configs;
		int64           m_values_size;
		ccfg_file		m_cfg_file;
	};
}//namespace chen

#endif //!#define _C_CONFIG_H_

