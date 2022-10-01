#include <Windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <thread>
#include "wndproc.h"
#include "resource.h"
#include "win32utility.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' \
						 name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
						 processorArchitecture='*' publicKeyToken='6595b64144ccf1df' \
						 language='*'\"")


extern win32SystemManager&  systemMgr;
extern memcleanManager&     cleanMgr;

void ShowAbout() {
	MessageBox(0,
		VERSION "更新内容：集成目前已有的所有清理内存方法，包括Memreduct使用的全部方法。\n\n"
		"说明：功能按原 Memory Cleaner 复刻，删除原版的强制联网更新导致打不开。\n\n"
		"一般勾上80%的就行了，如果觉得不够再勾上5分钟的。\n若内存够用就不要频繁清理，否则反而会导致系统性能下降。\n\n"
		"裁剪进程工作集 = Trim Processes' Working Set\n"
		"清理系统缓存 = Clear System Cache\n"
		"执行全部已知清理：可以开游戏前手动点一下；如果开了游戏再点可能短暂卡顿。\n\n"
		"  原作：Koshy John && henry++\n  重制：H3d9",
		"Memory Cleaner 重制版  RemasteRed by: @H3d9",
		MB_OK);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {

		case WM_INITDIALOG:
		{
			cleanMgr.hDlg = hDlg;

			auto icon = LoadIcon(systemMgr.hInstance, MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)icon);

			cleanMgr.hwndPB = GetDlgItem(hDlg, IDC_PROGRESS1);
			SendMessage(cleanMgr.hwndPB, PBM_SETRANGE, 0, MAKELPARAM(0, 16384));
			SendMessage(cleanMgr.hwndPB, PBM_SETSTEP, (WPARAM)1, 0);

			RECT rect;
			GetWindowRect(hDlg, &rect);
			SetWindowPos(hDlg, HWND_TOP,
				GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left) - 60,
				GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top) - 60,
				0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);

			CheckDlgButton(hDlg, IDC_CHECK2, cleanMgr.memCleanSwitches[0]);
			CheckDlgButton(hDlg, IDC_CHECK4, cleanMgr.memCleanSwitches[1]);
			CheckDlgButton(hDlg, IDC_CHECK5, cleanMgr.memCleanSwitches[2]);
			CheckDlgButton(hDlg, IDC_CHECK1, cleanMgr.memCleanSwitches[3]);
			CheckDlgButton(hDlg, IDC_CHECK3, cleanMgr.memCleanSwitches[4]);
			CheckDlgButton(hDlg, IDC_CHECK6, cleanMgr.memCleanSwitches[5]);
			CheckDlgButton(hDlg, IDC_CHECK7, cleanMgr.autoStart);
			CheckDlgButton(hDlg, IDC_CHECK8, cleanMgr.bruteMode);


			std::thread t([] () {

				while (cleanMgr.hDlg) {

					wchar_t buffer[1024];


					ULONGLONG totalVirtualMem = 0;
					ULONGLONG usedVirtualMem = 0;
					ULONGLONG totalPhysMem = 0;
					ULONGLONG usedPhysMem = 0;
					double physPercent = 0.0;
					double virtPercent = 0.0;

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
					physPercent = (double)usedPhysMem / totalPhysMem;
					virtPercent = (double)usedVirtualMem / totalVirtualMem;
					

					SIZE_T usedCache = 0;
					SIZE_T totalCache = 0;
					double cachePercent = 0.0;

					cleanMgr.getSystemCacheInfo(&usedCache, &totalCache);
					usedCache /= (1024 * 1024);
					totalCache /= (1024 * 1024);
					cachePercent = (double)usedCache / totalCache;


					swprintf(buffer, L"物理内存用量：%llu MB  /  %llu MB   (%.0f%% 负载)", usedPhysMem, totalPhysMem, physPercent * 100);
					SetDlgItemTextW(cleanMgr.hDlg, IDC_STATIC1, buffer);

					swprintf(buffer, L"页面文件：%llu MB  (%.0f%%)    系统缓存：%llu MB / %llu MB  (%.0f%%)", usedVirtualMem, virtPercent * 100, usedCache, totalCache, cachePercent * 100);
					SetDlgItemTextW(cleanMgr.hDlg, IDC_STATIC2, buffer);


					SendMessage(cleanMgr.hwndPB, PBM_SETPOS, (WPARAM)(ULONGLONG)((ULONGLONG)16384 * physPercent), 0);

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

				cleanMgr.flushSystemBuffer();
				SetDlgItemText(hDlg, IDC_STATIC1, "已清除系统缓存。");

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_BUTTON3) {

				if (MessageBox(hDlg, "如果你开游戏了，不建议点这个，否则清理时和清理后一小段时间有可能游戏变卡", "提示", MB_OKCANCEL | MB_SYSTEMMODAL) == IDOK) {
					
					cleanMgr.cleanMemory_all();
					SetDlgItemText(hDlg, IDC_STATIC1, "已执行其他任何可能的内存清理。");
				}

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK2) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK2);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[0] = checked;
				}
				cleanMgr.savecfg();
				
				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK4) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK4);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[1] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK5) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK5);
				if (checked == BST_CHECKED) {
					MessageBox(hDlg, "如果你开游戏了，不建议点这个，否则清理时和清理后一小段时间有可能游戏变卡", "提示", MB_OK | MB_SYSTEMMODAL);
					cleanMgr.memCleanSwitches[2] = checked;
				} else if (checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[2] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK1) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK1);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[3] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK3) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK3);
				if (checked == BST_CHECKED || checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[4] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK6) {
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK6);
				if (checked == BST_CHECKED) {
					MessageBox(hDlg, "如果你开游戏了，不建议点这个，否则清理时和清理后一小段时间有可能游戏变卡", "提示", MB_OK | MB_SYSTEMMODAL);
					cleanMgr.memCleanSwitches[5] = checked;
				} else if (checked == BST_UNCHECKED) {
					cleanMgr.memCleanSwitches[5] = checked;
				}
				cleanMgr.savecfg();

				return (INT_PTR)TRUE;

			} else if (LOWORD(wParam) == IDC_CHECK7) { // autostart

				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK7);
				if (checked == BST_CHECKED) {
					
					char exePath[1024] = {}; // "\""
					GetModuleFileName(NULL, exePath, 1024); // + 1
					strcat(exePath, " slient"); // \" --
					
					HKEY hKey;
					if (RegOpenKeyEx(HKEY_CURRENT_USER, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
						RegSetValueEx(hKey, "memclean", 0, REG_SZ, (const BYTE*)exePath, (DWORD)(strlen(exePath) + 1));
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
				auto checked = IsDlgButtonChecked(hDlg, IDC_CHECK8);
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
			EndDialog(hDlg, LOWORD(wParam));
			cleanMgr.hDlg = NULL;
			return (INT_PTR)TRUE;
		}
		break;
	}

	return (INT_PTR) FALSE;
}

