#include "stdafx.h"

#include "obj.h"
#include "loop.h"
#include "dll.h"
#include "thread.h"
#include "process.h"

#define DbgPrint /##/

YDll::YDll(PVOID BaseAddress, ULONG index) : _M_BaseAddress(BaseAddress), _M_index(index)
{
	DbgPrint("%hs<%p>(%p)\n", __FUNCTION__, this, BaseAddress);
}

YDll::~YDll()
{
	if (_M_ImagePath)
	{
		free(_M_ImagePath);
	}

	RemoveEntryList(this);

	DbgPrint("%hs<%p>(%p)\n", __FUNCTION__, this, _M_BaseAddress);
}

void YDll::Unload()
{
}

void YDll::SetImageName(PCWSTR psz)
{
	_M_ImagePath = _wcsdup(psz);
}