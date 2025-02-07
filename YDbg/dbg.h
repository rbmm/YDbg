#pragma once

class YDbg : public YSignalObject
{
	LIST_ENTRY _M_Processes = { &_M_Processes, &_M_Processes };
	YMessageLoop* _M_loop = 0;

	virtual void OnSignal();

	virtual void OnAbandoned()
	{
		__debugbreak();
	}

	virtual void OnStop();

protected:
public:

	BOOL Init(YMessageLoop* loop);

	virtual ~YDbg();

	BOOL IsEmpty()
	{
		return IsListEmpty(&_M_Processes);
	}
};