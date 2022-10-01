#pragma once
#include <Windows.h>
#include <vector>
#include <string>


class memcleanManager {

private:
	static memcleanManager cleanManager;
	
private:
	memcleanManager();
	~memcleanManager();
	memcleanManager(const memcleanManager&)               = delete;
	memcleanManager(memcleanManager&&)                    = delete;
	memcleanManager& operator= (const memcleanManager&)   = delete;
	memcleanManager& operator= (memcleanManager&&)        = delete;


public:
	static memcleanManager& getInstance();

public:
	void       init();

	bool       loadcfg();
	void       savecfg();

	void       trimProcessWorkingSet();
	void       trimProcessWorkingSet2();
	void       flushSystemBuffer();
	void       cleanMemory_all();

	void       getSystemCacheInfo(SIZE_T* used, SIZE_T* total);

	void       raiseMemCleanThread();

public:
	volatile bool    memCleanSwitches[6];
	volatile bool    autoStart;
	volatile bool    bruteMode;
	volatile HWND    hDlg;
	volatile HWND    hwndPB;

private:
	std::string      profile_str;

private:
	enum _SYSTEM_INFORMATION_CLASS { // info param 1
		SystemFileCacheInformation = 0x15, // 查询系统缓存用
		SystemMemoryListInformation = 0x50, // rammap用的，可清理sguard   6.0 and higher
		SystemCombinePhysicalMemoryInformation = 0x82, // memreduct 1   6.2 and higher
	};

	typedef struct _SYSTEM_FILECACHE_INFORMATION { // info param 2
		SIZE_T CurrentSize;
		SIZE_T PeakSize;
		ULONG  PageFaultCount;
		SIZE_T MinimumWorkingSet;
		SIZE_T MaximumWorkingSet;
		SIZE_T CurrentSizeIncludingTransitionInPages;
		SIZE_T PeakSizeIncludingTransitionInPages;
		ULONG  TransitionRePurposeCount;
		ULONG  Flags;
	} SYSTEM_FILECACHE_INFORMATION;

	typedef struct _MEMORY_COMBINE_INFORMATION_EX { // info param 2
		HANDLE    Handle;
		ULONG_PTR PagesCombined;
		ULONG     Flags;
	} MEMORY_COMBINE_INFORMATION_EX;

	enum _SYSTEM_MEMORY_LIST_COMMAND { // info param 2
		MemoryCaptureAccessedBits,
		MemoryCaptureAndResetAccessedBits,
		MemoryEmptyWorkingSets, // rammap用的
		MemoryFlushModifiedList, // 新加的
		MemoryPurgeStandbyList,
		MemoryPurgeLowPriorityStandbyList, // 新加的
		MemoryCommandMax
	};


	using NtSetSystemInformation_t   = NTSTATUS(WINAPI*)(INT, PVOID, ULONG);
	using NtQuerySystemInformation_t = DWORD(WINAPI*)(UINT, PVOID, DWORD, PDWORD);

	NtSetSystemInformation_t         NtSetSystemInformation;
	NtQuerySystemInformation_t       NtQuerySystemInformation;
};


class win32SystemManager {

private:
	static win32SystemManager  systemManager;

private:
	win32SystemManager();
	~win32SystemManager();
	win32SystemManager(const win32SystemManager&)               = delete;
	win32SystemManager(win32SystemManager&&)                    = delete;
	win32SystemManager& operator= (const win32SystemManager&)   = delete;
	win32SystemManager& operator= (win32SystemManager&&)        = delete;

public:
	static win32SystemManager&  getInstance();

public:
	bool       systemInit(HINSTANCE hInstance);
	void       setupProcessDpi();
	bool       createWindow(WNDPROC WndProc, DWORD WndIcon);
	void       createTray(UINT trayActiveMsg);
	void       removeTray();
	WPARAM     messageLoop();

public:
	void       panic(const char* format, ...);
	void       panic(DWORD errorCode, const char* format, ...);
	
private:
	ATOM     _registerMyClass(WNDPROC WndProc, DWORD iconRcNum);
	void     _panic(DWORD code, const char* showbuf);


public:
	HINSTANCE           hInstance;
	HWND                hWnd;

private:
	HANDLE              hProgram;
	NOTIFYICONDATA      icon;
};