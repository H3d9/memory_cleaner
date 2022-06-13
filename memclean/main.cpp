#include <Windows.h>
#include <Shlobj.h>
#include <thread>
#include "resource.h"
#include "wndproc.h"
#include "win32utility.h"


win32SystemManager& systemMgr = win32SystemManager::getInstance();
memcleanManager& cleanMgr = memcleanManager::getInstance();


INT WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd) {


	if (!IsUserAnAdmin()) { // 手动触发UAC，不然不能开机自启
		char path[1024];
		GetModuleFileName(NULL, path, 1024);
		ShellExecute(NULL, "runas", path, lpCmdLine, NULL, SW_SHOWNORMAL);
		return 0;
	}

	systemMgr.setupProcessDpi();

	if (!systemMgr.systemInit(hInstance)) {
		return -1;
	}

	if (!systemMgr.createWindow(WndProc, IDI_ICON1)) {
		return -1;
	}

	systemMgr.createTray(WM_TRAYACTIVATE);

	cleanMgr.init();

	if (!cleanMgr.loadcfg()) {
		extern void ShowAbout();
		ShowAbout();
	}

	cleanMgr.raiseMemCleanThread();

	if (0 == strstr(lpCmdLine, "slient")) {
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	}

	auto result =
	systemMgr.messageLoop();

	systemMgr.removeTray();

	return (INT) result;
}