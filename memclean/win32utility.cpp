#include <Windows.h>
#include <tlhelp32.h>
#include <time.h>
#include <Psapi.h>
#include <thread>
#include <mutex>
#include "wndproc.h"
#include "win32utility.h"


memcleanManager::memcleanManager() 
	: memCleanSwitches{}, autoStart(false), bruteMode(false), hDlg(NULL), hwndPB(NULL), NtSetSystemInformation(NULL) {}

memcleanManager::~memcleanManager() {
	if (hwndPB) {
		DestroyWindow(hwndPB);
		hwndPB = NULL;
	}
	if (hDlg) {
		EndDialog(hDlg, TRUE);
		hDlg = NULL;
	}
}

memcleanManager& memcleanManager::getInstance() {
	static memcleanManager mgr;
	return mgr;
}

void memcleanManager::init() {

	// raise privilege.
	HANDLE hToken;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
	
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	// optimize system cache
	LookupPrivilegeValue(NULL, "SeIncreaseQuotaPrivilege", &tp.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

	// modify memory standby list
	LookupPrivilegeValue(NULL, "SeProfileSingleProcessPrivilege", &tp.Privileges[0].Luid);
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);


	// check if privileges are aquired.
	BOOL bResult;

	PRIVILEGE_SET ps;
	ps.Control = PRIVILEGE_SET_ALL_NECESSARY;
	ps.PrivilegeCount = 1;
	ps.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;

	LookupPrivilegeValue(NULL, "SeIncreaseQuotaPrivilege", &ps.Privilege[0].Luid);
	PrivilegeCheck(hToken, &ps, &bResult);
	if (!bResult) {
		MessageBox(0, "SeIncreaseQuotaPrivilege fail", 0, 0);
	}

	LookupPrivilegeValue(NULL, "SeProfileSingleProcessPrivilege", &ps.Privilege[0].Luid);
	PrivilegeCheck(hToken, &ps, &bResult);
	if (!bResult) {
		MessageBox(0, "SeProfileSingleProcessPrivilege fail", 0, 0);
	}

	CloseHandle(hToken);


	// acquire undocumented function address.
	HMODULE hNtdll = GetModuleHandle("ntdll.dll");
	if (hNtdll) {
		NtSetSystemInformation = (NtSetSystemInformation_t)GetProcAddress(hNtdll, "NtSetSystemInformation");
	}


	// get profile.
	CHAR         buf[1024];
	DWORD        size = 1024;

	ExpandEnvironmentStrings("%appdata%\\memclean", buf, size);

	profile_str = buf;

	DWORD pathAttr = GetFileAttributes(profile_str.c_str());
	if ((pathAttr == INVALID_FILE_ATTRIBUTES) || !(pathAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		if (!CreateDirectory(profile_str.c_str(), NULL)) {
			profile_str = "C:";
		}
	}

	profile_str += "\\memclean.ini";
}

bool memcleanManager::loadcfg() {

	auto     profile = profile_str.c_str();
	bool     result = true;
	char     buf[0x1000];

	GetPrivateProfileString("Global", "Version", NULL, buf, 128, profile);
	if (strcmp(buf, VERSION) != 0) {
		WritePrivateProfileString("Global", "Version", VERSION, profile);
		result = false;
	}

	autoStart = GetPrivateProfileInt("memclean", "autostart", 0, profile);
	bruteMode = GetPrivateProfileInt("memclean", "bruteMode", 1, profile);

	for (int i = 0; i < 6; i++) {
		sprintf(buf, "switch%d", i);
		memCleanSwitches[i] = GetPrivateProfileInt("memclean", buf, 0, profile);
	}

	if (!result) {
		bruteMode = 1;
		memCleanSwitches[3] = memCleanSwitches[4] = true;
		savecfg();
	}

	return result;
}

void memcleanManager::savecfg() {

	auto     profile = profile_str.c_str();
	char     buf[0x1000];

	WritePrivateProfileString("memclean", "autostart", autoStart ? "1" : "0", profile);
	WritePrivateProfileString("memclean", "bruteMode", bruteMode ? "1" : "0", profile);

	for (int i = 0; i < 6; i++) {
		sprintf(buf, "switch%d", i);
		WritePrivateProfileString("memclean", buf, memCleanSwitches[i] ? "1" : "0", profile);
	}
}

void memcleanManager::trimProcessWorkingSet() {
	HANDLE            hSnapshot = NULL;
	PROCESSENTRY32    pe = {};
	pe.dwSize = sizeof(PROCESSENTRY32);


	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		return;
	}

	for (BOOL next = Process32First(hSnapshot, &pe); next; next = Process32Next(hSnapshot, &pe)) {

		auto handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
		if (!handle) {
			handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
		}
		if (!handle) {
			continue;
		}
		if (!EmptyWorkingSet(handle)) {
			//MessageBox(0, "EmptyWorkingSet false", 0, 0);
		}
		CloseHandle(handle);
	}

	CloseHandle(hSnapshot);
}

