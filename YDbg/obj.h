#pragma once

class __declspec(novtable) YObject 
{
	LONG _M_dwRefCount = 1;
protected:
	LONG _M_spare = 0;

	virtual ~YObject() = default;

public:

	virtual void AddRef()
	{
		InterlockedIncrementNoFence(&_M_dwRefCount);
	}

	virtual void Release()
	{
		if (!InterlockedDecrement(&_M_dwRefCount))
		{
			delete this;
		}
	}
};