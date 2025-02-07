#pragma once

enum PR {
	prDLL, prDLL2, prThread, prException, prDbgPrint, prGen
};

class YProcess : public YObject, public YSignalObject, public LIST_ENTRY
{
	LIST_ENTRY _M_Threads = { &_M_Threads, &_M_Threads }, _M_Dlls = { &_M_Dlls, &_M_Dlls };
	YThread* _M_pThread = 0;
	HWND _M_hwndLog = 0;
	HANDLE _M_hProcess = 0;
	ULONG _M_dwProcessId = 0;
	ULONG _M_dwThreadId = 0;

	ULONG _M_nDllCount = 0, _M_nThreadCount = 0;

	union {
		ULONG _M_Flags = 0;
		struct 
		{
			ULONG _IsInserted : 1;
			ULONG _IsWow64Process : 1;
			ULONG _IsDebugger : 1;
			ULONG _IsTerminated : 1;
			ULONG _IsWaitContinue : 1;
			ULONG _IsInTrace : 1;
			ULONG _IsDump : 1;
			ULONG _IsDetachCalled : 1;
			ULONG _IsDbgInherit : 1;
			ULONG _IsRemoteDebugger : 1;
			ULONG _IsLocalMemory : 1;
			ULONG _IsUdtTry : 1;
			ULONG _IsAttached : 1;
			ULONG _IsActive : 1;
			ULONG _IsRemoteWait : 1;
			ULONG _IsNotFirstBp : 1;
			ULONG _IsNotFirstWowBp : 1;
			ULONG _SpareBits : 15;
		};
	};

	LONG _M_printMask = -1;

	virtual ~YProcess();

	NTSTATUS OnException(ULONG dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance);
	NTSTATUS OnSingleStep(ULONG dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance);
	NTSTATUS OnBreakpoint(ULONG dwThreadId, PEXCEPTION_RECORD ExceptionRecord, BOOL dwFirstChance);
	void OnDbgPrint(SIZE_T cch, PVOID pv, BOOL bWideChar);

	BOOL OnCreateProcess(ULONG dwProcessId, ULONG dwThreadId, PDBGUI_CREATE_PROCESS CreateProcessInfo);
	void OnCreateThread(ULONG dwThreadId, PDBGUI_CREATE_THREAD CreateThreadInfo);
	void OnExitProcess(ULONG dwExitCode);
	void OnExitThread(ULONG dwThreadId, ULONG dwExitCode);
	NTSTATUS OnLoadDll(PDBGKM_LOAD_DLL LoadDll, BOOL bExe);
	void OnUnloadDll(PVOID lpBaseOfDll);

	virtual void OnSignal();
	virtual void OnAbandoned();
	virtual void OnStop();

	YDll* getDllByBaseNoRefNoParse(PVOID lpBaseOfDll);
	YDll* getExeNoRefNoParse();

	YThread* getThreadById(ULONG dwThreadId);

	void PrintException(
		ULONG dwThreadId, 
		NTSTATUS ExceptionCode, 
		PVOID ExceptionAddress, 
		ULONG NumberParameters, 
		PULONG_PTR ExceptionInformation, 
		PCSTR Chance);

	void PrintName2(PVOID NamePointer);

	//////////////////////////////////////////////////////////////////////////
	// log

	void cprintf(PR pr, PCWSTR buf);

	void vprintf(PR pr, PCWSTR format, va_list args);

	void printf(PR pr, PCWSTR format, ...);

public:

	YProcess(BOOL bDebugged);

	ULONG getId()
	{
		return _M_dwProcessId;
	}

	void OnDebugEvent(DBGUI_WAIT_STATE_CHANGE& StateChange);
	void Rundown();

	NTSTATUS Terminate(NTSTATUS ExitStatus = 0)
	{
		return NtTerminateProcess(_M_hProcess, ExitStatus);
	}
};