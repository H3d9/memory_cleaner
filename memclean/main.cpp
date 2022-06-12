#include <Windows.h>
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


	systemMgr.setupProcessDpi();

	systemMgr.enableDebugPrivilege();

	if (!systemMgr.checkDebugPrivilege()) {
		return -1;
	}

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

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	auto result =
	systemMgr.messageLoop();

	systemMgr.removeTray();

	return (INT) result;
}