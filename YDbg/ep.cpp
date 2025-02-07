#include "stdafx.h"

#include "..\include\initterm.h"

#include "loop.h"
#include "obj.h"
#include "dll.h"
#include "thread.h"
#include "process.h"
#include "dbg.h"
#include "tls.h"
#include "MiniWnd.h"
#include "resource.h"
#include "msgbox.h"

class YMainFrame : public YWnd
{
	HWND _M_hWndMDIClient = 0;
	YDbg* _M_pDbg;

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT DefWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefFrameProc(hwnd, _M_hWndMDIClient, uMsg, wParam, lParam);
	}

	BOOL OnCreate(HWND hwnd);
	BOOL OnClose(HWND hwnd);

	void OnSize(WPARAM wParam, ULONG cx, ULONG cy);

public:

	YMainFrame(YDbg* pDbg) : _M_pDbg(pDbg)
	{
	}
};

void YMainFrame::OnSize(WPARAM wParam, ULONG cx, ULONG cy)
{
	switch (wParam)
	{
	case SIZE_RESTORED:
	case SIZE_MAXIMIZED:
		MoveWindow(_M_hWndMDIClient, 0, 0, cx, cy, FALSE);
		break;
	}
}

#define AFX_IDW_RESIZE_BAR 0xE803
#define AFX_IDM_FIRST_MDICHILD 0xFF00

BOOL YMainFrame::OnCreate(HWND hwnd)
{
	RECT rc;
	if (GetClientRect(hwnd, &rc))
	{
		CLIENTCREATESTRUCT ccs = { GetSubMenu(GetMenu(hwnd), 1), AFX_IDM_FIRST_MDICHILD };

		if (_M_hWndMDIClient = CreateWindowEx(WS_EX_CLIENTEDGE, L"MDICLIENT", 0, 
			WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE, 
			0, 0, rc.right, rc.bottom, hwnd, (HMENU)AFX_IDW_RESIZE_BAR, 0, &ccs))
		{
			if (_Y_THREAD_STATE* state = Y_THREAD_STATE::get())
			{
				state->_M_hWndMDIClient = _M_hWndMDIClient;
				state->_M_hwndTop = hwnd;
				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOL YMainFrame::OnClose(HWND hwnd)
{
	if (_M_pDbg->IsEmpty())
	{
		return TRUE;
	}

	CustomMessageBox(hwnd, L"stop debugging first", 0, MB_ICONINFORMATION|MB_OK);
	return FALSE;
}

LRESULT YMainFrame::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		PostQuitMessage(0);
		OnNcDestroy();
		break;

	case WM_PAINT:
		EmptyPaint(hwnd);
		break;

	case WM_CREATE:
		if (!OnCreate(hwnd))
		{
			return -1;
		}
		break;

	case WM_CLOSE:
		if (!OnClose(hwnd))
		{
			return 0;
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case ID_FILE_EXIT40005:
			if (!OnClose(hwnd))
			{
				return 0;
			}
			uMsg = WM_CLOSE;
			break;
		}
		break;

	case WM_ERASEBKGND:
		RECT rc;
		if (GetClientRect(hwnd, &rc))
		{
			FillRect((HDC)wParam, &rc, (HBRUSH)(1 + COLOR_INFOBK));
		}
		return TRUE;
	}

	return DefWinProc(hwnd, uMsg, wParam, lParam);
}

void ymain()
{
	YMessageLoop loop;
	Y_THREAD_STATE state(&loop);

	if (state.Init())
	{
		YDbg dbg;
		if (dbg.Init(&loop))
		{
			YMainFrame frame(&dbg);
			if (HMENU hmenu = LoadMenuW((HINSTANCE)&__ImageBase, MAKEINTRESOURCEW(IDR_MENU1)))
			{
				if (frame.Create(0, L"YDbg", WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_VISIBLE, 
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, hmenu))
				{
					loop.Run();
				}
				DestroyMenu(hmenu);
			}
		}
	}
}

void WINAPI ep(void*)
{
	initterm();
	
	BOOLEAN b;
	if (STATUS_SUCCESS == RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, &b))
	{
		if (YWnd::Register())
		{
			ymain();
			YWnd::Unregister();
		}
	}

	destroyterm();

	ExitProcess(0);
}

