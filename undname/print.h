#pragma once

#include "rtlframe.h"

struct _PrintInfo
{
	HANDLE _G_hFile;
	UINT _G_CodePage;
	BOOLEAN _G_bConsole;
};

using PrintInfo = RTL_FRAME<_PrintInfo>;

void PutChars(PCWSTR pwz, ULONG cch);

inline void PutChars(PCWSTR pwz)
{
	PutChars(pwz, (ULONG)wcslen(pwz));
}

void PrintWA_v(PCWSTR format, ...);

#define DbgPrint(fmt, ...) PrintWA_v(_CRT_WIDE(fmt), __VA_ARGS__ )

template <typename T>
T HR(HRESULT& hr, T t)
{
	hr = t ? NOERROR : GetLastError();
	return t;
}

HRESULT PrintError(HRESULT dwError);

void InitPrintf();

