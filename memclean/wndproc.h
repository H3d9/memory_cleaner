#pragma once
#include <Windows.h>

#define VERSION   "22.10.1"

#define WM_TRAYACTIVATE     WM_APP + 10U

#define IDM_WORKINGSET     200
#define IDM_SYSCACHE       201
#define IDM_CLEANALL       202

#define IDM_OPEN           203
#define IDM_EXIT           204
#define IDM_ABOUT          205
#define IDM_SOURCE         206
#define IDM_DONATE         207


INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);