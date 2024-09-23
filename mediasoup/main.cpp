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
//#include <detours.h>
#include "cinput_device.h"

#include <shellapi.h>
#include "cwindow_capture_wgc.h" 
#include <UserEnv.h>
#include "cwindow_util.h"
#include "advapi.h"
#include "sbieapi.h"
#include <ntstatus.h>
#include <map>
#include "sbieiniwire.h"
#include "msgids.h"
#include "sbiedll.h"
#include "cwindow_util.h"
#include "my_version.h"
#include <ntstatus.h>
#include <Windows.h>
#include "advapi.h"
#include "sbieapi.h"
#include <ntstatus.h>
#include <map>
#include "sbieiniwire.h"
#include "msgids.h"
#include "sbiedll.h"
#include "cwindow_util.h"
#include "my_version.h"
cmediasoup::cmediasoup_mgr g_mediasoup_mgr;


bool stoped = false;

void signalHandler(int signum)
{
	stoped = true;
	
}





bool test_create_process_as_user()
{
	HANDLE hToken = NULL;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
	{
		return false;
	}

	HANDLE hTokenDup = NULL;
	bool bRet = DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityIdentification, TokenPrimary, &hTokenDup);
	if (!bRet || hTokenDup == NULL)
	{
		CloseHandle(hToken);
		return false;
	}

	DWORD dwSessionId = WTSGetActiveConsoleSessionId();
	//把服务hToken的SessionId替换成当前活动的Session(即替换到可与用户交互的winsta0下)
	if (!SetTokenInformation(hTokenDup, TokenSessionId, &dwSessionId, sizeof(DWORD)))
	{
		DWORD nErr = GetLastError();
		CloseHandle(hTokenDup);
		CloseHandle(hToken);
		return false;
	}

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));

	si.cb = sizeof(STARTUPINFO);
	//si.lpDesktop = LPWSTR("WinSta0\\Default");
	si.wShowWindow = SW_SHOW;
	si.dwFlags = STARTF_USESHOWWINDOW /*|STARTF_USESTDHANDLES*/;

	//创建进程环境块
	LPVOID pEnv = NULL;
	bRet = CreateEnvironmentBlock(&pEnv, hTokenDup, FALSE);
	if (!bRet)
	{
		CloseHandle(hTokenDup);
		CloseHandle(hToken);
		return false;
	}

	if (pEnv == NULL)
	{
		CloseHandle(hTokenDup);
		CloseHandle(hToken);
		return false;
	}

	//在活动的Session下创建进程
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));
	DWORD dwCreationFlag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | (CREATE_NO_WINDOW | CREATE_SUSPENDED
		| HIGH_PRIORITY_CLASS | ABOVE_NORMAL_PRIORITY_CLASS
		| BELOW_NORMAL_PRIORITY_CLASS | IDLE_PRIORITY_CLASS
		| CREATE_UNICODE_ENVIRONMENT);
	std::string strAppPath = "D:/Work/cabroad_server/Server/Sandboxie/Sandboxie/Bin/x64/SbieDebug/yuvplayer.exe";
	if (!CreateProcessAsUser(hTokenDup, NULL, LPWSTR(strAppPath.c_str()), NULL, NULL, FALSE, dwCreationFlag, pEnv, NULL, &si, &processInfo))
	{
		DWORD nRet = GetLastError();
		CloseHandle(hTokenDup);
		CloseHandle(hToken);
		return false;
	}

	DestroyEnvironmentBlock(pEnv);
	CloseHandle(hTokenDup);
	CloseHandle(hToken);
}

static wchar_t     g_password[66] = { 0 };
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
typedef NTSTATUS* PNTSTATUS;
BOOL   CallServerWithPassword(
	void* RequestBuf, WCHAR* pPasswordWithinRequestBuf,
	const std::wstring& SectionName, const std::wstring& SettingName)
{
	//  if (m_ServiceStopped)
	  //    return FALSE;

	BOOL ok = FALSE;
	ULONG status = STATUS_INSUFFICIENT_RESOURCES;

	BOOL FirstRetry = TRUE;

	while (1) {

		wcscpy(pPasswordWithinRequestBuf, g_password);

		MSG_HEADER* rpl = SbieDll_CallServer((MSG_HEADER*)RequestBuf);
		if (rpl) {
			status = rpl->status;
			SbieDll_FreeMem(rpl);
		}
		else
		{
			//status = STATUS_SERVER_DISABLED;
		}

		SecureZeroMemory(pPasswordWithinRequestBuf, sizeof(WCHAR) * 64);

		if (status != STATUS_WRONG_PASSWORD) {
			if (status == 0)
				ok = TRUE;
			break;
		}

		SecureZeroMemory(g_password, sizeof(g_password));

		if (FirstRetry)
		{
			FirstRetry = FALSE;
		}
		else {
			// int rc = CMyApp::MsgBox(NULL, MSG_4274, MB_YESNO);
			printf(" break;");
			//if (rc != IDYES)
			{
				break;
			}
		}

		//if (!InputPassword(MSG_4271, m_Password))
		  //  break;
	}

	if (!ok) {

		printf("create failed !!!\n");
		//CString err;
		//
		//if (status == STATUS_LOGON_NOT_GRANTED ||
		//    status == STATUS_WRONG_PASSWORD)
		//{
		//    err = CMyMsg(MSG_3312, SectionName);
		//}
		//else {
		//    err.Format(L"%08X", status);
		//    err = CMyMsg(MSG_3311, SectionName, SettingName, err);
		//}
		//
		//CMyApp::MsgBox(NULL, err, MB_OK);
	}

	return ok;
}


