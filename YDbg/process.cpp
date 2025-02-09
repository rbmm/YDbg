#include "stdafx.h"

#include "obj.h"
#include "loop.h"
#include "dll.h"
#include "thread.h"
#include "process.h"
#include "tls.h"

#include "MiniWnd.h"
#include "LogView.h"

YProcess::YProcess(BOOL bDebugged)
{
	_IsDebugger = bDebugged != 0;
	DbgPrint("%hs<%p>(%x)\n", __FUNCTION__, this);
}

YProcess::~YProcess()
{
	if (_M_hProcess)
	{
		NtClose(_M_hProcess);
	}
		
	DbgPrint("%hs<%p>(%x)\n", __FUNCTION__, this, _M_dwProcessId);
}

void YProcess::OnSignal()
{
	_IsTerminated = TRUE;
	Rundown();
}

void YProcess::OnAbandoned()
{
	__debugbreak();
}

void YProcess::OnStop()
{
	OnSignal();
}

void _cprintf(HWND hwnd, PCWSTR buf)
{
	SendMessage(hwnd, EM_SETSEL, MAXLONG, MAXLONG);
	SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)buf);	
}

void YProcess::cprintf(PR pr, PCWSTR buf)
{
	if (_bittest(&_M_printMask, pr) && _M_hwndLog)
	{
		_cprintf(_M_hwndLog, buf);	
	}
}

void YProcess::vprintf(PR pr, PCWSTR format, va_list args)
{
	if (_bittest(&_M_printMask, pr) && _M_hwndLog)
	{
		WCHAR sz[0x400];
		_vsnwprintf(sz, _countof(sz), format, args);
		sz[_countof(sz) - 1] = 0;

		_cprintf(_M_hwndLog, sz);	
	}
}

void YProcess::printf(PR pr, PCWSTR format, ...)
{ 
	va_list args;
	va_start(args, format);
	vprintf(pr, format, args); 
	va_end(args);
}

void YProcess::OnDebugEvent(DBGUI_WAIT_STATE_CHANGE& StateChange)
{
	NTSTATUS status = DBG_CONTINUE;

	ULONG dwThreadId = PtrToUlong(StateChange.AppClientId.UniqueThread);

	switch(StateChange.NewState) 
	{
	case DbgBreakpointStateChange:
		status = OnBreakpoint(dwThreadId, 
			&StateChange.Exception.ExceptionRecord, 
			StateChange.Exception.FirstChance);			
		break;

	case DbgSingleStepStateChange:
		status = OnSingleStep(dwThreadId, 
			&StateChange.Exception.ExceptionRecord, 
			StateChange.Exception.FirstChance);			
		break;

	case DbgExceptionStateChange:
		status = OnException(dwThreadId, 
			&StateChange.Exception.ExceptionRecord, 
			StateChange.Exception.FirstChance);			
		break;

	case DbgCreateProcessStateChange:
		if (!OnCreateProcess(PtrToUlong(StateChange.AppClientId.UniqueProcess), 
			dwThreadId, &StateChange.CreateProcessInfo))
		{
			Rundown();
			return;
		}
		break;

	case DbgCreateThreadStateChange:
		OnCreateThread(dwThreadId, &StateChange.CreateThread);			
		break;

	case DbgExitProcessStateChange:
		OnExitProcess(StateChange.ExitProcess.ExitStatus);			
		break;

	case DbgExitThreadStateChange:
		OnExitThread(dwThreadId, StateChange.ExitThread.ExitStatus);			
		break;

	case DbgLoadDllStateChange:
		status = OnLoadDll(&StateChange.LoadDll, FALSE);
		break;

	case DbgUnloadDllStateChange:
		OnUnloadDll(StateChange.UnloadDll.BaseAddress);			
		break;

	default: __debugbreak();
	}

	if (status) 
	{
		DbgUiContinue(&StateChange.AppClientId, status);

		if (DbgExitProcessStateChange != StateChange.NewState)
		{
			_M_dwThreadId = 0;
			//_pAsm->setPC(0);
			//RemoveSrcPC();
			__nop();
		}
	}
	else 
	{
		_M_dwThreadId = dwThreadId;
//#ifdef _WIN64
//		switch (ctx.SegCs)
//		{
//		case 0x33:
//			_pAsm->SetTarget(DIS::amd64);
//			break;
//		case 0x23:
//			_pAsm->SetTarget(DIS::ia32);
//			break;
//		}
//#endif
//
//		WCHAR sz[16];
//		swprintf_s(sz, _countof(sz), L"TID=%x", dwThreadId);
//		_ZGLOBALS* globals = ZGLOBALS::get();
//		globals->MainFrame->SetStatusText(stThread, sz);
//		SetForegroundWindow(globals->hwndMain);
//
//		ctx.Dr7 &= 0xfff0fffc;
//		ctx.Dr0 = 0;
//
//		_pThread->_Va = 0;
//		_pReg->SetContext(&ctx);
//		_IsWaitContinue = TRUE;
//		_pAsm->setPC(ctx.Xip);
//		UpdateAllViews(0, ALL_UPDATED, 0);
//
//		_pDbgTH->ShowStackTrace(ctx);
	}
}

