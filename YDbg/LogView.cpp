#include "stdafx.h"

#include "obj.h"
#include "loop.h"
#include "dll.h"
#include "thread.h"
#include "process.h"

#include "MiniWnd.h"
#include "LogView.h"
#include "tls.h"
#include "msgbox.h"

void EmptyPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	if (BeginPaint(hwnd, &ps))
	{
		EndPaint(hwnd, &ps);
	}
}

LRESULT YLogFrame::DefWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
}

LRESULT YLogFrame::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		_M_hwndEdit = 0;
		OnNcDestroy();
		break;

	case WM_NCCREATE:
		if (!OnCreate(hwnd))
		{
			return FALSE;
		}
		break;

	case WM_SIZE:
		OnSize(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_DETACH:
		Detach_I();
		return 0;

	case WM_CLOSE:
		if (_M_process)
		{
			WCHAR sz[32];
			swprintf_s(sz, _countof(sz), L"[%x]", _M_process->getId());
			if (CustomMessageBox(hwnd, sz, L"Detach ?", MB_ICONQUESTION|MB_DEFBUTTON2|MB_YESNO) == IDYES)
			{
				_M_process->Rundown();
			}
			return 0;
		}
		break;

	case WM_PAINT:
		EmptyPaint(hwnd);
		break;

	case WM_ERASEBKGND:
		return TRUE;
	}

	return DefWinProc(hwnd, uMsg, wParam, lParam);
}

YLogFrame::YLogFrame(YProcess* process)
{
	_M_process = process;
	process->AddRef();
}

YLogFrame::~YLogFrame()
{
	Detach_I();
}

void YLogFrame::Detach_I()
{
	if (_M_process)
	{
		_M_process->Release();
		_M_process = 0;
	}
}

void YLogFrame::Detach(HWND hwndEdit)
{
	if (hwndEdit = GetParent(hwndEdit))
	{
		SendMessageW(hwndEdit, WM_DETACH, 0, 0);
	}
}

BOOL YLogFrame::OnCreate(HWND hwnd)
{
	RECT rc;
	if (GetClientRect(hwnd, &rc))
	{
		if (hwnd = CreateWindowEx(0, WC_EDIT, 0, WS_VISIBLE|WS_CHILD|
			WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_WANTRETURN, 0, 0, rc.right, rc.bottom, hwnd, 0, 0, 0))
		{
			_M_hwndEdit = hwnd;
			if (_Y_THREAD_STATE* state = Y_THREAD_STATE::get())
			{
				SendMessage(hwnd, WM_SETFONT, (WPARAM)state->getFont(), 0);
			}
			SendMessage(hwnd, EM_LIMITTEXT, MAXLONG, 0);
			return TRUE;
		}
	}

	return 0;
}

HWND YLogFrame::CreateLog(YProcess* process)
{
	if (_Y_THREAD_STATE* state = Y_THREAD_STATE::get())
	{
		if (YLogFrame* pLog = new YLogFrame(process))
		{
			WCHAR sz[32];
			swprintf_s(sz, _countof(sz), L"%x(%u)", process->getId(), process->getId());

			pLog->MdiChildCreate(WS_EX_MDICHILD|WS_EX_CLIENTEDGE, sz, 
				WS_VISIBLE|WS_OVERLAPPEDWINDOW|WS_MAXIMIZE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|MDIS_ALLCHILDSTYLES, 
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, state->_M_hWndMDIClient, 0);

			HWND hwnd = pLog->_M_hwndEdit;
			pLog->Release();

			return hwnd;
		}
	}

	return 0;
}

void YLogFrame::OnSize(WPARAM wParam, ULONG cx, ULONG cy)
{
	switch (wParam)
	{
	case SIZE_RESTORED:
	case SIZE_MAXIMIZED:
		MoveWindow(_M_hwndEdit, 0, 0, cx, cy, FALSE);
		break;
	}
}