void memcleanManager::trimProcessWorkingSet2() {

	if (!NtSetSystemInformation) {
		return;
	}

	_SYSTEM_MEMORY_LIST_COMMAND command = MemoryEmptyWorkingSets;

	NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
}

int memcleanManager::flushSystemBuffer() {

	if (!NtSetSystemInformation) {
		return -1;
	}

	SYSTEM_FILECACHE_INFORMATION info;
	ZeroMemory(&info, sizeof(info));
	info.MinimumWorkingSet = -1;
	info.MaximumWorkingSet = -1;

	return (int)NtSetSystemInformation(SystemFileCacheInformation, &info, sizeof(info));
}

int memcleanManager::purgeMemoryStandbyList() {

	if (!NtSetSystemInformation) {
		return -1;
	}

	_SYSTEM_MEMORY_LIST_COMMAND command;
	int result;

	command = MemoryFlushModifiedList;
	result = (int)NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
	if (result < 0) {
		return result;
	}

	command = MemoryPurgeStandbyList;
	result = (int)NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
	if (result < 0) {
		return result;
	}

	command = MemoryPurgeLowPriorityStandbyList;
	result = (int)NtSetSystemInformation(SystemMemoryListInformation, &command, sizeof(command));
	if (result < 0) {
		return result;
	}

	return 0;
}

void memcleanManager::raiseMemCleanThread() {

	std::thread t([this] () {

		auto lastCleanTime = time(NULL);

		while (1) {

			bool cleaned = false;

			if (memCleanSwitches[0] || memCleanSwitches[1] || memCleanSwitches[2]) {

				auto currentTime = time(NULL);
				if (currentTime - lastCleanTime > 300) {

					if (memCleanSwitches[0]) {
						if (bruteMode) {
							trimProcessWorkingSet2();
						} else {
							trimProcessWorkingSet();
						}
					}
					if (memCleanSwitches[1]) {
						flushSystemBuffer();
					}
					if (memCleanSwitches[2]) {
						purgeMemoryStandbyList();
					}

					lastCleanTime = currentTime;
					cleaned = true;
				}
			}

			if (!cleaned && (memCleanSwitches[3] || memCleanSwitches[4] || memCleanSwitches[5])) {

				ULONGLONG totalPhysMem = 0;
				ULONGLONG usedPhysMem = 0;
				double usedPercent = 0.0;

				MEMORYSTATUSEX memInfo;
				memset(&memInfo, 0, sizeof(memInfo));
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				totalPhysMem = memInfo.ullTotalPhys;
				usedPhysMem = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
				usedPercent = (double)usedPhysMem / totalPhysMem;

				if (usedPercent >= 0.8) {
					if (memCleanSwitches[3]) {
						if (bruteMode) {
							trimProcessWorkingSet2();
						} else {
							trimProcessWorkingSet();
						}
					}
					if (memCleanSwitches[4]) {
						flushSystemBuffer();
					}
					if (memCleanSwitches[5]) {
						purgeMemoryStandbyList();
					}
				}
			}

			Sleep(15000);
		}
	});
	t.detach();
}