BOOL IsBufferSmall(NTSTATUS status)
{
	switch (status)
	{
	case STATUS_BUFFER_OVERFLOW:
	case STATUS_INFO_LENGTH_MISMATCH:
	case STATUS_BUFFER_TOO_SMALL:
		return TRUE;
	}

	return FALSE;
}

void YProcess::PrintName2(PVOID NamePointer)
{
	if (0 <= ZwReadVirtualMemory(_M_hProcess, NamePointer, &NamePointer, sizeof(NamePointer), 0))
	{
		PWSTR name = NtCurrentTeb()->StaticUnicodeBuffer;

		if (0 <= ZwReadVirtualMemory(_M_hProcess, NamePointer, name, sizeof(TEB::StaticUnicodeBuffer) - sizeof(WCHAR), 0))
		{
			name[_countof(TEB::StaticUnicodeBuffer) - 1] = 0;

			printf(prDLL, L"\t-- \"%ws\"\r\n", name);
		}
	}
}

NTSTATUS YProcess::OnLoadDll(PDBGKM_LOAD_DLL LoadDll, BOOL bExe)
{
	PVOID NamePointer = LoadDll->NamePointer;

	LoadDll->NamePointer = 0;

	union {
		PVOID buf;
		POBJECT_NAME_INFORMATION poni;
		PWSTR pwz;
	};
	ULONG cb;

	if (HANDLE hFile = LoadDll->FileHandle)
	{
		if (buf = Y_THREAD_STATE::_S_GetLocalBuf(cb))
		{
			NTSTATUS status = NtQueryObject(hFile, ObjectNameInformation, poni, cb - sizeof(WCHAR), &cb);

			if (0 <= status)
			{
				*(PWSTR)RtlOffsetToPointer(LoadDll->NamePointer = poni->Name.Buffer, poni->Name.Length) = 0;
			}
		}

		NtClose(hFile);
	}

	printf(prDLL, L"Load: base=%p \"%ws\"\r\n", LoadDll->BaseOfDll, LoadDll->NamePointer);

	if (YDll* pDll = new YDll(LoadDll->BaseOfDll, _M_nDllCount))
	{
		InsertHeadList(&_M_Dlls, pDll);
		_M_nDllCount++;
		pDll->SetImageName((PWSTR)LoadDll->NamePointer);
	}

	if (_bittest(&_M_printMask, prDLL2) && NamePointer)
	{
		PrintName2(NamePointer);
	}

	if (bExe && LoadDll->NamePointer)
	{
		if (buf = Y_THREAD_STATE::_S_GetLocalBuf(cb))
		{
			if (swprintf_s(pwz, cb / sizeof(WCHAR), L"[%x] \"%ws\"", _M_dwProcessId, (PWSTR)LoadDll->NamePointer))
			{
				SetWindowTextW(GetParent(_M_hwndLog), pwz);
			}
		}
	}

	return DBG_CONTINUE;
}

YThread* YProcess::getThreadById(ULONG dwThreadId)
{
	if (_M_pThread)
	{
		if (_M_pThread->_M_dwThreadId == dwThreadId)
		{
			return _M_pThread;
		}
	}

	PLIST_ENTRY head = &_M_Threads, entry = head;

	while ((entry = entry->Flink) != head)
	{
		YThread * pThread = static_cast<YThread *>(entry);

		if (pThread->_M_dwThreadId == dwThreadId)
		{
			_M_pThread = pThread;
			return pThread;
		}
	}

	return 0;
}

YDll* YProcess::getExeNoRefNoParse()
{
	PLIST_ENTRY head = &_M_Dlls, entry = head;

	while ((entry = entry->Blink) != head)
	{
		YDll* pDll = static_cast<YDll*>(entry);

		if (!pDll->_M_index)
		{
			return pDll;
		}
	}

	return 0;
}

