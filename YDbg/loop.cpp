#include "stdafx.h"

#include "loop.h"

BOOL IsDialogMessageEx(PMSG lpMsg)
{
	if (HWND hwnd = lpMsg->hwnd)
	{
		do 
		{
			if (GetClassLongW(hwnd, GCW_ATOM) == (ULONG)(ULONG_PTR)WC_DIALOG)
			{
				return IsDialogMessage(hwnd, lpMsg);
			}
		} while (hwnd = GetParent(hwnd));
	}

	return FALSE;
}

BOOL YMessageLoop::IsIdleMessage(UINT uMsg)
{
	switch (uMsg)
	{
	case 0x118:
	case WM_TIMER:
	case WM_PAINT:
	case WM_MOUSEMOVE:
	case WM_NCMOUSEMOVE:
	case WM_MOUSELEAVE:
	case WM_MOUSEHOVER:
	case WM_NCMOUSELEAVE:
	case WM_NCMOUSEHOVER:
		return TRUE;
	}

	return FALSE;
}

void YMessageLoop::InsertTM(PLIST_ENTRY entry)
{
	InsertHeadList(&_headTM, entry);
}

void YMessageLoop::InsertIdle(PLIST_ENTRY entry)
{
	InsertHeadList(&_headIdle, entry);
}

BOOL YMessageLoop::PreTranslateMessage(PMSG lpMsg)
{
	PLIST_ENTRY head = &_headTM, entry = head;

	while ((entry = entry->Flink) != head)
	{
		if (static_cast<YTranslateMsg*>(entry)->PreTranslateMessage(lpMsg))
		{
			return TRUE;
		}
	}

	return FALSE;
}

void YMessageLoop::OnIdle()
{
	PLIST_ENTRY head = &_headIdle, entry = head;

	while ((entry = entry->Flink) != head)
	{
		static_cast<YIdle*>(entry)->OnIdle();
	}
}

WPARAM YMessageLoop::Run()
{
	for (;;)
	{
		HANDLE* pHandles;

		ULONG nCount = GetWaitHandles(&pHandles);

		ULONG r = MsgWaitForMultipleObjectsEx(nCount, pHandles, GetTimeout(), QS_ALLINPUT, MWMO_ALERTABLE|MWMO_INPUTAVAILABLE);

		if (r == nCount)
		{
			BOOL bIdle = TRUE;

			MSG msg;

			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				if (bIdle)
				{
					bIdle = IsIdleMessage(msg.message);
				}

				if (PreTranslateMessage(&msg)) continue;

				if (WM_QUIT == msg.message) 
				{
					return msg.wParam;
				}

				if (!IsDialogMessageEx(&msg))
				{
					if (msg.message - WM_KEYFIRST <= WM_KEYLAST - WM_KEYFIRST)
					{
						TranslateMessage(&msg);
					}
					DispatchMessage(&msg);
				}
			}

			if (bIdle)
			{
				OnIdle();
			}

			continue;
		}

		if (r < nCount)
		{
			OnSignalObject(r);
			continue;
		}

		switch(r)
		{
		case STATUS_USER_APC:
			OnApcAlert();
			continue;
		case STATUS_TIMEOUT:
			OnTimeout();
			continue;
		}

		if ((r -= STATUS_ABANDONED) < nCount)
		{
			OnAbandoned(r);
			continue;
		}

		__debugbreak();
	}
}

void YMessageLoop::OnTimeout()
{
}

ULONG YMessageLoop::GetTimeout()
{
	return INFINITE;
}

void YMessageLoop::OnApcAlert()
{
}

BOOL YMessageLoop::CanClose(HWND /*hwnd*/)
{
	return TRUE;
}

BOOL YMessageLoop::addWaitObject(YSignalObject* pObject, HANDLE hObject)
{
	if (_nCount < _countof(_Handles))
	{
		_Handles[_nCount] = hObject;
		_objects[_nCount++] = pObject;

		return TRUE;
	}

	return FALSE;
}

BOOL YMessageLoop::delWaitObject(YSignalObject* pObject)
{
	if (ULONG nCount = _nCount)
	{
		YSignalObject** ppObjects = _objects;
		HANDLE* pHandles = _Handles;
		do 
		{
			--nCount;
			if (*ppObjects == pObject)
			{
				if (nCount)
				{
					memcpy(ppObjects, ppObjects + 1, nCount * sizeof(PVOID));
					memcpy(pHandles,  pHandles  + 1, nCount * sizeof(PVOID));
				}
				ppObjects[nCount] = 0, pHandles[nCount] = 0;
				_nCount--;
				return TRUE;
			}
		} while (ppObjects++, pHandles++, nCount);
	}

	return FALSE;
}

ULONG YMessageLoop::GetWaitHandles(HANDLE ** ppHandles)
{
	*ppHandles = _Handles;
	return _nCount;
}

void YMessageLoop::OnSignalObject(ULONG i)
{
	if (i < _nCount)
	{
		_objects[i]->OnSignal();
	}
}

void YMessageLoop::OnAbandoned(ULONG i)
{
	if (i < _nCount)
	{
		_objects[i]->OnSignal();
	}
}

YMessageLoop::~YMessageLoop()
{
	if (ULONG nCount = _nCount)
	{
		_nCount = 0;

		YSignalObject** ppObjects = _objects;
		do 
		{
			(*ppObjects++)->OnStop();
		} while (--nCount);
	}
}