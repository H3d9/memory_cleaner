#include <Windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <thread>
#include "wndproc.h"
#include "resource.h"
#include "win32utility.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' \
						 name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
						 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' \
						 language='*'\"")



extern win32SystemManager& systemMgr;
extern memcleanManager& cleanMgr;


void ShowAbout() {
	MessageBox(0,
		VERSION "更新内容：新增强力模式，开启后可清SGUARD内存\n\n"
		"说明：功能按原 Memory Cleaner 复刻，删除联网更新。\n\n"
		"提示：一般勾上80%的就行了，如果觉得不够再勾上5分钟的。\n若内存够用就不要频繁清理，否则反而会导致系统性能下降。\n\n"
		"裁剪进程工作集 = Trim Processes' Working Set\n"
		"清理系统缓存 = Clear System Cache\n"
		"清理StandbyList：新增，用后短时间玩游戏会卡\n（内存实在不够的话可手动点这个，过几分钟出优化效果）\n\n"
		"  原作：Koshy John\n   破解 & 重写：H3d9",
		"Memory Cleaner 免更新重制版  RemasteRed by: @H3d9",
		MB_OK);
}


INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {

		case WM_INITDIALOG:
		{
			cleanMgr.hDlg = hDlg;

			auto icon = LoadIcon(systemMgr.hInstance, MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon);

			RECT rect;
			GetClientRect(hDlg, &rect);
			
			cleanMgr.hwndPB = CreateWindowEx(0, PROGRESS_CLASS, (LPTSTR)NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
				rect.left + 60, rect.top + 40, rect.right - 80, 35,
				hDlg, (HMENU)0, systemMgr.hInstance, NULL);
			
			if (!cleanMgr.hwndPB) {
				systemMgr.panic("CreateWindowEx failed");
				return FALSE;
			}
			
			SendMessage(cleanMgr.hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 16384));
			SendMessage(cleanMgr.hwndPB, PBM_SETSTEP, (WPARAM)1, 0);

			GetWindowRect(cleanMgr.hDlg, &rect);
			SetWindowPos(cleanMgr.hDlg, HWND_TOP,
				GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left) - 60,
				GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top) - 60,
				0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK2, cleanMgr.memCleanSwitches[0]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK4, cleanMgr.memCleanSwitches[1]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK5, cleanMgr.memCleanSwitches[2]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK1, cleanMgr.memCleanSwitches[3]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK3, cleanMgr.memCleanSwitches[4]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK6, cleanMgr.memCleanSwitches[5]);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK7, cleanMgr.autoStart);
			CheckDlgButton(cleanMgr.hDlg, IDC_CHECK8, cleanMgr.bruteMode);


			std::thread t([] () {

				while (cleanMgr.hDlg) {
					
					ULONGLONG totalVirtualMem = 0;
					ULONGLONG usedVirtualMem = 0;
					ULONGLONG totalPhysMem = 0;
					ULONGLONG usedPhysMem = 0;
					double usedPercent = 0.0;

					MEMORYSTATUSEX memInfo;
					memset(&memInfo, 0, sizeof(memInfo));
					memInfo.dwLength = sizeof(MEMORYSTATUSEX);
					GlobalMemoryStatusEx(&memInfo);
					totalVirtualMem = memInfo.ullTotalPageFile;
					usedVirtualMem = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
					totalPhysMem = memInfo.ullTotalPhys;
					usedPhysMem = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
					totalVirtualMem /= (1024 * 1024);
					usedVirtualMem /= (1024 * 1024);
					totalPhysMem /= (1024 * 1024);
					usedPhysMem /= (1024 * 1024);
					usedPercent = (double)usedPhysMem / totalPhysMem;

					char buffer[1024];
					sprintf(buffer, "物理内存用量：%llu MB  /  %llu MB   (%.0f%% 负载)", usedPhysMem, totalPhysMem, usedPercent * 100);
					SetDlgItemText(cleanMgr.hDlg, IDC_STATIC1, buffer);

					sprintf(buffer, "页面文件用量：%llu MB               虚拟内存: %llu MB ", usedVirtualMem, totalVirtualMem);
					SetDlgItemText(cleanMgr.hDlg, IDC_STATIC2, buffer);

					SendMessage(cleanMgr.hwndPB, PBM_SETPOS, (WPARAM)(ULONGLONG)((ULONGLONG)16384 * usedPercent), 0);

					Sleep(1000);
				}
				
			});
			t.detach();

			return (INT_PTR)TRUE;
		}

		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDC_BUTTON1) {

				if (cleanMgr.bruteMode) {
					cleanMgr.trimProcessWorkingSet2();
				} else {
					cleanMgr.trimProcessWorkingSet();
				}

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_BUTTON2) {

				char buffer[1024];
				auto result = cleanMgr.flushSystemBuffer();
				
				if (result >= 0) {
					strcpy(buffer, "已清除系统缓存。");
				} else {
					sprintf(buffer, "清除系统缓存时发生错误：%x。", result);
				}
				SetDlgItemText(cleanMgr.hDlg, IDC_STATIC1, buffer);

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_BUTTON3) {

				char buffer[1024];
				auto result = cleanMgr.purgeMemoryStandbyList();

				if (result >= 0) {
					sprintf(buffer, "已清空内存等待链。");
				} else {
					sprintf(buffer, "清空内存等待链时发生错误：%x。", result);
				}
				SetDlgItemText(cleanMgr.hDlg, IDC_STATIC1, buffer);

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK2) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK2);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[0] = checked;
				}
				cleanMgr.savecfg();
				
				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK4) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK4);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[1] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK5) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK5);
				if (checked == BST_CHECKED) {
					MessageBox(hDlg, "自动清理“内存等待链”的时候可能导致游戏卡一下", "提示", MB_OK | MB_SYSTEMMODAL);
					cleanMgr.memCleanSwitches[2] = checked;
				} else if (checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[2] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK1) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK1);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[3] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK3) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK3);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[4] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK6) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK6);
				if (checked == BST_CHECKED) {
					MessageBox(hDlg, "自动清理“内存等待链”的时候可能导致游戏卡一下", "提示", MB_OK | MB_SYSTEMMODAL);
					cleanMgr.memCleanSwitches[5] = checked;
				} else if (checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[5] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK7) { // autostart

				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK7);
				if (checked == BST_CHECKED) {
					
					char exePath[1024] = {}; // "\""
					GetModuleFileName(NULL, exePath, 1024); // + 1
					strcat(exePath, " slient"); // \" --
					
					HKEY hKey;
					if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
						RegSetValueEx(hKey, "memclean", 0, REG_SZ, (const BYTE*)exePath, strlen(exePath) + 1);
						RegCloseKey(hKey);
					} else {
						systemMgr.panic("RegOpenKeyEx failed");
					}

					cleanMgr.autoStart = true;

				} else if (checked == BST_UNCHECKED) {
					
					HKEY hKey;
					if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
						RegDeleteValue(hKey, "memclean");
						RegCloseKey(hKey);
					} else {
						systemMgr.panic("RegOpenKeyEx failed");
					}

					cleanMgr.autoStart = false;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK8) {
				auto checked = IsDlgButtonChecked(cleanMgr.hDlg, IDC_CHECK8);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.bruteMode = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			}
		}
		break;

		case WM_CLOSE:
		{
			DestroyWindow(cleanMgr.hwndPB);
			cleanMgr.hwndPB = NULL;
			EndDialog(cleanMgr.hDlg, LOWORD(wParam));
			cleanMgr.hDlg = NULL;
			return (INT_PTR)TRUE;
		}
		break;
	}

	return (INT_PTR) FALSE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	static UINT msg_taskbarRestart;

	switch (msg) {
	case WM_CREATE:
		msg_taskbarRestart = RegisterWindowMessage("TaskbarCreated");
	break;
	case WM_TRAYACTIVATE:
	{
		if (lParam == WM_LBUTTONDBLCLK) {
			if (!cleanMgr.hDlg) {
				DialogBox(systemMgr.hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
			}
		}

		if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {

			HMENU hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MFT_STRING, IDM_OPEN, "打开主界面");
			AppendMenu(hMenu, MFT_STRING, IDM_ABOUT, "查看说明");
			AppendMenu(hMenu, MFT_STRING, IDM_SOURCE, "查看源代码");
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hMenu, MFT_STRING, IDM_WORKINGSET, "清理进程工作集");
			AppendMenu(hMenu, MFT_STRING, IDM_SYSCACHE, "清理系统缓存");
			AppendMenu(hMenu, MFT_STRING, IDM_STANDBYLIST, "清理内存等待链");
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hMenu, MFT_STRING, IDM_EXIT, "退出");

			POINT pt;
			GetCursorPos(&pt);
			SetForegroundWindow(hWnd);
			TrackPopupMenu(hMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
			DestroyMenu(hMenu);
		}
	}
	break;
	case WM_COMMAND:
	{
		UINT id = LOWORD(wParam);

		switch (id) {
			case IDM_WORKINGSET:
				if (cleanMgr.bruteMode) {
					cleanMgr.trimProcessWorkingSet2();
				} else {
					cleanMgr.trimProcessWorkingSet();
				}
				break;
			case IDM_SYSCACHE:
				cleanMgr.flushSystemBuffer();
				break;
			case IDM_STANDBYLIST:
				cleanMgr.purgeMemoryStandbyList();
				break;

			case IDM_OPEN:
				if (!cleanMgr.hDlg) {
					DialogBox(systemMgr.hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
				}
				break;
			case IDM_ABOUT:
				ShowAbout();
				break;
			case IDM_EXIT:
				PostMessage(hWnd, WM_CLOSE, 0, 0);
				break;
			case IDM_SOURCE:
				ShellExecute(0, "open", "https://github.com/H3d9/memory_cleaner", 0, 0, SW_SHOW);
				break;
		}
	}
	break;
	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
	}
	break;
	case WM_DESTROY:
	{
		PostQuitMessage(0);
	}
	break;
	default:
		if (msg == msg_taskbarRestart) {
			systemMgr.createTray(WM_TRAYACTIVATE);
		}
	break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}