YDll* YProcess::getDllByBaseNoRefNoParse(PVOID lpBaseOfDll)
{
	PLIST_ENTRY head = &_M_Dlls, entry = head;

	while ((entry = entry->Blink) != head)
	{
		YDll* pDll = static_cast<YDll*>(entry);

		if (pDll->_M_BaseAddress == lpBaseOfDll)
		{
			return pDll;
		}
	}

	return 0;
}

void YProcess::OnUnloadDll(PVOID lpBaseOfDll)
{
	if (YDll* pDll = getDllByBaseNoRefNoParse(lpBaseOfDll))
	{
		_M_nDllCount--;
		printf(prDLL, L"Unload: base=%p %s\r\n", lpBaseOfDll, pDll->_M_ImagePath);
		pDll->Unload();
		//OnLoadUnload(lpBaseOfDll, pDll->getID(), pDll->getSize(), FALSE);
		//_pAsm->OnUnloadDll(pDll);
		//UpdateAllViews(0, DLL_UNLOADED, lpBaseOfDll);
		pDll->Release();
		return ;
	}

	printf(prGen, L"Unload: base=%p ?!?\r\n", lpBaseOfDll);
}

BOOL YProcess::OnCreateProcess(ULONG dwProcessId, ULONG dwThreadId, PDBGUI_CREATE_PROCESS CreateProcessInfo)
{
	NtClose(CreateProcessInfo->HandleToProcess);

	if (CreateProcessInfo->HandleToProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessId))
	{
		_M_dwProcessId = dwProcessId;

		if (_M_hwndLog = YLogFrame::CreateLog(this))
		{
			_M_hProcess = CreateProcessInfo->HandleToProcess;

			DBGUI_CREATE_THREAD CreateThreadInfo = {
				CreateProcessInfo->HandleToThread, CreateProcessInfo->NewProcess.InitialThread
			};

			OnCreateThread(dwThreadId, &CreateThreadInfo);

			DBGKM_LOAD_DLL LoadDll = {
				CreateProcessInfo->NewProcess.FileHandle,
				CreateProcessInfo->NewProcess.BaseOfImage,
				CreateProcessInfo->NewProcess.DebugInfoFileOffset,
				CreateProcessInfo->NewProcess.DebugInfoSize
			};

			OnLoadDll(&LoadDll, TRUE);

			return TRUE;
		}
	}

	NtClose(CreateProcessInfo->NewProcess.FileHandle);
	NtClose(CreateProcessInfo->HandleToThread);
	NtClose(CreateProcessInfo->HandleToProcess);
	return FALSE;
}

void YProcess::OnExitProcess(ULONG dwExitCode)
{
	printf(prGen, L"process exit with code 0x%x(%d)\r\n", dwExitCode, dwExitCode);

	_IsTerminated = TRUE;

	DbgUiStopDebugging(_M_hProcess);
	Rundown();
}

void YProcess::Rundown()
{
	if (!_IsTerminated)
	{
		if (_M_dwThreadId)
		{
			CLIENT_ID cid = { (HANDLE)(ULONG_PTR)_M_dwProcessId, (HANDLE)(ULONG_PTR)_M_dwThreadId };

			DbgUiContinue(&cid, DBG_CONTINUE);

			_M_dwThreadId = 0;
		}

		//DoIoControl(IOCTL_SetProtectedProcess);
		NTSTATUS status = DbgUiStopDebugging(_M_hProcess);
		//DoIoControl(IOCTL_DelProtectedProcess);

		if (0 > status)
		{
			printf(prGen, L"DbgUiStopDebugging = %x\r\n", status);
			return ;
		}

		_IsTerminated = TRUE;

		printf(prGen, L"^^^^ Detached ^^^^\r\n");
	}

	if (_IsDetachCalled)
	{
		return;
	}

	_IsDetachCalled = TRUE;

	if (_M_hwndLog)
	{
		YLogFrame::Detach(_M_hwndLog);
		_M_hwndLog = 0;
	}

	if (_IsInserted)
	{
		if (YMessageLoop* loop = Y_THREAD_STATE::_S_GetMsgLoop())
		{
			loop->delWaitObject(this);
		}
		else
		{
			__debugbreak();
		}
		
		_IsInserted = FALSE;
	}

	PLIST_ENTRY head = &_M_Dlls, entry = head->Flink;

	while (entry != head)
	{
		YDll* p = static_cast<YDll*>(entry);
		entry = entry->Flink;
		p->Unload();
		p->Release();
	}

	InitializeListHead(head);

	head = &_M_Threads, entry = head->Flink;

	while (entry != head)
	{
		YThread* p = static_cast<YThread*>(entry);
		entry = entry->Flink;
		delete p;
	}

	InitializeListHead(head);

	if (_M_hProcess)
	{
		NtClose(_M_hProcess);
		_M_hProcess = 0;
	}

	RemoveEntryList(this);
	Release();
}

