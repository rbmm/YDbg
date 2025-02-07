#include "stdafx.h"

#include "obj.h"
#include "loop.h"
#include "dll.h"
#include "thread.h"
#include "process.h"

YThread::~YThread()
{
	if (_M_hThread)
	{
		NtClose(_M_hThread);
	}

	RemoveEntryList(this);

	//DbgPrint("%hs<%p>(%x)\n", __FUNCTION__, this, _M_dwThreadId);
}