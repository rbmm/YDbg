#pragma once

#include "obj.h"

class YDll : public YObject, public LIST_ENTRY
{
	friend class YProcess;

	PVOID _M_BaseAddress;
	PWSTR _M_ImagePath = 0;
	ULONG _M_index;

	virtual ~YDll();
public:

	PVOID GetImageBase()
	{
		return _M_BaseAddress;
	}

	void Unload();

	YDll(PVOID BaseAddress, ULONG index);

	void SetImageName(PCWSTR psz);
};
