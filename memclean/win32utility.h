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
	int        flushSystemBuffer();
	int        purgeMemoryStandbyList();

	void       raiseMemCleanThread();


public:
	volatile bool    memCleanSwitches[6];
	volatile bool    autoStart;
	volatile HWND    hDlg;
	volatile HWND    hwndPB;

private:
	std::string      profile_str;

private:
	enum _SYSTEM_INFORMATION_CLASS {
		SystemFileCacheInformation = 21,
		SystemMemoryListInformation = 80 // 不弄了 memreduct就用的这个 怕卡
	};

	enum _SYSTEM_MEMORY_LIST_COMMAND {
		MemoryCaptureAccessedBits,
		MemoryCaptureAndResetAccessedBits,
		MemoryEmptyWorkingSets, // 没弄，原版没有
		MemoryFlushModifiedList, // 没弄，原版没有
		MemoryPurgeStandbyList,
		MemoryPurgeLowPriorityStandbyList, // 没弄，原版没有
		MemoryCommandMax
	};

	typedef struct _SYSTEM_FILECACHE_INFORMATION {
		SIZE_T CurrentSize;
		SIZE_T PeakSize;
		ULONG PageFaultCount;
		SIZE_T MinimumWorkingSet;
		SIZE_T MaximumWorkingSet;
		SIZE_T CurrentSizeIncludingTransitionInPages;
		SIZE_T PeakSizeIncludingTransitionInPages;
		ULONG TransitionRePurposeCount;
		ULONG Flags;
	} SYSTEM_FILECACHE_INFORMATION;


	using NtSetSystemInformation_t   = NTSTATUS(WINAPI*)(INT, PVOID, ULONG);

	NtSetSystemInformation_t         NtSetSystemInformation;
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