void get_box_name_all_pid_window_name(const WCHAR * box_name)
{
	//wchar_t name[256] = L"test02";
	ULONG m_pids[512] = { 0 };
	//wcscpy(name, m_name);
	wchar_t name[1024] = {0};
	SbieApi_EnumProcess( box_name, m_pids);
	int32_t i = 1;
	for (i = 1; i <= m_pids[0]; ++i) {

		SbieApi_QueryProcessEx(
			(HANDLE)(ULONG_PTR)m_pids[i], 255, NULL, name, NULL, NULL);

		if (name[0])
		{
			//  m_images[i] = name;
		}
		else
		{
			//m_images[i] = _unknown;
		}
		HWND www = chen::FindMainWindow(m_pids[i]);
		wchar_t   window_title[1024] = { 0 };
		int32_t ret = GetWindowText(www, window_title, 1024);
	//	printf("[pid = %u][name = %ws][wnd = %u][window_title = %ws]\n", m_pids[i], name, www, window_title);
		if (www != NULL)
		{
			g_pid = m_pids[i];
			break;
		}
		/*HICON icon;
		const WCHAR* title = theWindowTitleMap.Get(m_pids[i], icon);
		if (title)
			m_titles[i] = title;
		else
			m_titles[i] = CString();

		m_icons[i] = icon;*/
	}

	// GetWindowText();
}

void sandbox_create_new(std::wstring  Section, std::wstring  Setting, std::wstring  Value)
{

#define REQUEST_LEN                 (4096)
	SBIE_INI_SETTING_REQ* req =
		(SBIE_INI_SETTING_REQ*)malloc(REQUEST_LEN);

	BOOL ok = FALSE;
	if (req) {

		req->refresh = TRUE;
		// sandbox_create_new((char2wchar)(boxName.c_str()), (L"Enabled"), L"y");
		wcscpy(req->section, Section.c_str() );
		wcscpy(req->setting, Setting.c_str());

		wcscpy(req->value, Value.c_str());
		req->value_len = Value.length();

		req->h.msgid = MSGID_SBIE_INI_SET_SETTING;
		req->h.length = sizeof(SBIE_INI_SETTING_REQ)
			+ req->value_len * sizeof(WCHAR);

		ok = CallServerWithPassword(
			req, req->password, Section, Setting);

		free(req);
	}
}


wchar_t* char2wchar(const char* cchar)
{
	wchar_t* m_wchar;
	int len = MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), NULL, 0);
	m_wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, cchar, strlen(cchar), m_wchar, len);
	m_wchar[len] = '\0';
	return m_wchar;
}