static INT_PTR CALLBACK DlgProc2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message) {

	case WM_INITDIALOG:
	{
		SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
		return (INT_PTR)TRUE;
	}

	case WM_COMMAND:
	{
		if (LOWORD(wParam) == IDC_AFDIAN) {
			ShellExecute(0, "open", "https://afdian.net/a/sguard_limit", 0, 0, SW_SHOW);
		}
	}
	break;

	case WM_CLOSE:
	{
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
	}
	break;
	}

	return (INT_PTR)FALSE;
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
			AppendMenu(hMenu, MFT_STRING, IDM_OPEN, "Memory Cleaner 重制版");
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hMenu, MFT_STRING, IDM_ABOUT, "查看说明");
			AppendMenu(hMenu, MFT_STRING, IDM_SOURCE, "查看源代码");
			AppendMenu(hMenu, MFT_STRING, IDM_DONATE, "赞助支持");
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
			AppendMenu(hMenu, MFT_STRING, IDM_WORKINGSET, "清理进程工作集");
			AppendMenu(hMenu, MFT_STRING, IDM_SYSCACHE, "清理系统缓存");
			AppendMenu(hMenu, MFT_STRING, IDM_CLEANALL, "执行全部已知清理");
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
			case IDM_CLEANALL:
				cleanMgr.cleanMemory_all();
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
			case IDM_DONATE:
				DialogBox(systemMgr.hInstance, MAKEINTRESOURCE(IDD_DONATE), NULL, DlgProc2);
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