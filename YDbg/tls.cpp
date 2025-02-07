#include "stdafx.h"

#include "tls.h"

_Y_THREAD_STATE::_Y_THREAD_STATE(YMessageLoop* loop) : _M_loop(loop)
{
}

_Y_THREAD_STATE::~_Y_THREAD_STATE()
{
	if (_M_buf)
	{
		delete [] _M_buf;
	}

	if (_M_hFont)
	{
		DeleteObject(_M_hFont);
	}
}

BOOL _Y_THREAD_STATE::Init()
{
	if (!(_M_buf = new UCHAR[cbBuf]))
	{
		return FALSE;
	}

	NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
	{
		ncm.lfCaptionFont.lfHeight = -ncm.iMenuHeight;
		ncm.lfCaptionFont.lfWeight = FW_NORMAL;
		ncm.lfCaptionFont.lfQuality = CLEARTYPE_QUALITY;
		ncm.lfCaptionFont.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;
		wcscpy(ncm.lfCaptionFont.lfFaceName, L"Courier New");

		_M_hFont = CreateFontIndirect(&ncm.lfCaptionFont);
	}

	return TRUE;
}

PVOID _Y_THREAD_STATE::_S_GetLocalBuf(ULONG& cb)
{
	if (_Y_THREAD_STATE* state = Y_THREAD_STATE::get())
	{
		return state->GetLocalBuf(cb);
	}

	return 0;
}

YMessageLoop* _Y_THREAD_STATE::_S_GetMsgLoop()
{
	if (_Y_THREAD_STATE* state = Y_THREAD_STATE::get())
	{
		return state->_M_loop;
	}

	return 0;
}
