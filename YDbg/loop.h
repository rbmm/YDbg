#pragma once

class __declspec(novtable) YTranslateMsg : public LIST_ENTRY
{
public:
	virtual BOOL PreTranslateMessage(PMSG lpMsg) = 0;

	YTranslateMsg()
	{
		InitializeListHead(this);
	}

	~YTranslateMsg()
	{
		RemoveEntryList(this);
	}
};

class __declspec(novtable) YIdle : public LIST_ENTRY
{
public:
	virtual void OnIdle() = 0;

	YIdle()
	{
		InitializeListHead(this);
	}

	~YIdle()
	{
		RemoveEntryList(this);
	}
};

class __declspec(novtable) YSignalObject
{
public:
	virtual ~YSignalObject() = default;
	virtual void OnSignal() = 0;
	virtual void OnAbandoned() = 0;
	virtual void OnStop() = 0;
};

class YMessageLoop
{
	LIST_ENTRY _headTM = { &_headTM, &_headTM }, _headIdle = { &_headIdle, &_headIdle };

	HANDLE _Handles[MAXIMUM_WAIT_OBJECTS - 1] = {};
	YSignalObject* _objects[MAXIMUM_WAIT_OBJECTS - 1] = {};
	ULONG _nCount = 0;

protected:

	virtual ULONG GetWaitHandles(HANDLE ** ppHandles);
	virtual ULONG GetTimeout();
	virtual void OnSignalObject(ULONG i);
	virtual void OnAbandoned(ULONG i);
	virtual void OnApcAlert();
	virtual void OnTimeout();
	virtual void OnIdle();
	virtual BOOL IsIdleMessage(UINT uMsg);

public:

	virtual ~YMessageLoop();

	BOOL addWaitObject(YSignalObject* pObject, HANDLE hObject);
	BOOL delWaitObject(YSignalObject* pObject);

	void InsertTM(PLIST_ENTRY entry);
	void InsertIdle(PLIST_ENTRY entry);
	BOOL PreTranslateMessage(PMSG lpMsg);

	virtual BOOL CanClose(HWND hwnd);

	WPARAM Run();
};