void YProcess::OnCreateThread(ULONG dwThreadId, PDBGUI_CREATE_THREAD CreateThreadInfo)
{
	HANDLE HandleToThread = CreateThreadInfo->HandleToThread;

	THREAD_BASIC_INFORMATION tbi;
	if (0 > NtQueryInformationThread(HandleToThread, ThreadBasicInformation, &tbi, sizeof(tbi), 0))
	{
		tbi.TebBaseAddress = 0;
	}

	if (!CreateThreadInfo->NewThread.StartAddress)
	{
		NtQueryInformationThread(HandleToThread, ThreadQuerySetWin32StartAddress, 
			&CreateThreadInfo->NewThread.StartAddress, sizeof(CreateThreadInfo->NewThread.StartAddress), 0);
	}

	printf(prThread, L"create thread %x at %p, teb=%p\r\n", dwThreadId, CreateThreadInfo->NewThread.StartAddress, tbi.TebBaseAddress);

	if (YThread* pThread = new YThread(HandleToThread, dwThreadId, tbi.TebBaseAddress, CreateThreadInfo->NewThread.StartAddress))
	{
		_M_pThread = pThread;
		_M_nThreadCount++;
		InsertHeadList(&_M_Threads, pThread);

		return ;
	}

	NtClose(HandleToThread);
}

void YProcess::OnExitThread(ULONG dwThreadId, ULONG dwExitCode)
{
	printf(prThread, L"thread %x exit with code 0x%x(%d)\r\n", dwThreadId, dwExitCode, dwExitCode);

	//_pDbgTH->DelThread(dwThreadId);

	if (YThread* pThread = getThreadById(dwThreadId))
	{
		_M_nThreadCount--;
		_M_pThread = 0;
		delete pThread;
	}
}

void YProcess::OnDbgPrint(SIZE_T cch, PVOID pv, BOOL bWideChar)
{
	if (!cch || !_bittest(&_M_printMask, prDbgPrint)) return;

	if (cch > 0x1000)
	{
		cch = 0x1000;
	}

	SIZE_T Length, cb = cch * sizeof(WCHAR);

	if (bWideChar)
	{
		Length = cb;
	}
	else
	{
		Length = cch;
	}

	union {
		PVOID buf;
		PSTR psz;
		PWSTR pwz;
	};

	ULONG cb2;
	if (!(buf = Y_THREAD_STATE::_S_GetLocalBuf(cb2)))
	{
		return ;
	}

	PWSTR str = pwz;

	if (!bWideChar)
	{
		psz += cch;
	}

	switch (ZwReadVirtualMemory(_M_hProcess, pv, buf, Length, &Length))
	{
	case STATUS_SUCCESS:
	case STATUS_PARTIAL_COPY:
		if (Length)
		{
			break;
		}
	default: return;
	}

	PWSTR wz = str;

	if (bWideChar)
	{
		cch = Length / sizeof(WCHAR);
	}
	else
	{
		if (!(cch = MultiByteToWideChar(CP_ACP, 0, psz, (ULONG)Length, str, (ULONG)cch)))
		{
			return ;
		}
	}

	Length = cch;

	ULONG len = (ULONG)Length;

	WCHAR c;
	BOOL wR = FALSE;
	do 
	{
		switch (c = *wz++)
		{
		case '\r':
			len++;
			wR = TRUE;
			continue;
		case 0:
			if (Length > 1) len += 2;
			break;
		case '\n':
			wR ? len-- : ++len;
			break;
		}

		wR = FALSE;

	} while (--Length);

	if (c)
	{
		len++;
	}

	if (len > cch)
	{
		Length = cch;
		PWSTR _wz = wz = str + len;

		wR = FALSE;
		do 
		{
			switch (c = *str++)
			{
			case '\r':
				*wz++ = '\r', *wz++ = '\n';
				wR = TRUE;
				continue;
			case '\n':
				if (!wR)
				{
					*wz++ = '\r', *wz++ = '\n';
				}
				break;
			case 0:
				if (Length > 1) 
				{
					*wz++ = '\r', *wz++ = '\n';
					break;
				}
			default:
				*wz++ = c;
			}

			wR = FALSE;
		} while (--Length);

		if (c)
		{
			*wz = 0;
		}

		str = _wz;
	}

	_cprintf(_M_hwndLog, str);
}

