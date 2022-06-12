#pragma once
#include <Windows.h>


#define WM_TRAYACTIVATE     WM_APP + 10U

#define IDM_OPEN           200
#define IDM_EXIT           201
#define IDM_ABOUT          202
#define IDM_SOURCE         203


INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);