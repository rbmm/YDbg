#include "stdafx.h"

#include "loop.h"
#include "obj.h"
#include "dll.h"
#include "thread.h"
#include "process.h"
#include "dbg.h"
#include "tls.h"

int ShowMessage(_In_opt_ PCWSTR lpText,
				_In_opt_ PCWSTR lpCaption,
				_In_ UINT uType)
{
	GUITHREADINFO gui = { sizeof(gui) };
	if (GetGUIThreadInfo(GetCurrentThreadId(), &gui))
	{
		SetForegroundWindow(gui.hwndActive);
	}
	return MessageBoxW(gui.hwndActive, lpText, lpCaption, uType);
}

void OnRemoteSectionMapped(DBGUI_WAIT_STATE_CHANGE& StateChange)
{
	PWSTR szDllDame = 0;

	if (HANDLE hFile = StateChange.LoadDll.FileHandle)
	{
		ULONG cb;
		union {
			PVOID buf;
			POBJECT_NAME_INFORMATION poni;
		};

		if (buf = Y_THREAD_STATE::_S_GetLocalBuf(cb))
		{
			NTSTATUS status = NtQueryObject(hFile, ObjectNameInformation, poni, cb - sizeof(WCHAR), &cb);

			if (0 <= status)
			{
				*(PWSTR)RtlOffsetToPointer(szDllDame = poni->Name.Buffer, poni->Name.Length) = 0;
			}
		}

		NtClose(hFile);
	}

	WCHAR sz[128];
	swprintf_s(sz, _countof(sz), L"remote section mapped by %x.%x at %p", 
		PtrToUlong(StateChange.AppClientId.UniqueProcess), 
		PtrToUlong(StateChange.AppClientId.UniqueThread), StateChange.LoadDll.BaseOfDll);

	ShowMessage(szDllDame, sz, MB_ICONWARNING);
}

void OnRemoteSectionUnMapped(DBGUI_WAIT_STATE_CHANGE& StateChange)
{
	WCHAR sz[64];
	swprintf_s(sz, _countof(sz), L"by %x.%x at %p", 
		PtrToUlong(StateChange.AppClientId.UniqueProcess), 
		PtrToUlong(StateChange.AppClientId.UniqueThread), StateChange.UnloadDll.BaseAddress);

	ShowMessage(sz, L"remote section Unmapped", MB_ICONWARNING);
}

void OnUnWaited(DBGUI_WAIT_STATE_CHANGE& StateChange)
{
	WCHAR sz[128];
	swprintf_s(sz, _countof(sz), L"code=%u pid=%x tid=%x", StateChange.NewState, 
		PtrToUlong(StateChange.AppClientId.UniqueProcess), 
		PtrToUlong(StateChange.AppClientId.UniqueThread));
	
	ShowMessage(sz, L"Unwaited debug event !!", MB_ICONHAND);
}

void YDbg::OnSignal()
{
	LARGE_INTEGER Timeout = {};
	DBGUI_WAIT_STATE_CHANGE StateChange;

	while (!DbgUiWaitStateChange(&StateChange, &Timeout))
	{
		YProcess* process;
		PLIST_ENTRY head = &_M_Processes, entry = head;

		while ((entry = entry->Flink) != head)
		{
			process = static_cast<YProcess*>(entry);

			if (process->getId() == PtrToUlong(StateChange.AppClientId.UniqueProcess))
			{
__de:
				process->AddRef();
				process->OnDebugEvent(StateChange);
				process->Release();
				return;
			}
		}

		switch (StateChange.NewState)
		{
		case DbgCreateProcessStateChange:
			if (process = new YProcess(TRUE))
			{
				InsertHeadList(&_M_Processes, process);
				goto __de;
			}
			break;

		case DbgLoadDllStateChange:
			OnRemoteSectionMapped(StateChange);
			break;

		case DbgUnloadDllStateChange:
			OnRemoteSectionUnMapped(StateChange);
			break;

		case DbgCreateThreadStateChange:
			if (StateChange.CreateThread.HandleToThread)
			{
				NtClose(StateChange.CreateThread.HandleToThread);
			}
		default:
			OnUnWaited(StateChange);
		}

		DbgUiContinue(&StateChange.AppClientId, DBG_CONTINUE);
	}
}

BOOL YDbg::Init(YMessageLoop* loop)
{
	if (0 > DbgUiConnectToDbg())
	{
		return FALSE;
	}

	//ULONG flag = DEBUG_KILL_ON_CLOSE;
	//NtSetInformationDebugObject(DbgUiGetThreadDebugObject(), DebugObjectKillProcessOnExitInformation, &flag, sizeof(flag), 0);

	if (loop->addWaitObject(this, DbgUiGetThreadDebugObject()))
	{
		_M_loop = loop;
		return TRUE;
	}
	return FALSE;
}

YDbg::~YDbg()
{
	if (_M_loop)
	{
		_M_loop->delWaitObject(this);
	}

	PLIST_ENTRY entry = _M_Processes.Flink;

	while (entry != &_M_Processes)
	{
		YProcess* process = static_cast<YProcess*>(entry);
		entry = entry->Flink;
		process->Rundown();
		process->Release();
	}

	InitializeListHead(&_M_Processes);

	if (HANDLE hDebug = DbgUiGetThreadDebugObject())
	{
		DbgUiSetThreadDebugObject(0);
		NtClose(hDebug);
	}
}

void YDbg::OnStop()
{
	__debugbreak();
}