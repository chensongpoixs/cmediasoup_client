/***********************************************************************************************
    created:         2019-03-06
    
    author:            chensong
                    
    purpose:        ccrypto
************************************************************************************************/
#ifndef _C_UTIL_H_
#define _C_UTIL_H_

//#include "type_define.h"
#include <string>
#include <vector>
//#include "cnet_type.h"
#include "cnet_types.h"
namespace chen
{
	namespace cutil
	{
		template<typename T>
		void rand_bytes(unsigned char *buf, int num, T& rand_func)
		{
			for (int i = 0; i < num; ++i)
			{
				buf[i] = rand_func() & 0xFF;
			}
		}

		std::string get_hex_str(const void *buf, int num);
	}




	namespace path_util
	{
		std::string convert_path(const std::string& path);


		std::string parent_path(const std::string& path);


		uint32 get_path_all_filenames(const std::string & path, std::vector<std::string> & filenames);
	}








}
#endif // _C_UTIL_H_