static void work_dir_path(std::string &work_path)
{
	WCHAR czFileName[1024] = { 0 };

	GetModuleFileName(NULL, czFileName, _countof(czFileName) - 1);
	//	std::to_string(czFileName);
		 //第一次调用确认转换后单字节字符串的长度，用于开辟空间
	int pSize = WideCharToMultiByte(CP_OEMCP, 0, czFileName, wcslen(czFileName), NULL, 0, NULL, NULL);
	char* pCStrKey = new char[pSize + 1];
	if (!pCStrKey)
	{
		//ERROR_EX_LOG("alloc failed !!!");
		return;
	}
	//第二次调用将双字节字符串转换成单字节字符串
	WideCharToMultiByte(CP_OEMCP, 0, czFileName, wcslen(czFileName), pCStrKey, pSize, NULL, NULL);
	work_path = pCStrKey;
	//std::string path(czFileName);
	//NORMAL_EX_LOG("work path = %s", pCStrKey);
	if (pCStrKey)
	{
		delete[] pCStrKey;
		pCStrKey = NULL;
	}
}
void open_start_app(std::wstring app_path_param)
{
	STARTUPINFO si = { sizeof(si) };
	GetStartupInfo(&si);
	si.wShowWindow = SW_HIDE;
	std::string work_path;
	wchar_t buffer[MAX_PATH] = {0};
	GetCurrentDirectory(MAX_PATH, buffer);
	//work_dir_path(work_path);
	std::wstring app_work_path = L"D:/Work/cabroad_server/Server/ccloud_game_rtc/mediasoup";// = L"D:/Work/cabroad_server/Server/Sandboxie/Sandboxie/Bin/x64/SbieDebug";
	PROCESS_INFORMATION info;
	//std::wstring app_path_param = L"Start.exe /box:test03  C:/Program Files/Google/Chrome/Application/chrome.exe  --incognito   http://baidu.com   --start-maximized --kiosk";
	bool ret = ::CreateProcess(/*LPSTR(app_path_name.c_str())*/NULL, LPWSTR(app_path_param.c_str()), NULL, NULL, CREATE_SUSPENDED, NORMAL_PRIORITY_CLASS, NULL/*environment*/, buffer, &si, &info);
	::CloseHandle(info.hProcess);
	::CloseHandle(info.hThread);
}
int  test_main(int argc, char *argv[])
{
	//test_create_process_as_user();
	//return EXIT_SUCCESS;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	std::string  boxName = std::string(argv[1]);
	std::string  apppathName = argv[2];
	std::string	 roomname = argv[3];
	std::string  media_ip = argv[4];
	uint32_t	media_port = std::atoi(argv[5]);
	std::string url = argv[6];
	printf("[boxname = %s][app = %s][roomname = %s][media_ip = %s][media_port = %u]\n", boxName.c_str(), apppathName.c_str(), roomname.c_str(), 
	media_ip.c_str(), media_port);
	 sandbox_create_new((char2wchar)(boxName.c_str()), (L"Enabled"), L"y");
	// chrome.exe  http://www.baidu.com    --disable-notifications  --disable-restore-session-state   --noerrdialogs    --disable-infobars  --disable-session-crashed-bubble   --disable-notification  --incognito   --start-fullscreen   --no-default-browser-check
	//// --start-maximized 
	 std::string app_param = "Start.exe /box:" + boxName + "  " + apppathName + "   --app=" + url + "    --disable-notifications  --disable-restore-session-state   --noerrdialogs    --disable-infobars  --disable-session-crashed-bubble   --disable-notification  --incognito   --start-fullscreen   --no-default-browser-check ";
	 
	 open_start_app((char2wchar)(app_param.c_str()));
	////g_pid 
	while (g_pid == 0)
	{
		get_box_name_all_pid_window_name(char2wchar(boxName.c_str()));
		if (g_pid != 0)
		{
			break;
		}
		Sleep(10);
	//	open_start_app((char2wchar)(app_param.c_str()));
	}
	//std::string  
	Sleep(10);

	/*chen::WindowCapture window_capture;

	window_capture.Init(60, 1);
	window_capture.StartCapture();


	while (1)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return EXIT_SUCCESS;*/
	//g_pid = 43032;
	g_mediasoup_mgr.init( 0);

	//g_mediasoup_mgr.set_mediasoup_status_callback(&mediasoup_callback);
	/*
	const char* mediasoupIp, uint16_t port
		, const char* roomName, const char* clientName
	
	*/
	
	g_mediasoup_mgr.startup(media_ip.c_str(), media_port, roomname.c_str(), roomname.c_str());
	while (!stoped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	g_mediasoup_mgr.destroy();
	return 0;
}

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
//void load_seecen()
//{
//	if (userdll32_ptr)
//	{
//		return;
//	}
//	char system_path[MAX_PATH] = { 0 };
//
//	UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
//	if (!ret)
//	{
//		//ERROR_EX_LOG("Failed to get windows system path: %lu\n", GetLastError());
//		//return false;
//	}
//	userdll32_ptr = get_system_module(system_path, "user32.dll");
//	if (!userdll32_ptr)
//	{
//		return;
//	}
//	void* EnumDisplaySettingsA_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsA");
//	void* EnumDisplaySettingsW_proc = GetProcAddress(userdll32_ptr, "EnumDisplaySettingsW");
//	{
//		//	SYSTEM_LOG("    input device  begin ... ");
//		DetourTransactionBegin();
//
//		if (EnumDisplaySettingsA_proc)
//		{
//			RealEnumDisplaySettingsA = (PFN_EnumDisplaySettingsA)EnumDisplaySettingsA_proc;
//			DetourAttach((PVOID*)&RealEnumDisplaySettingsA,
//				hook_EnumDisplaySettingsA);
//		}
//		if (EnumDisplaySettingsW_proc)
//		{
//			RealEnumDisplaySettingsW = (PFN_EnumDisplaySettingsW)EnumDisplaySettingsW_proc;
//			DetourAttach((PVOID*)&RealEnumDisplaySettingsW,
//				hook_EnumDisplaySettingsW);
//		}
//		const LONG error = DetourTransactionCommit();
//		const bool success = error == NO_ERROR;
//	}
//}
//__declspec(dllexport)
//BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
//{
//
//	if (reason == DLL_PROCESS_ATTACH)
//	{
//
//		//load_seecen();
//		wchar_t name[MAX_PATH];
//		 
//		/* this prevents the library from being automatically unloaded
//		 * by the next FreeLibrary call */
//		GetModuleFileNameW(hinst, name, MAX_PATH);
//		LoadLibraryW(name);
//		 
//		load_seecen();
//
//
//
//	}
//	else if (reason == DLL_PROCESS_DETACH) {
//
//
//		 
//
//		//free_hook();
//	}
//
//	(void)unused1;
//	return true;
//	return true;
//}