void YProcess::PrintException(ULONG dwThreadId, 
							  NTSTATUS ExceptionCode, 
							  PVOID ExceptionAddress, 
							  ULONG NumberParameters, 
							  PULONG_PTR ExceptionInformation, 
							  PCSTR Chance)
{
	if (NumberParameters > EXCEPTION_MAXIMUM_PARAMETERS)
	{
		NumberParameters = EXCEPTION_MAXIMUM_PARAMETERS;
	}
	WCHAR buf[256], *sz = buf;
	ULONG cch = _countof(buf) - 4;

	int len = swprintf_s(sz, cch, L"%x>OnException %x(%S) at %p [", dwThreadId, ExceptionCode, Chance, ExceptionAddress);

	if (0 < len)
	{
		sz = sz + len, cch -= len;
		if (NumberParameters)
		{
			do 
			{
				len = swprintf_s(sz, cch, L" %p,", (void*)*ExceptionInformation++);
				if (0 >= len)
				{
					break;
				}
				sz += len, cch -= len;
			} while (--NumberParameters);
			sz--;
		}
	}

	sz[0] = ']', sz[1] = '\r', sz[2] = '\n', sz[3] = 0;

	cprintf(prException, buf);
}

NTSTATUS OnFatal(HANDLE hProcess)
{
	ZwTerminateProcess(hProcess, STATUS_ABANDONED);
	return DBG_EXCEPTION_NOT_HANDLED;
}

NTSTATUS YProcess::OnException(DWORD dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance)
{
	NTSTATUS ExceptionCode = ExceptionRecord->ExceptionCode;

	BOOL bWideChar = FALSE;

	switch (ExceptionCode)
	{
	case DBG_PRINTEXCEPTION_WIDE_C:
		bWideChar = TRUE;
	case DBG_PRINTEXCEPTION_C:
		if (ExceptionRecord->NumberParameters >= 2)
		{
			OnDbgPrint(ExceptionRecord->ExceptionInformation[0],
				(PVOID)ExceptionRecord->ExceptionInformation[1], bWideChar);
		}
		return DBG_CONTINUE;
	}

	YThread* pThread = getThreadById(dwThreadId);

	if (!pThread)
	{
		return OnFatal(_M_hProcess);
	}

	CONTEXT ctx = {};
	ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_DEBUG_REGISTERS;

	HANDLE hThread = pThread->_M_hThread;

	if (0 > ZwGetContextThread(hThread, &ctx))
	{
		return OnFatal(_M_hProcess);
	}

	PVOID ExceptionAddress = ExceptionRecord->ExceptionAddress;

	//ZDbgThread::STATE state = pThread->_state;

	PrintException(dwThreadId, ExceptionCode, ExceptionAddress, 
		ExceptionRecord->NumberParameters, ExceptionRecord->ExceptionInformation, 
		dwFirstChance ? "First" : "Last");

	return dwFirstChance ? DBG_EXCEPTION_NOT_HANDLED : 0;
}

NTSTATUS YProcess::OnBreakpoint(ULONG dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance)
{
	switch (NTSTATUS ExceptionCode = ExceptionRecord->ExceptionCode)
	{
	case STATUS_WX86_BREAKPOINT:
		if (!_IsNotFirstWowBp)
		{
			_IsNotFirstWowBp = TRUE;
			return DBG_CONTINUE;
		}
		ExceptionCode = STATUS_BREAKPOINT;
	case STATUS_BREAKPOINT:
		if (!_IsNotFirstBp)
		{
			_IsNotFirstBp = TRUE;
			return DBG_CONTINUE;
		}
		//break;
	default: 
		return OnException(dwThreadId, ExceptionRecord, dwFirstChance);
	}
}

NTSTATUS YProcess::OnSingleStep(ULONG dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance)
{
	switch (NTSTATUS ExceptionCode = ExceptionRecord->ExceptionCode)
	{
	case STATUS_WX86_SINGLE_STEP:
		ExceptionCode = STATUS_SINGLE_STEP;
	case STATUS_SINGLE_STEP:
		//break;
	default: 
		return OnException(dwThreadId, ExceptionRecord, dwFirstChance);
	}
}
