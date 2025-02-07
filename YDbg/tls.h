#pragma once

class YMessageLoop;

#include "frame.h"

struct _Y_THREAD_STATE
{
	enum { cbBuf = 0x10000 };
	PVOID _M_buf = 0;
	YMessageLoop* _M_loop;
	HWND _M_hwndTop = 0, _M_hWndMDIClient = 0;
	HFONT _M_hFont = 0;

	_Y_THREAD_STATE(YMessageLoop* loop);
	_Y_THREAD_STATE() = default;

	~_Y_THREAD_STATE();

	BOOL Init();

	HFONT getFont()
	{
		return _M_hFont;
	}

	PVOID GetLocalBuf(ULONG& cb)
	{
		cb = cbBuf;
		return _M_buf;
	}

	static PVOID _S_GetLocalBuf(ULONG& cb);

	YMessageLoop* GetMsgLoop()
	{
		return _M_loop;
	}

	static YMessageLoop* _S_GetMsgLoop();
};

using Y_THREAD_STATE = RTL_FRAME<_Y_THREAD_STATE>;