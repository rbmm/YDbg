#pragma once

class YThread : public YObject, public LIST_ENTRY
{
	friend class YProcess;

	HANDLE _M_hThread;
	PVOID _M_TebBaseAddress;
	PVOID _M_StartAddress;
	ULONG _M_dwThreadId;

	virtual ~YThread();

	YThread(
		HANDLE hThread, 
		ULONG dwThreadId,
		PVOID TebBaseAddress,
		PVOID StartAddress) : 
	_M_hThread(hThread), _M_dwThreadId(dwThreadId), _M_StartAddress(StartAddress), _M_TebBaseAddress(TebBaseAddress)
	{
		//DbgPrint("%hs<%p>(%x)\n", __FUNCTION__, this, _M_dwThreadId);
	}

public:
	ULONG getId()
	{
		return _M_dwThreadId;
	}
};
