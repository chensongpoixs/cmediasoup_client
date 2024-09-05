/***********************************************************************************************
created: 		2022-01-19

author:			chensong

purpose:		input_device
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

#include "cinput_device.h"
#include "cwindow_util.h"
#include "cprotocol.h"
#include "cinput_device_event.h"
#include "rtc_base/logging.h"
#include "clog.h"
#if defined(_MSC_VER)
// TODO@chensong 20220711  UE 鼠标控制权问题
#define _WIN32_WINNT 0x0400 
#include <Windows.h>
#include <WinUser.h>
#include <UserEnv.h>
#include <mutex>
#include <detours.h>
#include "cint2str.h"
#include <shellapi.h>
uint64_t g_pid = 0;
//void CallMessage(HWND hwnd, int nMsgId, int wParam, int lParam)
//
//{
//
//	WNDPROC fWndProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC); //获取窗口wndproc
//
//	if (fWndProc != NULL)
//
//	{
//
//		fWndProc(hwnd, nMsgId, wParam, lParam);
//
//	}
//
//}
#endif // WIN
namespace chen {
	int32_t  g_width = 0;
	int32_t  g_height = 0;
	using FKeyCodeType = uint8;
	using FCharacterType = TCHAR;
	using FRepeatType = uint8;
	using FButtonType = uint8;
	using FPosType = uint16;
	using FDeltaType = int16;
	//using FTouchesType = TArray<FTouch>;
	using FControllerIndex = uint8;
	using FControllerButtonIndex = uint8;
	using FControllerAnalog = double;
	using FControllerAxis = uint8;


	//static long long timeA = systemtime();
	//std::chrono::milliseconds
	//cout << timeA << endl;


	////PostMessage
//#define MESSAGE(g_wnd, message_id, param1, param2) MSG msg;  \
//														  msg.hwnd = g_wnd;			\
//												          msg.message = message_id;	\
//														  msg.wParam = param1;		\
//														  msg.lParam = param2;		\
//			std::chrono::steady_clock::time_point cur_time_ms = std::chrono::steady_clock::now();     \
//											long long ms = static_cast<long long >(cur_time_ms.time_since_epoch().count() / 1000000); \
//												 msg.time = static_cast<DWORD>(ms);	\
//														  msg.pt.x = g_width; msg.pt.y = g_height; \
//														::TranslateMessage(&msg);         \
//													long long ret_dispatch =	 ::DispatchMessage(&msg); 
//													//NORMAL_EX_LOG("move cur_ms = %u, [ret_dispatch = %s]", ms, std::to_string(ret_dispatch).c_str());
//

#define MESSAGE(g_wnd, message_id, param1, param2)  PostMessage(g_wnd, message_id, param1, param2);



//#define MOUSE_INPUT(g_wnd)	

	//if (GetCapture() != g_wnd)
#define MOUSE_INPUT(g_wnd)		 { \
								SetForegroundWindow(g_wnd);  \
								SetCapture(g_wnd);           \
								SetFocus(g_wnd); }

#define CliENTTOSCREENPOINT(hwnd, xx, yy) POINT CursorPoint; \
	CursorPoint.x = xx; \
	CursorPoint.y = yy; \
	ClientToScreen(hwnd, &CursorPoint);
	//BringWindowToTop(g_wnd);	\
	//	SetForegroundWindow(g_wnd); \
	// SetWindowLong(g_wnd, GWL_STYLE, GetWindowLong(g_wnd, GWL_STYLE) | WS_EX_TRANSPARENT | WS_EX_LAYERED);
	//使用全局变量操作的哈 
#define SET_POINT() POINT pt; pt.x = g_width; pt.y = g_height;

#if defined(_MSC_VER)
#define WINDOW_MAIN()		HWND mwin = FindMainWindow(g_pid)
#define WINDOW_CHILD()	HWND childwin = MainChildPoint(mwin, pt); 
#define WINDOW_BNTTON_DOWN(v)  uint32 active_type = WM_LBUTTONDOWN;					 \
	switch (vec.button)																 \
	{                                                                             	 \
		case EMouseButtons::Left:													 \
		{																			 \
			active_type = WM_LBUTTONDOWN;											 \
			break;																	 \
		}																			 \
		case EMouseButtons::Middle:													 \
		{																			 \
			active_type = WM_MBUTTONDOWN;											 \
																					 \
			break;																	 \
		}																			 \
		case EMouseButtons::Right:													 \
		{																			 \
			active_type = WM_RBUTTONDOWN;											 \
			break;																	 \
		}																			 \
		default:																	 \
							 														 \
			break;																	 \
	}

#define WINDOW_BNTTON_UP(v)  uint32 active_type = WM_LBUTTONUP; switch (vec.button)   \
	{                                                                             	 \
		case EMouseButtons::Left:													 \
		{																			 \
			active_type = WM_LBUTTONUP;											 \
			break;																	 \
		}																			 \
		case EMouseButtons::Middle:													 \
		{																			 \
			active_type = WM_MBUTTONUP;											 \
																					 \
			break;																	 \
		}																			 \
		case EMouseButtons::Right:													 \
		{																			 \
			active_type = WM_RBUTTONUP;											 \
			break;																	 \
		}																			 \
		default:																	 \
							 														 \
			break;																	 \
	}
#endif //#if defined(_MSC_VER)

#define REGISTER_INPUT_DEVICE(type, handler_ptr) if (false == m_input_device.insert(std::make_pair(type, handler_ptr)).second){	 ERROR_EX_LOG("[type = %s][handler_ptr = %s]", #type, #handler_ptr);	return false;}

#define REGISTER_RTC_INPUT_DEVICE(type, handler_ptr) if (false == m_rtc_input_device.insert(std::make_pair(type, handler_ptr)).second){	 ERROR_EX_LOG("[type = %s][handler_ptr = %s]", #type, #handler_ptr);	return false;}



//struct c_init_input_device
//{
//	c_init_input_device()
//	{
//		s_input_device.init();
//	}
//};

    //static   c_init_input_device input_device_init;
	static HWND g_main_mouse_down_up = NULL;
	static HWND g_child_mouse_down_up = NULL;
///	cinput_device   g_input_device_mgr;
	cinput_device::cinput_device() 
		: m_stoped(false)
		, m_input_device()
		,  m_int_point()
		, m_all_consumer()
		, m_mouse_id("")
		, m_input_list()
		, m_init(false)
#if defined(_MSC_VER)
		, m_main_win(NULL)
#endif //#if defined(_MSC_VER)
	{}
	cinput_device::~cinput_device() {}

#if defined(_MSC_VER)
	static void registerinputdevice(HWND hwnd)
	{
		static bool load = false;
		if (load)
		{
			return;
		}
		load = true;
		DWORD flags = RIDEV_REMOVE;


		// NOTE: Currently has to be created every time due to conflicts with Direct8 Input used by the wx unrealed
		RAWINPUTDEVICE RawInputDevice;

		//The HID standard for mouse
		const uint16 StandardMouse = 0x02;

		RawInputDevice.usUsagePage = 0x01;
		RawInputDevice.usUsage = StandardMouse;
		RawInputDevice.dwFlags = flags;

		// Process input for just the window that requested it.  NOTE: If we pass NULL here events are routed to the window with keyboard focus
		// which is not always known at the HWND level with Slate
		RawInputDevice.hwndTarget = hwnd;

		// Register the raw input device
		::RegisterRawInputDevices(&RawInputDevice, 1, sizeof(RAWINPUTDEVICE));
	}


#endif //#if defined(_MSC_VER)
	//static bool g_move_init = false;
	///////////////////////////////////////////////////////////////////////////////
	static HMODULE user32dll = NULL;
	typedef UINT(WINAPI* PFN_GetRawInputData)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
	PFN_GetRawInputData RealGetRawInputData = NULL;
	/// <summary>
	/// GetCursorPos
	/// </summary>

	typedef BOOL (WINAPI* PFN_GetCursorPos)( _Out_ LPPOINT lpPoint);
	PFN_GetCursorPos  RealGetCursorPos = NULL;

	typedef BOOL (WINAPI* PFN_SetCursorPos)( _In_ int X, _In_ int Y);
	PFN_SetCursorPos RealSetCursorPos = NULL;


	typedef  ATOM (WINAPI* PFN_RegisterClassA)( _In_ CONST WNDCLASSA* lpWndClass);
	PFN_RegisterClassA RealRegisterClassA;


	typedef ATOM (WINAPI* PFN_RegisterClassW)( _In_ CONST WNDCLASSW* lpWndClass);
	PFN_RegisterClassW  RealRegisterClassW;



	typedef BOOL (WINAPI* PFN_UnregisterClassA)(_In_ LPCSTR lpClassName,_In_opt_ HINSTANCE hInstance);
	PFN_UnregisterClassA RealUnregisterClassA;

	typedef BOOL (WINAPI* PFN_UnregisterClassW)(_In_ LPCWSTR lpClassName, _In_opt_ HINSTANCE hInstance);
	PFN_UnregisterClassW RealUnregisterClassW;


	typedef SHORT (WINAPI* PFN_GetKeyState)( _In_ int nVirtKey);
	PFN_GetKeyState RealGetKeyState;


	typedef SHORT (WINAPI* PFN_GetAsyncKeyState)( _In_ int vKey);
	PFN_GetAsyncKeyState  RealGetAsyncKeyState;

	typedef BOOL(WINAPI* PFN_EnumDisplaySettingsA)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
	PFN_EnumDisplaySettingsA RealEnumDisplaySettingsA;
	typedef BOOL(WINAPI* PFN_EnumDisplaySettingsW)(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode);
	PFN_EnumDisplaySettingsW RealEnumDisplaySettingsW;




	typedef   BOOL(WINAPI* PFN_RegisterHotKey)(
		_In_opt_ HWND hWnd,
		_In_ int id,
		_In_ UINT fsModifiers,
		_In_ UINT vk);
	PFN_RegisterHotKey  RealRegisterHotKey;



	typedef BOOL
		(WINAPI* PFN_GetMessageA)(
			_Out_ LPMSG lpMsg,
			_In_opt_ HWND hWnd,
			_In_ UINT wMsgFilterMin,
			_In_ UINT wMsgFilterMax);

	PFN_GetMessageA    RealGetMessageA;
	typedef   
		BOOL
		(WINAPI* 
			PFN_GetMessageW)(
			_Out_ LPMSG lpMsg,
			_In_opt_ HWND hWnd,
			_In_ UINT wMsgFilterMin,
			_In_ UINT wMsgFilterMax);
	PFN_GetMessageW    RealGetMessageW;

	///////////////////////////////////////////////////////////////////////////////
	 

	static  std::map<uint64, RAWINPUT> g_hrawinput;
	static const uint32 RAW_INPUT_SIZE = 300;
	static uint32     g_read_index = 0;
	static uint32	  g_write_index = 0;
	static  std::mutex   g_mutex;
	typedef std::lock_guard<std::mutex>		clock_guard;
	 
	static std::map<const WNDCLASSA*, const  WNDCLASSA*> g_wnd_classA;
	static std::map<const  WNDCLASSW*,const  WNDCLASSW*> g_wnd_classW;
	static SHORT    g_ctrl = 0;
	static SHORT    g_alt = 0;//  


	static uint64  g_hot_key = 0;
	
	static std::unordered_map<uint64, std::unordered_map<uint64, uint64>>       g_app_hot;

//#define MESSAGE(g_wnd, message_id, param1, param2)  for (std::map<const WNDCLASSA*, const WNDCLASSA*>::const_iterator iter = g_wnd_classA.begin(); iter != g_wnd_classA.end(); ++iter) \
//	{																																	\
//		if (iter->first && iter->first->lpfnWndProc)																					\
//		{																																\
//			iter->first->lpfnWndProc(g_wnd, message_id, param1, param2);																\
//		}																																\
//	}																																	\
//	for (std::map<const WNDCLASSW*, const WNDCLASSW*>::const_iterator iter = g_wnd_classW.begin(); iter != g_wnd_classW.end(); ++iter)	\
//	{																																	\
//		if (iter->first && iter->first->lpfnWndProc)																					\
//		{																																\
//			iter->first->lpfnWndProc(g_wnd, message_id, param1, param2);																\
//		}																																\
//	}

	static UINT WINAPI hook_get_raw_input_data(_In_ HRAWINPUT hRawInput, _In_ UINT uiCommand, _Out_writes_bytes_to_opt_(*pcbSize, return) LPVOID pData, _Inout_ PUINT pcbSize, _In_ UINT cbSizeHeader)
	{ 
		UINT ret = RealGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
		  
		{ 
			clock_guard lock(g_mutex);
			if (  uiCommand == RID_INPUT /*&& (ret != 48 && ret != 0) && g_hrawinput.size() > g_read_index*/ )
			{ 
				std::map<uint64, RAWINPUT>::iterator iter = g_hrawinput.find((uint64)hRawInput);
				if (iter != g_hrawinput.end())
				{
					DEBUG_LOG("[g_hrawinput.size() = %u][hRawInput = %u][uiCommand = %u][ret = %u][cbSizeHeader = %u]", g_hrawinput.size(), hRawInput, uiCommand, ret, cbSizeHeader);

					//DEBUG_LOG("find [hRawInput = %u]", hRawInput);
					ret = iter->second.header.dwSize;
					if (pcbSize)
					{
						*pcbSize = ret;
					}
					if (pData)
					{
						memcpy(pData, &iter->second, ret);
					}
					
				}
				else
				{
					DEBUG_LOG("not find hRawInput = %u", hRawInput);
				}  
			}
		}
		
		if (false)
		{
			RAWINPUT* temp_input = (RAWINPUT*)pData;
			DEBUG_LOG("[g_hrawinput.size() = %u][g_read_index = %u][uiCommand = %u][ret = %u][cbSizeHeader = %u]", g_hrawinput.size(), g_read_index, uiCommand, ret, cbSizeHeader);
			DEBUG_LOG("[temp_input->header.dwType = %u]", temp_input->header.dwType);
			DEBUG_LOG("[temp_input->data.mouse.lLastX = %u][temp_input->data.mouse.lLastY = %u]", temp_input->data.mouse.lLastX, temp_input->data.mouse.lLastY);
		}
		return ret;
	}


	static BOOL WINAPI hook_get_cursor_pos (_Out_ LPPOINT lpPoint)
	{
		if (lpPoint)
		{
			lpPoint->x = g_width;
			lpPoint->y = g_height;
			return 1;
		}
		return RealGetCursorPos(lpPoint);;
	}
	static BOOL WINAPI hook_set_cursor_pos(_In_ int X, _In_ int Y)
	{
		/*if (lpPoint)
		{
			lpPoint->x = g_width;
			lpPoint->y = g_height;
			return 1;
		}*/
		return RealSetCursorPos(X, Y);
	}

	static ATOM  hook_RegisterClassA(_In_ CONST WNDCLASSA* lpWndClass)
	{
		if (lpWndClass && lpWndClass->lpfnWndProc)
		{
			NORMAL_EX_LOG("add wnd [lpszClassName = %s][lpszMenuName = %s]", lpWndClass->lpszClassName, lpWndClass->lpszMenuName);
			g_wnd_classA[lpWndClass] = lpWndClass;
		}
		else if(lpWndClass)
		{
			NORMAL_EX_LOG("not add wnd [lpszClassName = %s][lpszMenuName = %s]", lpWndClass->lpszClassName, lpWndClass->lpszMenuName);
		}
		return RegisterClassA(lpWndClass);
	}


	static ATOM  hook_RegisterClassW(_In_ CONST WNDCLASSW* lpWndClass)
	{
		if (lpWndClass && lpWndClass->lpfnWndProc)
		{
			NORMAL_EX_LOG("add wnd [lpszClassName = %s][lpszMenuName = %s]", lpWndClass->lpszClassName, lpWndClass->lpszMenuName);
			g_wnd_classW[lpWndClass] = lpWndClass;
		}
		else if (lpWndClass)
		{
			NORMAL_EX_LOG("not add wnd [lpszClassName = %s][lpszMenuName = %s]", lpWndClass->lpszClassName, lpWndClass->lpszMenuName);
		}
		return RegisterClassW(lpWndClass);
	}


	static BOOL hook_UnregisterClassA(_In_ LPCSTR lpClassName, _In_opt_ HINSTANCE hInstance)
	{

		return UnregisterClassA(lpClassName, hInstance);
	}


	static BOOL hook_UnregisterClassW(_In_ LPCWSTR lpClassName, _In_opt_ HINSTANCE hInstance)
	{
		return UnregisterClassW(lpClassName, hInstance);
	}

	static BOOL hook_RegisterHotKey(
		_In_opt_ HWND hWnd,
		_In_ int id,
		_In_ UINT fsModifiers,
		_In_ UINT vk)
	{
		NORMAL_EX_LOG("");
		BOOL ret = RealRegisterHotKey(hWnd, id, fsModifiers, vk);;
			//g_app_hot
		if (ret)
		{
			NORMAL_EX_LOG("[id = %u][mod = %u][vk = %u]", id, fsModifiers, vk);
			g_app_hot[vk][fsModifiers] = id;
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
	static inline SHORT hook_GetKeyState(int key)
	{
		SHORT ret = RealGetKeyState(key);
		if ((key == 17 || key == 162)  )
		{
			//if (g_ctrl > 0)
			{
				ret = g_ctrl;
			}
			
		}
		if (key == VK_MENU || key == VK_LMENU || key == VK_RMENU)
		{
			ret = g_alt;
		}
		// NORMAL_EX_LOG("[hook_RealGetKeyState][][key = %u][ret = %u]", key,  static_cast<int>(ret));
		return ret;
	}
	static inline SHORT hook_GetAsyncKeyState(int  Key)
	{
		SHORT  ret = RealGetAsyncKeyState(Key);;
		 
		if ((Key == 17 || Key == 162)   )
		{
			//if (g_ctrl > 0)
			{
				ret = g_ctrl;
			}
			
		}
		if (Key == VK_MENU || Key == VK_LMENU || Key == VK_RMENU)
		{
			ret = g_alt;
		}
		
	 //	NORMAL_EX_LOG("[key = %u][ret = %u]", Key, ret);
		return ret;
	}

	std::string WcharTochar(const std::wstring& wp, size_t m_encode = CP_ACP)
	{
		std::string str;
		int32 len = WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), NULL, 0, NULL, NULL);
		str.resize(len);
		WideCharToMultiByte(m_encode, 0, wp.c_str(), wp.size(), (LPSTR)(str.data()), len, NULL, NULL);
		return str;
	}

	static BOOL hook_EnumDisplaySettingsA(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
	{
		NORMAL_EX_LOG("");
		BOOL ret = RealEnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode);
		if (ret)
		{
			//::GetCommandLineA();
			LPWSTR* szArglist;
			int nArgs;
			szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
			int32 width = 1920;
			int32 height = 1080;
			if (nArgs > 14)
			{
				// 0 0 1920 1040
				// 11 12 13 14
				width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
				height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
				NORMAL_EX_LOG("width  = %u, height = %u", width, height);
				lpDevMode->dmPelsWidth = width;
				lpDevMode->dmPelsHeight = height;
			}
			
		}
		return ret;
	}
	static BOOL hook_EnumDisplaySettingsW(_In_opt_ LPCSTR lpszDeviceName, _In_ DWORD iModeNum, _Inout_ DEVMODEA* lpDevMode)
	{
		NORMAL_EX_LOG("");
		BOOL ret = RealEnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
		if (ret)
		{
			LPWSTR* szArglist;
			int nArgs;
			szArglist = ::CommandLineToArgvW(GetCommandLineW(), &nArgs);
			int32 width = 1920;
			int32 height = 1080;

			if (nArgs > 14)
			{
				// 0 0 1920 1040
				// 11 12 13 14
				width = ::atoi((const char*)(WcharTochar(szArglist[13]).c_str()));
				height = ::atoi((const char*)(WcharTochar(szArglist[14]).c_str()));
				NORMAL_EX_LOG("width  = %u, height = %u", width, height);
				lpDevMode->dmPelsWidth = width;
				lpDevMode->dmPelsHeight = height;
			}
			
		}
		return ret;
	}

	static  BOOL hook_GetMessageA(
		_Out_ LPMSG lpMsg,
		_In_opt_ HWND hWnd,
		_In_ UINT wMsgFilterMin,
		_In_ UINT wMsgFilterMax)
	{

		NORMAL_EX_LOG("");
		if (g_hot_key != 0 && lpMsg && !hWnd)
		{
			lpMsg->message = WM_HOTKEY;
			lpMsg->wParam = g_hot_key;
			g_hot_key = 0;
			return true;
		}
		return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	static  BOOL hook_GetMessageW(
		_Out_ LPMSG lpMsg,
		_In_opt_ HWND hWnd,
		_In_ UINT wMsgFilterMin,
		_In_ UINT wMsgFilterMax)
	{
		NORMAL_EX_LOG("");
		if (g_hot_key != 0 && lpMsg && !hWnd)
		{
			lpMsg->message = WM_HOTKEY;
			lpMsg->wParam = g_hot_key;
			g_hot_key = 0;
			return true;
		}
		return RealGetMessageW(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	//static inline 

	bool cinput_device::init()
	{
		if (m_init)
		{
			return true;

		}
		m_init = true;
		//g_hrawinput.resize(RAW_INPUT_SIZE);
		 // win
		char system_path[MAX_PATH] = {0};
		 
		UINT ret = GetSystemDirectoryA(system_path, MAX_PATH);
		if (!ret)
		{
			ERROR_EX_LOG("Failed to get windows system path: %lu\n", GetLastError());
			//return false;
		}
		SYSTEM_LOG("[system_path = %s] ok ", system_path);
		// hook mouse move  GetRawInputData
		user32dll = get_system_module(system_path, "user32.dll");
		if (!user32dll)
		{
			WARNING_EX_LOG(" REGISTER mouse  failed !!!");
			//return false;
		}
		SYSTEM_LOG("REGISTER  mouse ok !!!");
		void* get_raw_input_data_proc = GetProcAddress(user32dll, "GetRawInputData");
		void* get_cursor_pos_proc = GetProcAddress(user32dll, "GetCursorPos");
		void* set_cursor_pos_proc = GetProcAddress(user32dll, "SetCursorPos");
		void * get_key_state_proc = GetProcAddress(user32dll, "GetKeyState"); //hook_RealGetKeyState
		void* get_async_key_state_proc = GetProcAddress(user32dll, "GetAsyncKeyState");
		void* get_RealRegisterHotKey = GetProcAddress(user32dll, "RegisterHotKey");
		void* get_RealGetMessageW = GetProcAddress(user32dll, "GetMessageW");
		void* get_RealGetMessageA = GetProcAddress(user32dll, "GetMessageA");
		 

		//void* EnumDisplaySettingsA_proc = GetProcAddress(user32dll, "EnumDisplaySettingsA");
		//void* EnumDisplaySettingsW_proc = GetProcAddress(user32dll, "EnumDisplaySettingsW");
		/// <summary>
		/// ///////////////////
		/// </summary>
		/// <returns></returns>
		/*void* register_class_a_proc = GetProcAddress(user32dll, "RegisterClassA");
		void* register_class_w_proc = GetProcAddress(user32dll, "RegisterClassW");*/


		/*if (!get_raw_input_data_proc)
		{
			ERROR_EX_LOG("seatch mouse table not find GetRawInputData !!!");
		}
		else*/
		{
			SYSTEM_LOG("    input device  begin ... ");
			DetourTransactionBegin();

			
			if (get_raw_input_data_proc)
			{
				RealGetRawInputData = (PFN_GetRawInputData)get_raw_input_data_proc;
				DetourAttach((PVOID*)&RealGetRawInputData,
					hook_get_raw_input_data);
			}
			if (get_cursor_pos_proc)
			{
				RealGetCursorPos = (PFN_GetCursorPos)get_cursor_pos_proc;
				DetourAttach((PVOID*)&RealGetCursorPos,
					hook_get_cursor_pos);
			}
			if (set_cursor_pos_proc)
			{
				RealSetCursorPos = (PFN_SetCursorPos)set_cursor_pos_proc;
				DetourAttach((PVOID*)&RealSetCursorPos,
					hook_set_cursor_pos);
			}
			if (get_async_key_state_proc)
			{
				RealGetAsyncKeyState = (PFN_GetAsyncKeyState)get_async_key_state_proc;
				DetourAttach((PVOID*)&RealGetAsyncKeyState,
					hook_GetAsyncKeyState );
			}
			if (get_key_state_proc)
			{
				RealGetKeyState = (PFN_GetKeyState)get_key_state_proc;
				DetourAttach((PVOID*)&RealGetKeyState,
					hook_GetKeyState);
			}

			if (get_RealRegisterHotKey)
			{
				RealRegisterHotKey = (PFN_RegisterHotKey)get_RealRegisterHotKey;
				DetourAttach((PVOID*)&RealRegisterHotKey,
					hook_RegisterHotKey);
			}

			if (get_RealGetMessageA)
			{
				RealGetMessageA = (PFN_GetMessageA)get_RealGetMessageA;
				DetourAttach((PVOID*)&RealGetMessageA,
					hook_GetMessageA);
			}

			if (get_RealGetMessageW)
			{
				RealGetMessageW = (PFN_GetMessageW)get_RealGetMessageW;
				DetourAttach((PVOID*)&RealGetMessageW,
					hook_GetMessageW);
			}

			 /*if (EnumDisplaySettingsA_proc)
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
			} */
			/*if (register_class_a_proc)
			{
				RealRegisterClassA = (PFN_RegisterClassA)register_class_a_proc;
				DetourAttach((PVOID*)&RealRegisterClassA,
					hook_RegisterClassA);
			}

			if (register_class_w_proc)
			{
				RealRegisterClassW = (PFN_RegisterClassW)register_class_w_proc;
				DetourAttach((PVOID*)&RealRegisterClassW,
					hook_RegisterClassW);
			}*/

			SYSTEM_LOG("   input end  ... ");
			const LONG error = DetourTransactionCommit();
			const bool success = error == NO_ERROR;
			if (success)
			{
				NORMAL_EX_LOG("  input device");
				if (get_raw_input_data_proc)
				{
					NORMAL_EX_LOG("  input device");
				}
				if (get_cursor_pos_proc)
				{
					NORMAL_EX_LOG("  input device");
				}
				if (set_cursor_pos_proc)
				{
					NORMAL_EX_LOG("  input device");
				}
				if (get_async_key_state_proc)
				{
					NORMAL_EX_LOG("input key ");
				}
				if (get_key_state_proc)
				{
					NORMAL_EX_LOG("input key state");
				}
				/*if (register_class_a_proc)
				{
					NORMAL_EX_LOG(" input device");
				}
				if (register_class_w_proc)
				{
					NORMAL_EX_LOG(" input device");
				}*/
				NORMAL_EX_LOG("  input device ");
			}
			else
			{
				RealGetRawInputData = NULL;
				RealGetCursorPos = NULL;
				RealSetCursorPos = NULL;
				RealGetAsyncKeyState = NULL;
				/*RealRegisterClassA = NULL;
				RealRegisterClassW = NULL;*/
				ERROR_EX_LOG("Failed to attach  mouse  : %ld", error);
			}
		}

		

		REGISTER_INPUT_DEVICE(RequestQualityControl, &cinput_device::OnKeyChar);
		REGISTER_INPUT_DEVICE(KeyDown, &cinput_device::OnKeyDown);
		REGISTER_INPUT_DEVICE(KeyUp, &cinput_device::OnKeyUp);
		REGISTER_INPUT_DEVICE(KeyPress, &cinput_device::OnKeyPress);
		////////////////////////////////////////////////////////////
		REGISTER_INPUT_DEVICE(MouseEnter, &cinput_device::OnMouseEnter);
		REGISTER_INPUT_DEVICE(MouseLeave, &cinput_device::OnMouseLeave);
		REGISTER_INPUT_DEVICE(MouseDown, &cinput_device::OnMouseDown);
		REGISTER_INPUT_DEVICE(MouseUp, &cinput_device::OnMouseUp);
		REGISTER_INPUT_DEVICE(MouseMove, &cinput_device::OnMouseMove);
		REGISTER_INPUT_DEVICE(MouseWheel, &cinput_device::OnMouseWheel);
		REGISTER_INPUT_DEVICE(MouseDoubleClick, &cinput_device::OnMouseDoubleClick);



		///////////////////
		REGISTER_RTC_INPUT_DEVICE(RequestQualityControl, &cinput_device::OnRtcKeyChar);
		REGISTER_RTC_INPUT_DEVICE(KeyDown, &cinput_device::OnRtcKeyDown);
		REGISTER_RTC_INPUT_DEVICE(KeyUp, &cinput_device::OnRtcKeyUp);
		REGISTER_RTC_INPUT_DEVICE(KeyPress, &cinput_device::OnRtcKeyPress);
		REGISTER_RTC_INPUT_DEVICE(MouseEnter, &cinput_device::OnRtcMouseEnter);
		REGISTER_RTC_INPUT_DEVICE(MouseLeave, &cinput_device::OnRtcMouseLeave);
		REGISTER_RTC_INPUT_DEVICE(MouseDown, &cinput_device::OnRtcMouseDown);
		REGISTER_RTC_INPUT_DEVICE(MouseUp, &cinput_device::OnRtcMouseUp);
		REGISTER_RTC_INPUT_DEVICE(MouseMove, &cinput_device::OnRtcMouseMove);
		REGISTER_RTC_INPUT_DEVICE(MouseWheel, &cinput_device::OnRtcMouseWheel);
		REGISTER_RTC_INPUT_DEVICE(MouseDoubleClick, &cinput_device::OnRtcMouseDoubleClick);


		std::thread td(&cinput_device::_work_pthread, this);
		m_thread.swap(td);
		return true;
	}
	void cinput_device::Destroy()
	{
		m_stoped = true;
		if (m_thread.joinable())
		{
			m_thread.join();
		}
		m_input_device.clear();
		m_rtc_input_device.clear();
		m_all_consumer.clear();
		
		{
			std::lock_guard<std::mutex>  lock(m_input_mutex);
			m_input_device.clear();
		}
	}
	void cinput_device::startup()
	{
		
	}
	bool cinput_device::set_point(uint32 x, uint32 y)
	{
		m_int_point.X = x;
		m_int_point.Y = y;
		return true;
	}
	bool cinput_device::OnMessage(const std::string & consumer_id, const webrtc::DataBuffer& Buffer)
	{
		//NORMAL_LOG("consumer_id = %s", consumer_id.c_str());
		m_mouse_id = consumer_id;
		const uint8* Data = Buffer.data.data();
		uint32 Size = static_cast<uint32>(Buffer.data.size());

		GET(EToStreamMsg, MsgType);
		
		M_INPUT_DEVICE_MAP::iterator iter =  m_input_device.find(MsgType);
		if (iter == m_input_device.end())
		{
			//RTC_LOG(LS_ERROR) << "input_device not find id = " << MsgType;
			//log --->  not find type 
			ERROR_EX_LOG("input_device msg_type = %d not find failed !!!", MsgType);
			return false;
		}
		
		/*if (MsgType != MouseEnter)
		{
			std::map<std::string, std::map<uint32, cmouse_info>>::const_iterator consumer_iter = m_all_consumer.find(consumer_id);
			if (consumer_iter == m_all_consumer.end())
			{
				WARNING_EX_LOG("consumer_id = %s not Enter !!!", consumer_id.c_str());
				return false;
			}

		} */
		//NORMAL_EX_LOG("move type =%d", MsgType);
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration dur;
		std::chrono::microseconds microseconds;
		uint32_t elapse = 0;
		//备往：尽管GetFocus返回NULL，但可能另一线程的队列与拥有输入焦点的窗口相关。
		//便用GetForeyroundWindow函数来获得用户目前工作的窗口。
		//可以使用AttachThreadlnPut函数把线程的消息队列与另一线程的窗口关联起来。
		

		//HWND wnd = GetFocus();





		
		   //说明：SetForegroundWindow在debug模式一直成功，非debug模式会有失败的情况，解决方法是利用AttachThreadInput
        //AttachThreadInput这个函数可以使两个线程的输入队列共享。 
        //如果我们把当前的焦点的输入队列跟我们要显示的窗口的输入队列共享，
        //我们就可以让我们的窗口SetForegroundWindow 成功，然后成功 获得焦点，显示在前台
		/*
		HWND hForgroundWnd = GetForegroundWindow(); \
						DWORD dwForeID = ::GetWindowThreadProcessId(hForgroundWnd, NULL); \
	DWORD dwCurID = ::GetCurrentThreadId();		\
	::AttachThreadInput(dwCurID, dwForeID, TRUE);	\
	::ShowWindow(g_wnd, SW_SHOWNORMAL);		\
	::SetForegroundWindow(g_wnd);	\
	::AttachThreadInput(dwCurID, dwForeID, FALSE);	\
		*/
       /*
	   SetWindowPos(handle, -1, 0, 0, 0, 0, 0x001 | 0x002 | 0x040);    //将这个窗口永远置于最上面
    //---------------------------在需要激活窗体的地方添加如下代码-----------------------//
    PostMessage(handle, WM_LBUTTONDOWN, 0, 0);
    PostMessage(handle, WM_LBUTTONUP, 0, 0);
	   */
		//BringWindowToTop();

		 (this->*(iter->second))(Data, Size);
		 cur_time_ms = std::chrono::steady_clock::now();
		 dur = cur_time_ms - pre_time;
		 microseconds = std::chrono::duration_cast<std::chrono::microseconds>(dur);
		 elapse = static_cast<uint32_t>(microseconds.count());
		 if (elapse > 900)
		 {
			 WARNING_EX_LOG("input_device  microseconds = %lu", microseconds);
		 }
		 return true;
		//return true;
	}

	bool cinput_device::OnMessage(const nlohmann::json & datachannel)
	{
		 
		/*const uint8 * Data = (const uint8*)(datachannel.c_str());
		uint32 Size = datachannel.size();*/

		//std::string hex = hexmem((const void *)Data, (size_t)Size);
		//NORMAL_EX_LOG("hex = %s", hex.c_str);
		//GET(EToStreamMsg, MsgType);

		 
		if (datachannel.find("type") == datachannel.end())
		{
			WARNING_EX_LOG("not find msg_type [data = %s]", datachannel.dump().c_str());
			return false;
		}
		if (!datachannel["type"].is_number() )
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", datachannel.dump().c_str());
			return false;
		}
		EToStreamMsg msg_type = datachannel["type"].get<EToStreamMsg>();
		M_RTC_INPUT_DEVICE_MAP::iterator iter = m_rtc_input_device.find(msg_type);
		if (iter == m_rtc_input_device.end())
		{
			//RTC_LOG(LS_ERROR) << "input_device not find id = " << MsgType;
			//log --->  not find type 
			ERROR_EX_LOG("rtc input_device msg_type = %d not find failed !!!", msg_type);
			return false;
		}

		 
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration dur;
		std::chrono::microseconds microseconds;
		uint32_t elapse = 0;
		 

		(this->*(iter->second))(datachannel);
		cur_time_ms = std::chrono::steady_clock::now();
		dur = cur_time_ms - pre_time;
		microseconds = std::chrono::duration_cast<std::chrono::microseconds>(dur);
		elapse = static_cast<uint32_t>(microseconds.count());
		if (elapse > 900)
		{
			WARNING_EX_LOG("input_device  microseconds = %lu", microseconds);
		}
		return true;
		return true;
	}

	void cinput_device::insert_message(const webrtc::DataBuffer & Buffer)
	{
		{
			std::lock_guard<std::mutex> lock(m_input_mutex);
			m_input_list.push_back(Buffer);
		}
	}


	bool cinput_device::OnRequestQualityControl(const uint8*& Data, uint32 size)
	{
		// 这边需要启动引擎推送视频流
		return true;
	}
	/**
	* 输入字符
	*/
	bool cinput_device::OnKeyChar(const uint8*& Data,   uint32 size)
	{
		NORMAL_LOG("KeyChar");
		//WINDOW_MAIN();
		//SET_POINT(vec);
		//WINDOW_CHILD();
		//if (childwin)
		//{
		//	::PostMessageW(childwin, WM_CHAR, Character, 1);
		//}
		//else if (mwin)
		//{
		//	::PostMessageW(mwin, WM_CHAR, Character, 1);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}

		return true;
	}
	bool cinput_device::OnRtcKeyChar(const nlohmann::json & data)
	{
		return false;
	}
	/**
	* 按下键
	*/
	bool cinput_device::OnKeyDown(const uint8*& Data,  uint32 Size)
	{
		 
		GET(FKeyCodeType, KeyCode);
		GET(FRepeatType, Repeat);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("key down: %d, repeat: %d"), KeyCode, Repeat);
		// log key down -> repeat keyCode Repeat 
		FEvent KeyDownEvent(EventType::KEY_DOWN);
		KeyDownEvent.SetKeyDown(KeyCode, Repeat != 0);
		NORMAL_LOG("OnKeyDown==KeyCode = %u, Repeat = %u", KeyCode, Repeat);
		#if defined(_MSC_VER)
		// TODO@chensong 20240509  win11电脑按下win键 只发送一个键盘事件
		if (KeyCode == 91)
		{
			return true;
		}
		if (KeyCode < 58 && KeyCode > 47 && !g_ctrl)
		{
			return true;
		}
		if (KeyCode < 106 && KeyCode > 95 && !g_ctrl)
		{
			return true;
		}

		/*if (KeyCode == 17 && g_ctrl != 0)
		{
			return true;
		}*/
		WINDOW_MAIN();
		// TODO@chensong 2022-01-20  keydown -> keycode -> repeat 
		/*if (mwin)
		{
			::PostMessageW(mwin, WM_KEYDOWN, KeyCode, Repeat != 0);
		}*/
		
		
		SET_POINT();
		WINDOW_CHILD();
		if (VK_CONTROL == KeyCode)
		{
			g_ctrl = 4294967168;
			       //4294934529;
		}
	 	if (VK_MENU == KeyCode)
		{
			g_alt = 4294934529;
		} 
		/*else if (32 == KeyCode)
		{
			g_ctrl = 0;
		}*/
		if (childwin)
		{
			//PostMessageW
	        /*
			这个 KeyDown与 KeyUP 是有讲究哈 ^_^
			发送1次按键结果出现2次按键的情况
发送一次WM_KEYDOWN及一次WM_KEYUP结果出现了2次按键，原因是最后一个参数lParam不规范导致，
此参数0到15位为该键在键盘上的重复次数，经常设为1，即按键1次；
16至23位为键盘的扫描码，通过API函数MapVirtualKey可以得到；24位为扩展键，
即某些右ALT和CTRL；29一般为0；30位-[原状态]已按下为1否则0（KEYUP要设为1）；31位-[状态切换]（KEYDOWN设为0，KEYUP要设为1）。


资料显示第30位对于keydown在和shift等结合的时候通常要设置为1，未经验证。


			*/
			
			//{//MAKELPARAM(PosX, PosY)
			//	keybd_event(VK_CONTROL, 0, 0, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(childwin);
			//	MESSAGE(childwin, WM_KEYDOWN, KeyCode, 3224961025/*Repeat*/);
				 if (KeyCode != 18)
				{

					MESSAGE(childwin, WM_KEYDOWN, KeyCode, 3224961025);
				}
				 else
				{
					MESSAGE(childwin, WM_SYSKEYDOWN, KeyCode, 1614282753);
				} 
				  
				 // g_app_hot[vk][fsModifiers] = id;
				 //if (g_app_hot[KeyCode][MOD_CONTROL] != 0)
				 if (VK_F1 == KeyCode  && g_ctrl)
				 {
					 NORMAL_EX_LOG("---> mod");
					 MESSAGE(NULL, WM_HOTKEY, 10001, 10001);
					 g_hot_key = 10001;
					 MESSAGE(NULL, WM_HOTKEY, g_hot_key, g_hot_key);
				 }
				 else if (VK_F2 == KeyCode && g_ctrl)
				 {
					 NORMAL_EX_LOG("---> mod");
					 MESSAGE(NULL, WM_HOTKEY, 10002, 10002);
					 g_hot_key = 10002;
					 MESSAGE(NULL, WM_HOTKEY, g_hot_key, g_hot_key);
				 }

			}//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, 0, 0);
			//	SendInput();
			//}
			
		}
		else  if ( mwin   )
		{
			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, 0, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(mwin);
				if (KeyCode != 18)
				{

					MESSAGE(mwin, WM_KEYDOWN, KeyCode, 3224961025);
				}
				else
				{
					MESSAGE(mwin, WM_SYSKEYDOWN, KeyCode, 1614282753);
				}
				/*if (g_app_hot[KeyCode][MOD_CONTROL] != 0)
				{
					 
					MESSAGE(NULL, WM_HOTKEY, g_app_hot[KeyCode][MOD_CONTROL], g_app_hot[KeyCode][MOD_CONTROL]);
				}*/
				if (VK_F1 == KeyCode && g_ctrl)
				{
					NORMAL_EX_LOG("---> mod");
					MESSAGE(NULL, WM_HOTKEY, 10001, 10001);
					g_hot_key = 10001;
					MESSAGE(NULL, WM_HOTKEY, g_hot_key, g_hot_key);
					
				}
				else if (VK_F2 == KeyCode && g_ctrl)
				{
					NORMAL_EX_LOG("---> mod");
					MESSAGE(NULL, WM_HOTKEY, 10002, 10002);
					g_hot_key = 10002;
					MESSAGE(NULL, WM_HOTKEY, g_hot_key, g_hot_key);
				}
			}
			//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, 0, 0);
			//}
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
		#endif //#if defined(_MSC_VER)
		//KeyDownEvent.GetKeyDown();
		//ProcessEvent(KeyDownEvent);
		//WINDOW_MAIN();
		//SET_POINT(vec);
		//WINDOW_CHILD();
		//if (childwin)
		//{
		//	::PostMessageW(childwin, WM_KEYDOWN, KeyCode, 1);
		//}
		//else if (mwin)
		//{
		//	::PostMessageW(mwin, WM_KEYDOWN, KeyCode, 1);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}
		return true;
	}

	bool cinput_device::OnRtcKeyDown(const nlohmann::json & data)
	{
		//GET(FKeyCodeType, KeyCode);
		//GET(FRepeatType, Repeat);
		 
		if (data.find("keyCode") == data.end() || data.find("repeat") == data.end())
		{
			WARNING_EX_LOG("not find 'keycode' or 'repeat' [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["keyCode"].is_number() || !data["repeat"].is_number())
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FKeyCodeType KeyCode = data["keyCode"].get<FKeyCodeType>();
		FRepeatType  Repeat = data["repeat"].get<FKeyCodeType>();
		//checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("key down: %d, repeat: %d"), KeyCode, Repeat);
		// log key down -> repeat keyCode Repeat 
		FEvent KeyDownEvent(EventType::KEY_DOWN);
		KeyDownEvent.SetKeyDown(KeyCode, Repeat != 0);
		NORMAL_LOG("OnKeyDown==KeyCode = %u, Repeat = %u", KeyCode, Repeat);
#if defined(_MSC_VER)
		if (KeyCode < 58 && KeyCode > 47 && !g_ctrl)
		{
			return true;
		}
		if (KeyCode < 105 && KeyCode > 96 && !g_ctrl)
		{
			return true;
		}
		WINDOW_MAIN();
		// TODO@chensong 2022-01-20  keydown -> keycode -> repeat 
		/*if (mwin)
		{
			::PostMessageW(mwin, WM_KEYDOWN, KeyCode, Repeat != 0);
		}*/


		SET_POINT();
		WINDOW_CHILD();
		if (VK_CONTROL == KeyCode)
		{
			g_ctrl = 4294967168;
		}
		 if (VK_MENU == KeyCode)
		{
			g_alt = 4294934529;
		} 
		/*else if (32 == KeyCode)
		{
			g_ctrl = 0;
		}*/
		if (childwin)
		{
			//PostMessageW
			/*
			这个 KeyDown与 KeyUP 是有讲究哈 ^_^
			发送1次按键结果出现2次按键的情况
发送一次WM_KEYDOWN及一次WM_KEYUP结果出现了2次按键，原因是最后一个参数lParam不规范导致，此参数0到15位为该键在键盘上的重复次数，经常设为1，即按键1次；16至23位为键盘的扫描码，通过API函数MapVirtualKey可以得到；24位为扩展键，即某些右ALT和CTRL；29一般为0；30位-[原状态]已按下为1否则0（KEYUP要设为1）；31位-[状态切换]（KEYDOWN设为0，KEYUP要设为1）。


资料显示第30位对于keydown在和shift等结合的时候通常要设置为1，未经验证。


			*/

			//{//MAKELPARAM(PosX, PosY)
			//	keybd_event(VK_CONTROL, 0, 0, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(childwin);
				MESSAGE(childwin, WM_KEYDOWN, KeyCode, 0);
			}//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, 0, 0);
			//	SendInput();
			//}

		}
		else  if (mwin)
		{
			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, 0, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(mwin);
				MESSAGE(mwin, WM_KEYDOWN, KeyCode, 0);
			}
			//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, 0, 0);
			//}
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
#endif //#if defined(_MSC_VER)
		 
		return true;
	}

	/**
	* 松开键
	*/
	bool cinput_device::OnKeyUp(const uint8*& Data,   uint32 Size)
	{
		GET(FKeyCodeType, KeyCode);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("key up: %d"), KeyCode);
		// log key up -> KeyCode
		FEvent KeyUpEvent(EventType::KEY_UP);
		KeyUpEvent.SetKeyUp(KeyCode);
		NORMAL_LOG("OnKeyUp==KeyCode = %u", KeyCode);
		#if defined(_MSC_VER)
		if (KeyCode == 91)
		{
			return true;
		}
		if (KeyCode < 58 && KeyCode > 47 && !g_ctrl)
		{
			return true;
		}
		if (KeyCode < 106 && KeyCode > 95 && !g_ctrl)
		{
			return true;
		}
		/*if (KeyCode == 17 && g_ctrl == 0)
		{
			return true;
		}*/
		WINDOW_MAIN();
		
		
		SET_POINT();
		WINDOW_CHILD();
		
		if (childwin)
		{
			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(childwin);
				if (KeyCode != 18)
				{

					MESSAGE(childwin, WM_KEYUP, KeyCode, 3224961025);
				}
				else
				{
					MESSAGE(childwin, WM_SYSKEYUP, KeyCode, 0);
				}
			}//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, KEYEVENTF_KEYUP, 0);
			//}
		}
		else  if (mwin)
		{
			 
			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(mwin);
				 if (KeyCode != 18)
				{

					MESSAGE(mwin, WM_KEYUP, KeyCode, 3224961025);
				}
				  else
				{
					MESSAGE(mwin, WM_SYSKEYUP, KeyCode, 0);
				}  
			}
			
			//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, KEYEVENTF_KEYUP, 0);
			//}
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
		if (VK_CONTROL == KeyCode)
		{
			g_ctrl = 0;
		}
		if (VK_MENU == KeyCode)
		{
			g_alt = 0;
		}
		/*else if (32 == KeyCode)
		{
			g_ctrl = 4294967168;
		}*/
		#endif //#if defined(_MSC_VER)
		//ProcessEvent(KeyUpEvent);
		//WINDOW_MAIN();
		//SET_POINT(vec);
		//WINDOW_CHILD();
		//if (childwin)
		//{
		//	::PostMessageW(childwin, WM_KEYUP, KeyCode, 1);
		//}
		//else if (mwin)
		//{
		//	::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}
		return true;
	}
	bool cinput_device::OnRtcKeyUp(const nlohmann::json & data)
	{

		if (data.find("keyCode") == data.end() || data.find("repeat") == data.end())
		{
			WARNING_EX_LOG("not find 'keycode' or 'repeat' [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["keyCode"].is_number() || !data["repeat"].is_number())
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FKeyCodeType KeyCode = data["keyCode"].get<FKeyCodeType>();
		FRepeatType  Repeat = data["repeat"].get<FKeyCodeType>();
		FEvent KeyUpEvent(EventType::KEY_UP);
		KeyUpEvent.SetKeyUp(KeyCode);
		NORMAL_LOG("OnKeyUp==KeyCode = %u", KeyCode);
#if defined(_MSC_VER)
		if (KeyCode < 58 && KeyCode > 47 && !g_ctrl)
		{
			return true;
		}
		if (KeyCode < 105 && KeyCode > 96 && !g_ctrl)
		{
			return true;
		}
		WINDOW_MAIN();


		SET_POINT();
		WINDOW_CHILD();

		if (childwin)
		{
			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(childwin);
				MESSAGE(childwin, WM_KEYUP, KeyCode, 3224961025);
			}//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, KEYEVENTF_KEYUP, 0);
			//}
		}
		else  if (mwin)
		{

			//if (VK_CONTROL == KeyCode)
			//{
			//	keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
			//	//MESSAGE(mwin, WM_KEYUP, KeyCode, 1);
			//}
			//else
			{
				MOUSE_INPUT(childwin);
				MESSAGE(childwin, WM_KEYUP, KeyCode, 3224961025);
			}

			//if (KeyCode == 17 || KeyCode == 77 || KeyCode == 109)
			//{
			//	//keybd_event(16, 0, 0, 0);//按下Shift键
			//	//keybd_event('A', 0, 0, 0);//按下a键
			//	//keybd_event('A', 0, KEYEVENTF_KEYUP, 0);//松开a键
			//	//keybd_event(16, 0, KEYEVENTF_KEYUP, 0);//松开Shift键
			//	keybd_event(KeyCode, 0, KEYEVENTF_KEYUP, 0);
			//}
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
		if (VK_CONTROL == KeyCode)
		{
			g_ctrl = 0;
		}
		if (VK_MENU == KeyCode)
		{
			g_alt = 0;
		}
		/*else if (32 == KeyCode)
		{
			g_ctrl = 4294967168;
		}*/
#endif //#if defined(_MSC_VER)
	 
		return true;
	}
	/**
	*keydown：按下键盘键
	*	keypress：紧接着keydown事件触发（只有按下字符键时触发）
	*	keyup：释放键盘键
	*/
	bool cinput_device::OnKeyPress(const uint8*& Data,   uint32 Size)
	{
		// TODO@chensong 2022-01-20  KeyPress -->>>>> net

		GET(FCharacterType, Character);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("key up: %d"), KeyCode);
		// log key up -> KeyCode
		FEvent KeyUpEvent(EventType::KEY_PRESS);
		KeyUpEvent.SetKeyUp(Character);
		NORMAL_LOG("OnKeyPress==KeyCode = %u", Character);
		if (Character == 10 && g_ctrl != 0)
		{
			Character = 109;
		}
#if defined(_MSC_VER)
		if (Character == 91)
		{
			return true;
		}
		/*if (VK_MENU == Character)
		{
			g_alt = 4294934528;
		}*/
		
		 if (Character == VK_MENU || Character == VK_CONTROL)
		{
			return true;
		} 
		WINDOW_MAIN();


		
		SET_POINT();
		WINDOW_CHILD();
		if (childwin)
		{
			MOUSE_INPUT(childwin);
			MESSAGE(childwin, WM_CHAR, Character, 3224961025);
		}
		else if (mwin)
		{
			MOUSE_INPUT(mwin);
			//MESSAGE(mwin, WM_PR, Character, 0);
			MESSAGE(mwin, WM_CHAR, Character, 3224961025);
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
#endif //#if defined(_MSC_VER)
		//ProcessEvent(KeyUpEvent);
		//WINDOW_MAIN();
		//SET_POINT(vec);
		//WINDOW_CHILD();
		//if (childwin)
		//{
		//	::PostMessageW(childwin, WM_KEYUP, KeyCode, 1);
		//}
		//else if (mwin)
		//{
		//	::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}
		return true;
	}

	bool cinput_device::OnRtcKeyPress(const nlohmann::json & data)
	{
		if (data.find("keyCode") == data.end() )
		{
			WARNING_EX_LOG("not find 'keycode'  [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["keyCode"].is_number()  )
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FCharacterType Character = data["keyCode"].get<FCharacterType>();

		FEvent KeyUpEvent(EventType::KEY_PRESS);
		KeyUpEvent.SetKeyUp(Character);
		NORMAL_LOG("OnKeyPress==KeyCode = %u", Character);
#if defined(_MSC_VER)
		WINDOW_MAIN();



		SET_POINT();
		WINDOW_CHILD();
		if (childwin)
		{
			MOUSE_INPUT(childwin);
			MESSAGE(childwin, WM_CHAR, Character, 1);
		}
		else if (mwin)
		{
			MOUSE_INPUT(mwin);
			//MESSAGE(mwin, WM_PR, Character, 0);
			MESSAGE(mwin, WM_CHAR, Character, 1);
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
#endif //#if defined(_MSC_VER)
		return false;
	}

	bool cinput_device::OnMouseEnter(const uint8*& Data,   uint32 size)
	{
		// TODO@chensong 2022-01-20  OnMouseEnter -->>>>> net 
		/*if (!m_all_consumer.insert(std::make_pair(m_mouse_id, std::map<uint32, cmouse_info>())).second)
		{
			WARNING_EX_LOG("mouse enter insert [mouse_id = %s] failed !!!", m_mouse_id.c_str());
			return false;
		}*/
		NORMAL_EX_LOG("OnMouseEnter===>");
 #if defined(_MSC_VER)
		 
		//WINDOW_MAIN();
		//

		// 
		//if (mwin)
		//{
		//	TRACKMOUSEEVENT tme;
		//	tme.cbSize = sizeof(tme);	//结构体缓冲区大小
		//	tme.dwFlags = TME_HOVER;	//注册WM_MOUSEHOVER消息
		//	tme.dwHoverTime = 1; //WM_MOUSEHOVER消息触发间隔时间
		//	tme.hwndTrack = mwin; //当前窗口句柄
		//	::TrackMouseEvent(&tme); //注册发送消息
		//	//MESSAGE(mwin, WM_MOUSEHOVER, 0, 0);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}
 
#endif // _MSC_VER


		return true;
		 
	}

	bool cinput_device::OnRtcMouseEnter(const nlohmann::json & data)
	{
		return false;
	}

	/** 
	*鼠标离开控制范围？
	*/
	bool cinput_device::OnMouseLeave(const uint8*& Data,   uint32 size)
	{
		// TODO@chensong 2022-01-20  OnMouseLeave 
		std::map<std::string, std::map<uint32, cmouse_info>>::iterator iter =  m_all_consumer.find(m_mouse_id);
		/*if (iter == m_all_consumer.end())
		{
			WARNING_EX_LOG("mouse leave  not find  [mouse_id = %s] failed !!!", m_mouse_id.c_str());
			return false;
		}
		iter->second.clear();
		m_all_consumer.erase(iter);*/

		//std::map<uint32, cmouse_info>& cmouse_info = iter->second;
		NORMAL_EX_LOG("OnMouseLeave===>");
#if defined(_MSC_VER)

		//WINDOW_MAIN();
		// 
		//if (mwin)
		//{
		//	TRACKMOUSEEVENT tme;
		//	tme.cbSize = sizeof(tme);	//结构体缓冲区大小
		//	tme.dwFlags = TME_LEAVE;	//注册WM_MOUSEHOVER消息
		//	tme.dwHoverTime = 1; //WM_MOUSEHOVER消息触发间隔时间
		//	tme.hwndTrack = mwin; //当前窗口句柄
		//	::TrackMouseEvent(&tme); //注册发送消息
		//	//MESSAGE(mwin, WM_MOUSEHOVER, 0, 0);
		//}
		//else
		//{
		//	// log -> error 
		//	return false;
		//}

#endif // _MSC_VER
		return true; 
	}

	bool cinput_device::OnRtcMouseLeave(const nlohmann::json & data)
	{
		return false;
	}
 
	


	/**
	* 鼠标按下 
	* right <-> left 
	*/
	bool cinput_device::OnMouseDown(const uint8*& Data,   uint32 Size)
	{

		GET(FButtonType, Button);
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseDown at (%d, %d), button %d"), PosX, PosY, Button);
		// log mousedown -> log posX , poxY -> Button 
		//NORMAL_EX_LOG("Button = %d, PosX = %d, PoxY = %d", Button, PosX, PosY );
		_UnquantizeAndDenormalize(PosX, PosY);

		FEvent MouseDownEvent(EventType::MOUSE_DOWN);
		MouseDownEvent.SetMouseClick(Button, PosX, PosY);
		uint32 active_type;

		MouseDownEvent.GetMouseClick(active_type, PosX, PosY);
		//ProcessEvent(MouseDownEvent);
		
		g_width = PosX;
		g_height = PosY;
		
		/*
		PosX = g_width;
		PosY = g_height;*/
		NORMAL_EX_LOG("g_width = %d, g_height = %d, active_type = %d, PosX = %d, PoxY = %d", g_width, g_height, active_type, PosX, PosY );
		//g_move_init = true;
		#if defined(_MSC_VER)
		WINDOW_MAIN();
		//MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		g_main_mouse_down_up = mwin;
		SET_POINT();
		//WINDOW_CHILD();
		//WINDOW_BNTTON_DOWN(vec);
		/*if (mwin)
		{
		::PostMessageW(mwin, active_type, MAKEWPARAM(0, 0), MAKEWPARAM(PosX, PosY));
		}*/

		if (mwin)
		{
			MOUSE_INPUT(mwin);
			//CliENTTOSCREENPOINT(m_main_win, PosX, PosY);
			MESSAGE(mwin, active_type, MAKEWPARAM(0,0), MAKELPARAM(PosX, PosY));//::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log -> error 
			return false;
		}
		#endif//#if defined(_MSC_VER)
		return true;
	
	}

	bool cinput_device::OnRtcMouseDown(const nlohmann::json & data)
	{
		if (data.find("button") == data.end() || data.find("x") == data.end() || data.find("y") == data.end())
		{
			WARNING_EX_LOG("not find 'button' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["button"].is_number() || !data["y"].is_number() ||
			!data["x"].is_number())
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FButtonType Button = data["button"].get<FButtonType>();
		FPosType PosX = data["x"].get<FPosType>();
		FPosType PosY = data["y"].get<FPosType>();
		 
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseDown at (%d, %d), button %d"), PosX, PosY, Button);
		// log mousedown -> log posX , poxY -> Button 
		//NORMAL_EX_LOG("Button = %d, PosX = %d, PoxY = %d", Button, PosX, PosY );
		_UnquantizeAndDenormalize(PosX, PosY);

		FEvent MouseDownEvent(EventType::MOUSE_DOWN);
		MouseDownEvent.SetMouseClick(Button, PosX, PosY);
		uint32 active_type;

		MouseDownEvent.GetMouseClick(active_type, PosX, PosY);
		//ProcessEvent(MouseDownEvent);

		g_width = PosX;
		g_height = PosY;

		/*
		PosX = g_width;
		PosY = g_height;*/
		NORMAL_EX_LOG("g_width = %d, g_height = %d, active_type = %d, PosX = %d, PoxY = %d", g_width, g_height, active_type, PosX, PosY);
		//g_move_init = true;
#if defined(_MSC_VER)
		WINDOW_MAIN();
		//MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		g_main_mouse_down_up = mwin;
		SET_POINT();
		//WINDOW_CHILD();
		//WINDOW_BNTTON_DOWN(vec);
		/*if (mwin)
		{
		::PostMessageW(mwin, active_type, MAKEWPARAM(0, 0), MAKEWPARAM(PosX, PosY));
		}*/

		if (mwin)
		{
			MOUSE_INPUT(mwin);
			//CliENTTOSCREENPOINT(m_main_win, PosX, PosY);
			MESSAGE(mwin, active_type, MAKEWPARAM(0, 0), MAKELPARAM(PosX, PosY));//::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log -> error 
			return false;
		}
#endif//#if defined(_MSC_VER)
		return true;
	}

	/** 
	*松开鼠标
	* 
	*/
	bool cinput_device::OnMouseUp(const uint8*& Data,   uint32 Size)
	{
		GET(FButtonType, Button);
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseUp at (%d, %d), button %d"), PosX, PosY, Button);
		// log mouseup posx, posy, button 
		//NORMAL_EX_LOG("Button = %u, PosX = %d, PoxY = %d ", Button, PosX, PosY );
		_UnquantizeAndDenormalize(PosX, PosY);
		//NORMAL_EX_LOG("PosX = %d, PoxY = %d", PosX, PosY );
		FEvent MouseDownEvent(EventType::MOUSE_UP);
		MouseDownEvent.SetMouseClick(Button, PosX, PosY);
		uint32  active_type;
		MouseDownEvent.GetMouseClick(active_type, PosX, PosY);
		
		g_width = PosX;
		g_height = PosY;
		/*
		PosX = g_width;
		PosY = g_height;*/
		NORMAL_EX_LOG("g_width = %d, g_height = %d, active_type = %d, PosX = %d, PoxY = %d", g_width, g_height,  active_type, PosX, PosY );
		//ProcessEvent(MouseDownEvent);
		//g_move_init = false;
		#if defined(_MSC_VER)
		WINDOW_MAIN();
		//MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		/*SET_POINT();
		WINDOW_CHILD();*/
		//WINDOW_BNTTON_UP(vec);
		
		if (mwin)
		{
			{
				MOUSE_INPUT(mwin);
				//CliENTTOSCREENPOINT(m_main_win, PosX, PosY);
				MESSAGE(mwin, active_type, MAKEWPARAM(0, 0), MAKELPARAM(PosX, PosY));
			}//::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
			
			
		}
		/*if (mwin)
		{
			::PostMessageW(mwin, active_type, MAKEWPARAM(0, 0), MAKEWPARAM( PosX, PosY));
		}*/
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log -> error 
			return false;
		} 
		#endif //#if defined(_MSC_VER)
		return true;
	}

	bool cinput_device::OnRtcMouseUp(const nlohmann::json & data)
	{
		if (data.find("button") == data.end() || data.find("x") == data.end() || data.find("y") == data.end())
		{
			WARNING_EX_LOG("not find 'button' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["button"].is_number() || !data["y"].is_number() ||
			!data["x"].is_number())
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FButtonType Button = data["button"].get<FButtonType>();
		FPosType PosX = data["x"].get<FPosType>();
		FPosType PosY = data["y"].get<FPosType>();
		/*	GET(FButtonType, Button);
			GET(FPosType, PosX);
			GET(FPosType, PosY);
			checkf(Size == 0, TEXT("%d"), Size);*/
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseUp at (%d, %d), button %d"), PosX, PosY, Button);
		// log mouseup posx, posy, button 
		//NORMAL_EX_LOG("Button = %u, PosX = %d, PoxY = %d ", Button, PosX, PosY );
		_UnquantizeAndDenormalize(PosX, PosY);
		//NORMAL_EX_LOG("PosX = %d, PoxY = %d", PosX, PosY );
		FEvent MouseDownEvent(EventType::MOUSE_UP);
		MouseDownEvent.SetMouseClick(Button, PosX, PosY);
		uint32  active_type;
		MouseDownEvent.GetMouseClick(active_type, PosX, PosY);

		g_width = PosX;
		g_height = PosY;
		/*
		PosX = g_width;
		PosY = g_height;*/
		NORMAL_EX_LOG("g_width = %d, g_height = %d, active_type = %d, PosX = %d, PoxY = %d", g_width, g_height, active_type, PosX, PosY);
		//ProcessEvent(MouseDownEvent);
		//g_move_init = false;
#if defined(_MSC_VER)
		WINDOW_MAIN();
		//MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		/*SET_POINT();
		WINDOW_CHILD();*/
		//WINDOW_BNTTON_UP(vec);

		if (mwin)
		{
			{
				MOUSE_INPUT(mwin);
				//CliENTTOSCREENPOINT(m_main_win, PosX, PosY);
				MESSAGE(mwin, active_type, MAKEWPARAM(0, 0), MAKELPARAM(PosX, PosY));
			}//::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);


		}
		/*if (mwin)
		{
			::PostMessageW(mwin, active_type, MAKEWPARAM(0, 0), MAKEWPARAM( PosX, PosY));
		}*/
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log -> error 
			return false;
		}
#endif //#if defined(_MSC_VER)
		return true;
		return false;
	}



	/** 
	*鼠标移动
	*/
	bool cinput_device::OnMouseMove(const uint8*& Data,   uint32 Size)
	{
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		GET(FDeltaType, DeltaX);
		GET(FDeltaType, DeltaY);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseMove to (%d, %d), delta (%d, %d)"), PosX, PosY, DeltaX, DeltaY);
		// log mousemove to posx, posy, [DeltaX, DeltaY]
		//NORMAL_EX_LOG("PosX = %d, PoxY = %d, DeltaY = %d", PosX, PosY, DeltaY);
		
		//RTC_LOG(LS_INFO) << "mousemove -->  PosX = " << PosX << ", PoxY = " << PosY << ", DeltaY = " << DeltaY;
		_UnquantizeAndDenormalize(PosX, PosY);
		_UnquantizeAndDenormalize(DeltaX, DeltaY);
		//RTC_LOG(LS_INFO) << "mousemove <==>  PosX = " << PosX << ", PoxY = " << PosY << ", DeltaY = " << DeltaY;
		NORMAL_EX_LOG("g_width = %d, g_height = %d, ---> PosX = %d, PoxY = %d, DeltaY = %d", g_width, g_height, PosX, PosY, DeltaY);
		
		FEvent MouseMoveEvent(EventType::MOUSE_MOVE);
		MouseMoveEvent.SetMouseDelta(PosX, PosY, DeltaX, DeltaY);
		int32_t width = g_width;
		int32_t height = g_height;
		g_width = PosX;
		g_height = PosY;

		/*
		PosX = g_width;
		PosY = g_height;*/
		
		#if defined(_MSC_VER)

		WINDOW_MAIN();
		//MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		//WINDOW_BNTTON_UP(vec);
		
		if (mwin)
		{
			//TRACKMOUSEEVENT tme;
			//tme.cbSize = sizeof(tme);	//结构体缓冲区大小
			//tme.dwFlags = TME_HOVER;	//注册WM_MOUSEHOVER消息
			//tme.dwHoverTime = 1; //WM_MOUSEHOVER消息触发间隔时间
			//tme.hwndTrack = mwin; //当前窗口句柄
			//::TrackMouseEvent(&tme); //注册发送消息

			POINT CursorPoint;
			CursorPoint.x = PosX;
			CursorPoint.y = PosY;
			//::ClientToScreen(mwin, &CursorPoint);
			//::SetFocus(mwin);
			//INPUT input[4];
			//hwndConsole = GetConsoleWindow();
			//hwndTX = FindWindow(TEXT("TXGuiFoundation"), TEXT("N3verL4nd"));
			/*if (hwndTX != NULL)
			{
				for (int i = 0; i < 10; i++)
				{
					SendMessage(hwndTX, WM_CHAR, 'A', 0);
				}*/

				//SetForegroundWindow(hwndTX);
				//memset(input, 0, sizeof(input));
				/*input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
				input[0].ki.wVk = input[2].ki.wVk = VK_MENU;
				input[1].ki.wVk = input[3].ki.wVk = 0x53;
				input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;*/
				//SendInput(4, input, sizeof(INPUT));
				//SetForegroundWindow(hwndConsole);
			/*{
				INPUT Input = { 0 };
				Input.type = INPUT_MOUSE;
				Input.mi.dx = CursorPoint.x;
				Input.mi.dy = CursorPoint.y;
				Input.mi.mouseData = 0;
				Input.mi.dwFlags = MOUSEEVENTF_MOVE;
				Input.mi.time = 0;
				Input.mi.dwExtraInfo = 0;
				SendInput(1, &Input, sizeof(INPUT));
			}*/
			// TODO@chensong 20220612 完全为了兼容Win的input设备
			//mouse_event();
			//
			//GetDesktopWindow()->GetActiveWindow() 
				//GetForegroundWindow();
			//SetForegroundWindow(mwin);
			/*INPUT input;
			input.type = INPUT_HARDWARE;
			input.hi.uMsg = WM_INPUT;
			input.hi.wParamL = MAKEWPARAM(DeltaX, DeltaY);
			input.hi.wParamH = MAKELPARAM(CursorPoint.x, CursorPoint.y);*/
			//static int x = 0;
			//static int y = 0;
			//int xx = CursorPoint.x ;
			//int yy = CursorPoint.y ;
			//input.mi.dx = xx - x;
			//input.mi.dy = yy - y;
			//x = xx;
			//y = yy;
			//input.mi.mouseData = 0;
			//input.mi.dwFlags = /*MOUSEEVENTF_ABSOLUTE |*/ MOUSEEVENTF_MOVE;   //MOUSEEVENTF_ABSOLUTE 代表决对位置  MOUSEEVENTF_MOVE代表移动事件
			//input.mi.time = 0;
			//input.mi.dwExtraInfo = 0;
			//SendInput(1, &input, sizeof(INPUT));

			//::SetCursorPos(CursorPoint.x, CursorPoint.y);
		//	MESSAGE(mwin, WM_INPUT/*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY), MAKELPARAM(CursorPoint.x, CursorPoint.y));
		//try sending 'W' 
			RAWINPUT raw = { 0 };
			//char c = 'W';
			//header 
			/*raw.header.dwSize = sizeof(raw);
			raw.header.dwType = RIM_TYPEMOUSE;
			raw.header.wParam = MAKEWPARAM(DeltaX, DeltaY);*/ //(wParam & 0xff =0 => 0) 
			//raw.header.hDevice = hDevice;


			//data 
			//raw.data.mouse.lLastX = DeltaX;
			//raw.data.mouse.lLastY = DeltaY;
			//raw.data.mouse.usFlags
			//raw.data.keyboard.Reserved = 0;
			//raw.data.keyboard.Flags = RI_KEY_MAKE;  //Key down 
			//raw.data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
			//raw.data.keyboard.Message = WM_KEYDOWN;
			//raw.data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
			//raw.data.keyboard.ExtraInformation = 0;   //??? 
			//HGLOBAL raw_input_ptr = ::GlobalAlloc(GHND, sizeof(RAWINPUT));
			// *pRaw = reinterpret_cast<RAWINPUT*>(::GlobalLock(raw_input_ptr));
			//if (pRaw)
			if (false)
			{
				//char c = 'W';
			//header
				RAWINPUT pRaw;
				pRaw.header.dwSize = sizeof(RAWINPUT);
				pRaw.header.dwType = RIM_TYPEMOUSE;
				pRaw.header.wParam = 0; //(wParam & 0xff =0 => 0)
				pRaw.header.hDevice = 0;

				//data
				pRaw.data.mouse.lLastX = g_width - width ;
				pRaw.data.mouse.lLastY =    g_height - height;
				//pRaw->data.keyboard.Reserved = 0;
				//pRaw->data.keyboard.Flags = RI_KEY_MAKE;
				//pRaw->data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
				//pRaw->data.keyboard.Message = WM_KEYDOWN;
				//pRaw->data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
				//pRaw->data.keyboard.ExtraInformation = 0;
				//memcpy(g_hrawinput[g_write_index % RAW_INPUT_SIZE], &pRaw, sizeof(RAWINPUT));
				//g_hrawinput[g_write_index % RAW_INPUT_SIZE] = pRaw;
				
				 {
					clock_guard lock(g_mutex);
					/*if (g_hrawinput.size() > 300)
					{
						g_hrawinput.clear();
					}*/
					//g_hrawinput.push_back(pRaw);
					//DEBUG_LOG("MAKELPARAM(CursorPoint.x, CursorPoint.y) = %u", MAKELPARAM(CursorPoint.x, CursorPoint.y));
					g_hrawinput[MAKELPARAM(CursorPoint.x, CursorPoint.y)] = pRaw;
					//g_write_index++;
				}
				//::GlobalUnlock(raw_input_ptr);

				//UINT dataSz{ 0 };
				 MOUSE_INPUT(mwin);
				//Send the message ///*Raw input handle*/
				MESSAGE(mwin, WM_INPUT, (WPARAM)RIM_INPUT, MAKELPARAM(CursorPoint.x, CursorPoint.y));  //TODO: Handle to raw input 
			}
			
			//MOUSE_INPUT(mwin);
			MESSAGE(mwin, WM_MOUSEMOVE /*WM_MOUSEMOVE*//*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY) , MAKELPARAM(PosX, PosY));
			//MESSAGE(mwin, WM_INPUT /*WM_MOUSEMOVE*//*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY), MAKELPARAM(CursorPoint.x, CursorPoint.y));


			//WM_MOUSEHOVER
			//WM_MOUSELEAVE
			//RAWINPUT raw = { 0 };
			//char c = 'W';
			////header
			//raw.header.dwSize = sizeof(raw);
			//raw.header.dwType = RIM_TYPEKEYBOARD;
			//raw.header.wParam = 0; //(wParam & 0xff =0 => 0)
			//raw.header.hDevice = hDevice;

			//data
			//raw.data.keyboard.Reserved = 0;
			//raw.data.keyboard.Flags = RI_KEY_MAKE;      //Key down
			//raw.data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
			//raw.data.keyboard.Message = WM_KEYDOWN;
			//raw.data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
			//raw.data.keyboard.ExtraInformation = 0;         //???

			//Send the message
			//SendMessage(mwin, WM_INPUT, 0, (LPARAM)raw/*Raw input handle*/);
		}
		else
		{
			// log -> error 
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
		#endif // #if defined(_MSC_VER)
		//ProcessEvent(MouseMoveEvent);
		return true;
	}
	bool cinput_device::OnRtcMouseMove(const nlohmann::json & data)
	{
		if (data.find("delta_x") == data.end() || data.find("delta_y") == data.end() || data.find("x") == data.end() || data.find("y") == data.end())
		{
			WARNING_EX_LOG("not find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		 if (!data["x"].is_number() || !data["y"].is_number() || 
			 !data["delta_x"].is_number() || !data["delta_y"].is_number())
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		} 
		 
		FPosType PosX = data["x"].get<FPosType>();
		FPosType PosY = data["y"].get<FPosType>();
		FDeltaType DeltaX = data["delta_x"].get<FDeltaType>();
		FDeltaType DeltaY = data["delta_y"].get<FDeltaType>();
			/*GET(FPosType, PosX);
			GET(FPosType, PosY);
			GET(FDeltaType, DeltaX);
			GET(FDeltaType, DeltaY);
			checkf(Size == 0, TEXT("%d"), Size);*/
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseMove to (%d, %d), delta (%d, %d)"), PosX, PosY, DeltaX, DeltaY);
		// log mousemove to posx, posy, [DeltaX, DeltaY]
		//NORMAL_EX_LOG("PosX = %d, PoxY = %d, DeltaY = %d", PosX, PosY, DeltaY);

		//RTC_LOG(LS_INFO) << "mousemove -->  PosX = " << PosX << ", PoxY = " << PosY << ", DeltaY = " << DeltaY;
		_UnquantizeAndDenormalize(PosX, PosY);
		_UnquantizeAndDenormalize(DeltaX, DeltaY);
		//RTC_LOG(LS_INFO) << "mousemove <==>  PosX = " << PosX << ", PoxY = " << PosY << ", DeltaY = " << DeltaY;
		NORMAL_EX_LOG("g_width = %d, g_height = %d, ---> PosX = %d, PoxY = %d, DeltaY = %d", g_width, g_height, PosX, PosY, DeltaY);

		FEvent MouseMoveEvent(EventType::MOUSE_MOVE);
		MouseMoveEvent.SetMouseDelta(PosX, PosY, DeltaX, DeltaY);
		int32_t width = g_width;
		int32_t height = g_height;
		g_width = PosX;
		g_height = PosY;

		/*
		PosX = g_width;
		PosY = g_height;*/

#if defined(_MSC_VER)

		WINDOW_MAIN();
		MOUSE_INPUT(mwin);
		/*registerinputdevice(mwin);
		SetForegroundWindow(mwin);
		SetActiveWindow(mwin);*/
		//WINDOW_BNTTON_UP(vec);

		if (mwin)
		{
			//TRACKMOUSEEVENT tme;
			//tme.cbSize = sizeof(tme);	//结构体缓冲区大小
			//tme.dwFlags = TME_HOVER;	//注册WM_MOUSEHOVER消息
			//tme.dwHoverTime = 1; //WM_MOUSEHOVER消息触发间隔时间
			//tme.hwndTrack = mwin; //当前窗口句柄
			//::TrackMouseEvent(&tme); //注册发送消息

			POINT CursorPoint;
			CursorPoint.x = PosX;
			CursorPoint.y = PosY;
			::ClientToScreen(mwin, &CursorPoint);
			//::SetFocus(mwin);
			//INPUT input[4];
			//hwndConsole = GetConsoleWindow();
			//hwndTX = FindWindow(TEXT("TXGuiFoundation"), TEXT("N3verL4nd"));
			/*if (hwndTX != NULL)
			{
				for (int i = 0; i < 10; i++)
				{
					SendMessage(hwndTX, WM_CHAR, 'A', 0);
				}*/

				//SetForegroundWindow(hwndTX);
				//memset(input, 0, sizeof(input));
				/*input[0].type = input[1].type = input[2].type = input[3].type = INPUT_KEYBOARD;
				input[0].ki.wVk = input[2].ki.wVk = VK_MENU;
				input[1].ki.wVk = input[3].ki.wVk = 0x53;
				input[2].ki.dwFlags = input[3].ki.dwFlags = KEYEVENTF_KEYUP;*/
				//SendInput(4, input, sizeof(INPUT));
				//SetForegroundWindow(hwndConsole);
			/*{
				INPUT Input = { 0 };
				Input.type = INPUT_MOUSE;
				Input.mi.dx = CursorPoint.x;
				Input.mi.dy = CursorPoint.y;
				Input.mi.mouseData = 0;
				Input.mi.dwFlags = MOUSEEVENTF_MOVE;
				Input.mi.time = 0;
				Input.mi.dwExtraInfo = 0;
				SendInput(1, &Input, sizeof(INPUT));
			}*/
			// TODO@chensong 20220612 完全为了兼容Win的input设备
			//mouse_event();
			//
			//GetDesktopWindow()->GetActiveWindow() 
				//GetForegroundWindow();
			//SetForegroundWindow(mwin);
			/*INPUT input;
			input.type = INPUT_HARDWARE;
			input.hi.uMsg = WM_INPUT;
			input.hi.wParamL = MAKEWPARAM(DeltaX, DeltaY);
			input.hi.wParamH = MAKELPARAM(CursorPoint.x, CursorPoint.y);*/
			//static int x = 0;
			//static int y = 0;
			//int xx = CursorPoint.x ;
			//int yy = CursorPoint.y ;
			//input.mi.dx = xx - x;
			//input.mi.dy = yy - y;
			//x = xx;
			//y = yy;
			//input.mi.mouseData = 0;
			//input.mi.dwFlags = /*MOUSEEVENTF_ABSOLUTE |*/ MOUSEEVENTF_MOVE;   //MOUSEEVENTF_ABSOLUTE 代表决对位置  MOUSEEVENTF_MOVE代表移动事件
			//input.mi.time = 0;
			//input.mi.dwExtraInfo = 0;
			//SendInput(1, &input, sizeof(INPUT));

			//::SetCursorPos(CursorPoint.x, CursorPoint.y);
		//	MESSAGE(mwin, WM_INPUT/*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY), MAKELPARAM(CursorPoint.x, CursorPoint.y));
		//try sending 'W' 
			RAWINPUT raw = { 0 };
			//char c = 'W';
			//header 
			/*raw.header.dwSize = sizeof(raw);
			raw.header.dwType = RIM_TYPEMOUSE;
			raw.header.wParam = MAKEWPARAM(DeltaX, DeltaY);*/ //(wParam & 0xff =0 => 0) 
			//raw.header.hDevice = hDevice;


			//data 
			//raw.data.mouse.lLastX = DeltaX;
			//raw.data.mouse.lLastY = DeltaY;
			//raw.data.mouse.usFlags
			//raw.data.keyboard.Reserved = 0;
			//raw.data.keyboard.Flags = RI_KEY_MAKE;  //Key down 
			//raw.data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
			//raw.data.keyboard.Message = WM_KEYDOWN;
			//raw.data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
			//raw.data.keyboard.ExtraInformation = 0;   //??? 
			//HGLOBAL raw_input_ptr = ::GlobalAlloc(GHND, sizeof(RAWINPUT));
			// *pRaw = reinterpret_cast<RAWINPUT*>(::GlobalLock(raw_input_ptr));
			//if (pRaw)
			{
				//char c = 'W';
			//header
				RAWINPUT pRaw;
				pRaw.header.dwSize = sizeof(RAWINPUT);
				pRaw.header.dwType = RIM_TYPEMOUSE;
				pRaw.header.wParam = 0; //(wParam & 0xff =0 => 0)
				pRaw.header.hDevice = 0;

				//data
				pRaw.data.mouse.lLastX = g_width - width;
				pRaw.data.mouse.lLastY = g_height - height;
				//pRaw->data.keyboard.Reserved = 0;
				//pRaw->data.keyboard.Flags = RI_KEY_MAKE;
				//pRaw->data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
				//pRaw->data.keyboard.Message = WM_KEYDOWN;
				//pRaw->data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
				//pRaw->data.keyboard.ExtraInformation = 0;
				//memcpy(g_hrawinput[g_write_index % RAW_INPUT_SIZE], &pRaw, sizeof(RAWINPUT));
				//g_hrawinput[g_write_index % RAW_INPUT_SIZE] = pRaw;

				{
					clock_guard lock(g_mutex);
					/*if (g_hrawinput.size() > 300)
					{
						g_hrawinput.clear();
					}*/
					//g_hrawinput.push_back(pRaw);
					//DEBUG_LOG("MAKELPARAM(CursorPoint.x, CursorPoint.y) = %u", MAKELPARAM(CursorPoint.x, CursorPoint.y));
					g_hrawinput[MAKELPARAM(CursorPoint.x, CursorPoint.y)] = pRaw;
					//g_write_index++;
				}
				//::GlobalUnlock(raw_input_ptr);

				//UINT dataSz{ 0 };
				MOUSE_INPUT(mwin);
				//Send the message ///*Raw input handle*/
				MESSAGE(mwin, WM_INPUT, (WPARAM)RIM_INPUT, MAKELPARAM(CursorPoint.x, CursorPoint.y));  //TODO: Handle to raw input 
			}

			MOUSE_INPUT(mwin);
			MESSAGE(mwin, WM_MOUSEMOVE /*WM_MOUSEMOVE*//*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY), MAKELPARAM(PosX, PosY));
			//MESSAGE(mwin, WM_INPUT /*WM_MOUSEMOVE*//*WM_INPUT 、 WM_NCMOUSEMOVE UE4 move dug TODO@chensong 20220611 */ /*WM_MOUSEMOVE*/, MAKEWPARAM(DeltaX, DeltaY), MAKELPARAM(CursorPoint.x, CursorPoint.y));


			//WM_MOUSEHOVER
			//WM_MOUSELEAVE
			//RAWINPUT raw = { 0 };
			//char c = 'W';
			////header
			//raw.header.dwSize = sizeof(raw);
			//raw.header.dwType = RIM_TYPEKEYBOARD;
			//raw.header.wParam = 0; //(wParam & 0xff =0 => 0)
			//raw.header.hDevice = hDevice;

			//data
			//raw.data.keyboard.Reserved = 0;
			//raw.data.keyboard.Flags = RI_KEY_MAKE;      //Key down
			//raw.data.keyboard.MakeCode = static_cast<WORD>(MapVirtualKeyEx(c, MAPVK_VK_TO_VSC, GetKeyboardLayout(0)));
			//raw.data.keyboard.Message = WM_KEYDOWN;
			//raw.data.keyboard.VKey = VkKeyScanEx(c, GetKeyboardLayout(0));
			//raw.data.keyboard.ExtraInformation = 0;         //???

			//Send the message
			//SendMessage(mwin, WM_INPUT, 0, (LPARAM)raw/*Raw input handle*/);
		}
		else
		{
			// log -> error 
			WARNING_EX_LOG("not find main window failed !!!");
			return false;
		}
#endif // #if defined(_MSC_VER)
		//ProcessEvent(MouseMoveEvent);
		return true;
		return false;
	}
	/** 
	* 鼠标双击
	*/
	bool cinput_device::OnMouseDoubleClick(const uint8*& Data,   uint32 Size)
	{
		return false;
		//WM_LBUTTONDBLCLK
		GET(FButtonType, Button);
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseDown at (%d, %d), button %d"), PosX, PosY, Button);
		// log mousedown -> log posX , poxY -> Button 
		//NORMAL_EX_LOG("Button = %d, PosX = %d, PoxY = %d", Button, PosX, PosY );
		_UnquantizeAndDenormalize(PosX, PosY);

		FEvent MouseDownEvent(EventType::MOUSE_DOWN);
		MouseDownEvent.SetMouseClick(Button, PosX, PosY);
		uint32 active_type;

		MouseDownEvent.GetMouseClick(active_type, PosX, PosY);
		g_width = PosX;
		g_height = PosY;
		/*
		PosX = g_width;
		PosY = g_height;*/
		//ProcessEvent(MouseDownEvent);
		NORMAL_EX_LOG("g_width = %d, g_height = %d, ---> PosX = %d, PoxY = %d", g_width, g_height, PosX, PosY);

		//NORMAL_EX_LOG("active_type = %d, PosX = %d, PoxY = %d", active_type, PosX, PosY );
#if defined(_MSC_VER)
		WINDOW_MAIN();

		SET_POINT();
		WINDOW_CHILD();
		//WINDOW_BNTTON_DOWN(vec);
		/*if (mwin)
		{
		::PostMessageW(mwin, active_type, MAKEWPARAM(0, 0), MAKEWPARAM(PosX, PosY));
		}*/
		if (childwin)
		{
			CliENTTOSCREENPOINT(childwin, PosX, PosY);
			MESSAGE(childwin, WM_LBUTTONDBLCLK, MAKEWPARAM(0, 0), MAKELPARAM(CursorPoint.x, CursorPoint.y));//::PostMessageW(childwin, WM_KEYUP, KeyCode, 1);
		}
		else if (mwin) 
		{
			//CliENTTOSCREENPOINT(mwin, PosX, PosY);
			MESSAGE(mwin, WM_LBUTTONDBLCLK, MAKEWPARAM(0, 0), MAKELPARAM(PosX, PosY));//::PostMessageW(mwin, WM_KEYUP, KeyCode, 1);
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log -> error 
			return false;
		}
#endif//#if defined(_MSC_VER)
		 
		return true;
	}

	bool cinput_device::OnRtcMouseDoubleClick(const nlohmann::json & data)
	{
		return false;
	}

	/** 
	* 鼠标的滚轮滚动 
	*/
	bool cinput_device::OnMouseWheel(const uint8*& Data,   uint32 Size)
	{
		GET(FDeltaType, Delta);
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		checkf(Size == 0, TEXT("%d"), Size);
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseWheel, delta %d"), Delta);
		// mouseWheel delta -> 
		_UnquantizeAndDenormalize(PosX, PosY);

		FEvent MouseWheelEvent(EventType::MOUSE_WHEEL);
		MouseWheelEvent.SetMouseWheel(Delta, PosX, PosY);
		//ProcessEvent(MouseWheelEvent);
		g_width = PosX;
		g_height = PosY;
		/*PosX = g_width;
		PosY = g_height;*/
		#if defined(_MSC_VER)
		WINDOW_MAIN();
		NORMAL_EX_LOG(" PosX = %d, PoxY = %d", PosX, PosY);
		if (mwin)
		{
			MOUSE_INPUT(mwin);
			//CliENTTOSCREENPOINT(mwin, PosX, PosY);
			//::PostMessage(mwin, WM_MOUSEWHEEL, MAKEWPARAM(0, Delta) /* ascii码 */, MAKELPARAM(PosX, PosY));
			MESSAGE(mwin, WM_MOUSEWHEEL, MAKEWPARAM(0, Delta) /* ascii码 */, MAKELPARAM(PosX, PosY));
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log error 
			return false;
		}
		#endif // #if defined(_MSC_VER)
		return true;
	}

	bool cinput_device::OnRtcMouseWheel(const nlohmann::json & data)
	{
		if (data.find("delta") == data.end()  || data.find("x") == data.end() || data.find("y") == data.end())
		{
			WARNING_EX_LOG("not find 'delta'   or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		if (!data["x"].is_number() || !data["y"].is_number() ||
			!data["delta"].is_number() )
		{
			WARNING_EX_LOG("  find 'delta_x' or 'delta_y' or 'x' or'y'  [data = %s]", data.dump().c_str());
			return false;
		}
		FDeltaType Delta = data["delta"].get<FDeltaType>();
		FPosType PosX = data["x"].get<FPosType>();
		FPosType PosY = data["y"].get<FPosType>();
		/*GET(FDeltaType, Delta);
		GET(FPosType, PosX);
		GET(FPosType, PosY);
		checkf(Size == 0, TEXT("%d"), Size);*/
		//UE_LOG(PixelStreamerInput, Verbose, TEXT("mouseWheel, delta %d"), Delta);
		// mouseWheel delta -> 
		_UnquantizeAndDenormalize(PosX, PosY);

		FEvent MouseWheelEvent(EventType::MOUSE_WHEEL);
		MouseWheelEvent.SetMouseWheel(Delta, PosX, PosY);
		//ProcessEvent(MouseWheelEvent);
		g_width = PosX;
		g_height = PosY;
		/*PosX = g_width;
		PosY = g_height;*/
#if defined(_MSC_VER)
		WINDOW_MAIN();
		NORMAL_EX_LOG(" PosX = %d, PoxY = %d", PosX, PosY);
		if (mwin)
		{
			MOUSE_INPUT(mwin);
			//CliENTTOSCREENPOINT(mwin, PosX, PosY);
			//::PostMessage(mwin, WM_MOUSEWHEEL, MAKEWPARAM(0, Delta) /* ascii码 */, MAKELPARAM(PosX, PosY));
			MESSAGE(mwin, WM_MOUSEWHEEL, MAKEWPARAM(0, Delta) /* ascii码 */, MAKELPARAM(PosX, PosY));
		}
		else
		{
			WARNING_EX_LOG("not find main window failed !!!");
			// log error 
			return false;
		}
#endif // #if defined(_MSC_VER)
		return true;
	}

	// XY positions are the ratio (0.0..1.0) along a viewport axis, quantized
	// into an uint16 (0..65536). This allows the browser viewport and player
	// viewport to have a different size.
	void cinput_device::_UnquantizeAndDenormalize(uint16& InOutX, uint16& InOutY)
	{
		InOutX = InOutX / 65536.0f * m_int_point.X;
		InOutY = InOutY / 65536.0f * m_int_point.Y;
	}
	
	// XY deltas are the ratio (-1.0..1.0) along a viewport axis, quantized
	// into an int16 (-32767..32767). This allows the browser viewport and
	// player viewport to have a different size.
	void cinput_device::_UnquantizeAndDenormalize(int16& InOutX, int16& InOutY)
	{ 
		InOutX = InOutX / 32767.0f * m_int_point.X;
		InOutY = InOutY / 32767.0f * m_int_point.Y;
	}
	void cinput_device::_work_pthread()
	{
		std::list< webrtc::DataBuffer>  temp_list;
		static const uint32 TICK_TIME = 50;
		std::chrono::steady_clock::time_point cur_time_ms;
		std::chrono::steady_clock::time_point pre_time = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration dur;
		std::chrono::milliseconds ms;
		uint32_t elapse = 0;

		while (!m_stoped)
		{
			pre_time = std::chrono::steady_clock::now();
			{
				std::lock_guard<std::mutex>  lock(m_input_mutex);
				if (!m_input_list.empty())
				{
					temp_list.swap (m_input_list);
					//m_input_list.clear();
				}
			}

			while (!temp_list.empty())
			{
				OnMessage("", temp_list.front());
				temp_list.pop_front();
				 
			}
			if (m_input_list.empty())
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
	}
}