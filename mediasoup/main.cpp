/*
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
*/
#include <iostream>
#include <csignal> // sigsuspend()
#include <cstdlib>
#include <iostream>
#include <string>
#include "cmediasoup_mgr.h"
#include <detours.h>
#include "cinput_device.h"
#include <shellapi.h>
#include "cclient.h"
 
chen::cmediasoup_mgr g_mediasoup_mgr;


bool stoped = false;

void signalHandler(int signum)
{
	stoped = true;
	
}



//int  main(int argc, char *argv[])
//{
//	signal(SIGINT, signalHandler);
//	signal(SIGTERM, signalHandler);
//
//
//	std::string media_ip = argv[1];
//	uint32_t  media_port = std::atoi(argv[2]);
//	std::string room_name = argv[3];
//	std::string user_name = argv[4];
//
//	chen::g_gpu_address = std::atoll(argv[5]);
//
//	
//	g_mediasoup_mgr.init(0);
//
//	//g_mediasoup_mgr.set_mediasoup_status_callback(&mediasoup_callback);
//	/*
//	const char* mediasoupIp, uint16_t port
//		, const char* roomName, const char* clientName
//	
//	*/
//	g_mediasoup_mgr.startup(media_ip.c_str(), media_port, room_name.c_str(), user_name.c_str());
//	while (!stoped)
//	{
//		std::this_thread::sleep_for(std::chrono::milliseconds(100));
//	}
//	g_mediasoup_mgr.destroy();
//	return 0;
//}

static  HMODULE  userdll32_ptr = NULL;

typedef BOOL(WINAPI* PFN_EnumDisplaySettingsA)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
PFN_EnumDisplaySettingsA RealEnumDisplaySettingsA;
typedef BOOL(WINAPI* PFN_EnumDisplaySettingsW)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
PFN_EnumDisplaySettingsW RealEnumDisplaySettingsW;

std::string WcharTochar(const std::wstring& wp, size_t m_encode = CP_ACP)
{
	std::string str;
	int32_t len = WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), NULL, 0, NULL, NULL);
	str.resize(len);
	WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), (LPSTR)(str.data()), len, NULL, NULL);
	return str;
}
static BOOL hook_EnumDisplaySettingsA(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
{

	BOOL ret = RealEnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode);
	if (ret)
	{
		//::GetCommandLineA();
		LPWSTR* szArglist;
		int nArgs;
		szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
		int32_t width = 1920;
		int32_t height = 1080;
		if (nArgs > 14)
		{
			// 0 0 1920 1040
			// 11 12 13 14
			width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
			height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
			//NORMAL_EX_LOG("width  = %u, height = %u", width, height);
			lpDevMode->dmPelsWidth = width;
			lpDevMode->dmPelsHeight = height;
		}

	}
	return ret;
}
static BOOL hook_EnumDisplaySettingsW(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
{

	BOOL ret = RealEnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
	if (ret)
	{
		LPWSTR* szArglist;
		int nArgs;
		szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
		int32_t width = 1920;
		int32_t height = 1080;

		if (nArgs > 14)
		{
			// 0 0 1920 1040
			// 11 12 13 14
			width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
			height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
			//NORMAL_EX_LOG("width  = %u, height = %u", width, height);
			lpDevMode->dmPelsWidth = width;
			lpDevMode->dmPelsHeight = height;
		}

	}
	return ret;
}
static inline HMODULE get_system_module(const char* system_path, const char* module)
{
	char base_path[MAX_PATH];

	strcpy(base_path, system_path);
	strcat(base_path, "\\");
	strcat(base_path, module);
	return GetModuleHandleA(base_path);
}
void load_seecen()
{
	if (userdll32_ptr)
	{
		return;
	}
	char system_path[MAX_PATH] = { 0 };

	UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
	if (!ret)
	{
		//ERROR_EX_LOG("Failed to get windows system path: %lu\n", GetLastError());
		//return false;
	}
	userdll32_ptr = get_system_module(system_path, "user32.dll");
	if (!userdll32_ptr)
	{
		return;
	}
	void* EnumDisplaySettingsA_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsA");
	void* EnumDisplaySettingsW_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsW");
	{
		//	SYSTEM_LOG("    input device  begin ... ");
		DetourTransactionBegin();

		if (EnumDisplaySettingsA_proc)
		{
			RealEnumDisplaySettingsA = (PFN_EnumDisplaySettingsA)EnumDisplaySettingsA_proc;
			DetourAttach((PVOID*)&RealEnumDisplaySettingsA,
				hook_EnumDisplaySettingsA);
		}
		if (EnumDisplaySettingsW_proc)
		{
			RealEnumDisplaySettingsW = (PFN_EnumDisplaySettingsW)EnumDisplaySettingsW_proc;
			DetourAttach((PVOID*)&RealEnumDisplaySettingsW,
				hook_EnumDisplaySettingsW);
		}
		const LONG error = DetourTransactionCommit();
		const bool success = error == NO_ERROR;
	}
}



#if 0
__declspec(dllexport)
BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{

	if (reason == DLL_PROCESS_ATTACH)
	{

		//load_seecen();
		wchar_t name[MAX_PATH];
		 
		/* this prevents the library from being automatically unloaded
		 * by the next FreeLibrary call */
		GetModuleFileNameW(hinst, name, MAX_PATH);
		LoadLibraryW(name);
		 
	//	load_seecen();



	}
	else if (reason == DLL_PROCESS_DETACH) {


		 

		//free_hook();
	}

	(void)unused1;
	return true;
	return true;
}

#endif // #if 0