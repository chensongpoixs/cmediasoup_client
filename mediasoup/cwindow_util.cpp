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
#include "cwindow_util.h"
#if defined(_MSC_VER)
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include "ccfg.h"
#include <thread>

namespace chen {






	int Pnum = 0, Cnum;//父窗口数量，每一级父窗口的子窗口数量

					   //---------------------------------------------------------
					   //EnumChildWindows回调函数，hwnd为指定的父窗口
					   //---------------------------------------------------------
	BOOL CALLBACK EnumChildWindowsProc(HWND hWnd, LPARAM lParam)
	{
		wchar_t WindowTitle[100] = { 0 };
		Cnum++;
		::GetWindowText(hWnd, WindowTitle, 100);
		
		return true;
	}
	//---------------------------------------------------------
	//EnumWindows回调函数，hwnd为发现的顶层窗口
	//---------------------------------------------------------
	BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
	{
		if (GetParent(hWnd) == NULL && IsWindowVisible(hWnd))  //判断是否顶层窗口并且可见
		{
			Pnum++;
			Cnum = 0;
			wchar_t WindowTitle[100] = { 0 };
			::GetWindowText(hWnd, WindowTitle, 100);
			
			EnumChildWindows(hWnd, EnumChildWindowsProc, NULL); //获取父窗口的所有子窗口
		}
		return true;
	}
	//---------------------------------------------------------
	//main函数
	//---------------------------------------------------------

	//获取屏幕上所有的顶层窗口,每发现一个窗口就调用回调函数一次


	struct handle_data {
		unsigned long process_id;
		HWND best_handle;
	};

	BOOL IsMainWindow(HWND handle)
	{
		return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
	}
	BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
	{
		handle_data& data = *(handle_data*)lParam;
		unsigned long process_id = 0;
		GetWindowThreadProcessId(handle, &process_id);
		//printf("process_id = %lu\n", process_id);
		if (data.process_id != process_id || !IsMainWindow(handle)) {
			return TRUE;
		}
		data.best_handle = handle;
		return FALSE;
	}

	HWND FindMainWindow()
	{ 
		/*handle_data data;
		data.process_id = ::GetCurrentProcessId();
		data.best_handle = 0;
		EnumWindows(EnumWindowsCallback, (LPARAM)&data);
		return data.best_handle;*/

		uint32 wid  = g_cfg.get_uint32(ECI_SharedWindowId);

		return (HWND)wid;

	}

	HWND FindMainWindow(long long id)
	{
		handle_data data;
		data.process_id = id;
		data.best_handle = 0;
		EnumWindows(EnumWindowsCallback, (LPARAM)&data);
		return data.best_handle;
	}
	
	/** 
	* TODO@chensong 2022-01-19 
	* 获取子窗口的api  -> 搞怎么一个东西用 三天 哭了
	*/
	HWND MainChildPoint(HWND mwnd, POINT pt)
	{
		if (!mwnd)
		{
			return NULL;
		}
		return ::ChildWindowFromPoint(mwnd, pt);
	}
}

#endif //#if defined(_MSC_VER)