win32SystemManager win32SystemManager::systemManager;

win32SystemManager::win32SystemManager() 
	: hWnd(NULL), hInstance(NULL),
	  hProgram(NULL), icon{} {}

win32SystemManager::~win32SystemManager() {
	if (hProgram) {
		ReleaseMutex(hProgram);
	}
}

win32SystemManager& win32SystemManager::getInstance() {
	return systemManager;
}

void win32SystemManager::setupProcessDpi() {

	HMODULE hUser32 = LoadLibrary("User32.dll");

	if (hUser32) {

		typedef BOOL(WINAPI* fp)(DPI_AWARENESS_CONTEXT);
		fp SetProcessDpiAwarenessContext = (fp)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");

		if (SetProcessDpiAwarenessContext) {
			SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
		
		} else {
			SetProcessDPIAware();
		}

		FreeLibrary(hUser32);
	}
}

bool win32SystemManager::systemInit(HINSTANCE hInstance) {

	this->hInstance      = hInstance;

	hProgram = CreateMutex(NULL, FALSE, "memcleaner");
	if (!hProgram || GetLastError() == ERROR_ALREADY_EXISTS) {
		panic(0, "Memory Cleaner已经启动了。");
		return false;
	}

	return true;
}

bool win32SystemManager::createWindow(WNDPROC WndProc, DWORD WndIcon) {
	
	if (!_registerMyClass(WndProc, WndIcon)) {
		panic("创建窗口类失败。");
		return false;
	}

	hWnd = CreateWindow(
		"memcleaner_WindowClass",
		"memcleaner_Window",
		WS_EX_TOPMOST, CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, 0, 0, hInstance, 0);

	if (!hWnd) {
		panic("创建窗口失败。");
		return false;
	}

	ShowWindow(hWnd, SW_HIDE);

	return true;
}

WPARAM win32SystemManager::messageLoop() {
	
	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

void win32SystemManager::createTray(UINT trayActiveMsg) {

	icon.cbSize = sizeof(icon);
	icon.hWnd = hWnd;
	icon.uID = 0;
	icon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
	icon.uCallbackMessage = trayActiveMsg;
	icon.hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
	strcpy(icon.szTip, "Memory Cleaner");

	Shell_NotifyIcon(NIM_ADD, &icon);
}

void win32SystemManager::removeTray() {
	Shell_NotifyIcon(NIM_DELETE, &icon);
}

void win32SystemManager::panic(const char* format, ...) {
	
	// call GetLastError first; to avoid errors in current function.
	DWORD errorCode = GetLastError();

	CHAR buf[0x1000];

	va_list arg;
	va_start(arg, format);
	vsprintf(buf, format, arg);
	va_end(arg);

	_panic(errorCode, buf);
}

void win32SystemManager::panic(DWORD errorCode, const char* format, ...) {

	CHAR buf[0x1000];

	va_list arg;
	va_start(arg, format);
	vsprintf(buf, format, arg);
	va_end(arg);

	_panic(errorCode, buf);
}

ATOM win32SystemManager::_registerMyClass(WNDPROC WndProc, DWORD iconRcNum) {

	WNDCLASS wc = { 0 };

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(iconRcNum));
	wc.hCursor = 0;
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = "memcleaner_WindowClass";

	return RegisterClass(&wc);
}

void win32SystemManager::_panic(DWORD code, const char* showbuf) {

	CHAR result[0x1000];

	// put message to result.
	strcpy(result, showbuf);

	// if code != 0, add details in another line.
	if (code != 0) {

		char* description = NULL;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
			          code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&description, 0, NULL);

		sprintf(result + strlen(result), "\n\n发生的错误：(0x%x) %s", code, description);
		LocalFree(description);
	}

	MessageBox(0, result, 0, MB_OK);
}
