#include "stdafx.h"

#include "rtlframe.h"
#include "print.h"

#ifndef UNDNAME_COMPLETE
#define UNDNAME_COMPLETE                 0x0000  // Enable full undecoration
#define UNDNAME_NO_LEADING_UNDERSCORES   0x0001  // Remove leading underscores from MS extended keywords
#define UNDNAME_NO_MS_KEYWORDS           0x0002  // Disable expansion of MS extended keywords
#define UNDNAME_NO_FUNCTION_RETURNS      0x0004  // Disable expansion of return type for primary declaration
#define UNDNAME_NO_ALLOCATION_MODEL      0x0008  // Disable expansion of the declaration model
#define UNDNAME_NO_ALLOCATION_LANGUAGE   0x0010  // Disable expansion of the declaration language specifier
#define UNDNAME_NO_MS_THISTYPE           0x0020  // NYI Disable expansion of MS keywords on the 'this' type for primary declaration
#define UNDNAME_NO_CV_THISTYPE           0x0040  // NYI Disable expansion of CV modifiers on the 'this' type for primary declaration
#define UNDNAME_NO_THISTYPE              0x0060  // Disable all modifiers on the 'this' type
#define UNDNAME_NO_ACCESS_SPECIFIERS     0x0080  // Disable expansion of access specifiers for members
#define UNDNAME_NO_THROW_SIGNATURES      0x0100  // Disable expansion of 'throw-signatures' for functions and pointers to functions
#define UNDNAME_NO_MEMBER_TYPE           0x0200  // Disable expansion of 'static' or 'virtual'ness of members
#define UNDNAME_NO_RETURN_UDT_MODEL      0x0400  // Disable expansion of MS model for UDT returns
#define UNDNAME_32_BIT_DECODE            0x0800  // Undecorate 32-bit decorated names
#define UNDNAME_NAME_ONLY                0x1000  // return just [scope::]name.  Does expand template params;  
#define UNDNAME_NO_ARGUMENTS             0x2000  // Don't undecorate arguments to function
#define UNDNAME_NO_SPECIAL_SYMS          0x4000  // Don't undecorate special names (v-table, vcall, vector xxx, metatype, etc)
#endif
#define UNDNAME_NO_ECSU					 0x8000   // Suppresses enum/class/struct/union. 

#define UNDNAME_DEFAULT (UNDNAME_NO_MS_KEYWORDS|UNDNAME_NO_ACCESS_SPECIFIERS|UNDNAME_NO_THROW_SIGNATURES|UNDNAME_NO_ECSU|UNDNAME_NO_ALLOCATION_MODEL|UNDNAME_NO_THISTYPE|UNDNAME_NO_RETURN_UDT_MODEL)

typedef RTL_FRAME<DATA_BLOB> AFRAME;

static void* __cdecl fAlloc(ULONG cb)
{
	if (DATA_BLOB* prf = AFRAME::get())
	{
		if (cb > prf->cbData)
		{
			return 0;
		}
		prf->cbData -= cb;
		PVOID pv = prf->pbData;
		prf->pbData += cb;
		return pv;
	}

	return 0;
}

static void __cdecl fFree(void*)
{
}

EXTERN_C
_CRTIMP
PSTR __cdecl __unDNameEx
(
	PSTR buffer,
	PCSTR mangled,
	DWORD cb,
	void* (__cdecl* memget)(DWORD),
	void(__cdecl* memfree)(void*),
	PSTR(__cdecl* GetParameter)(long i),
	DWORD flags
);

#ifdef _X86_
	#define __imp__unDNameEx _imp____unDNameEx
#else
	#define __imp__unDNameEx __imp___unDNameEx
#endif // _X86_

EXTERN_C PVOID __imp__unDNameEx = 0;

PSTR __cdecl GetParameter(long /*i*/)
{
	return const_cast<PSTR>("");
}

static PSTR _unDName(PCSTR mangled, PSTR buffer, DWORD cb, DWORD flags)
{
	if (__imp__unDNameEx)
	{
	__ok:
		AFRAME af;
		af.cbData = 32 * PAGE_SIZE;
		af.pbData = (PUCHAR)alloca(32 * PAGE_SIZE);

		return __unDNameEx(buffer, mangled, cb, fAlloc, fFree, GetParameter, flags);
	}

	if (HMODULE hmod = LoadLibraryW(L"msvcrt.dll"))
	{
		if (__imp__unDNameEx = GetProcAddress(hmod, "__unDNameEx"))
		{
			goto __ok;
		}
	}

	return 0;
}

#define CASE_XY(x, y) case x: c = y; break

PSTR UndecorateString(_In_ PSTR pszSym, _Out_ PSTR *pNameSpace)
{
	BOOL bUnicode;
	PSTR pc = pszSym, name = pszSym;

	switch (*pc++)
	{
	case '0':
		bUnicode = FALSE;
		break;

	case '1':
		bUnicode = TRUE;
		break;

	default:
		//__debugbreak();
		return 0;
	}

	if (*pc - '0' >= 10 && !(pc = strchr(pc, '@')))
	{
		//__debugbreak();
		return 0;
	}

	if (pc = strchr(pc + 1, '@'))
	{
		if (bUnicode)
		{
			*pszSym++ = 'L';
		}
		*pszSym++ = '\"';
	}
	else
	{
		//__debugbreak();
		return 0;
	}

	int i = 0;
	char c;

	while ('@' != (c = *++pc))
	{
		// special char ?
		union {
			USHORT u = 0;
			char pp[2];
		};

		if ('?' == c)
		{
			switch (*++pc)
			{
			case '$':
				pp[1] = *++pc, pp[0] = *++pc;

				switch (u)
				{
					CASE_XY('AA', 0);
					CASE_XY('AH', '\a');
					CASE_XY('AI', '\b');
					CASE_XY('AM', '\f');
					CASE_XY('AL', '\v');
					CASE_XY('AN', '\r');
					CASE_XY('CC', '\"');
					CASE_XY('HL', '{');
					CASE_XY('HN', '}');
					CASE_XY('FL', '[');
					CASE_XY('FN', ']');
					CASE_XY('CI', '(');
					CASE_XY('CJ', ')');
					CASE_XY('DM', '<');
					CASE_XY('DO', '>');
					CASE_XY('GA', '`');
					CASE_XY('CB', '!');
					CASE_XY('EA', '@');
					CASE_XY('CD', '#');
					CASE_XY('CF', '%');
					CASE_XY('FO', '^');
					CASE_XY('CG', '&');
					CASE_XY('CK', '*');
					CASE_XY('CL', '+');
					CASE_XY('HO', '~');
					CASE_XY('DN', '=');
					CASE_XY('HM', '|');
					CASE_XY('DL', ';');
					CASE_XY('DP', '?');
				default:
					return 0;
				}
				break;
				CASE_XY('0', ',');
				CASE_XY('1', '/');
				CASE_XY('2', '\\');
				CASE_XY('3', ':');
				CASE_XY('4', '.');
				CASE_XY('5', ' ');
				CASE_XY('6', '\n');
				CASE_XY('7', '\t');
				CASE_XY('8', '\'');
				CASE_XY('9', '-');
			case '@':
				//__debugbreak();
			default:
				return 0;
			}
		}

		if (bUnicode)
		{
			if (++i & 1)
			{
				if (c)
				{
					//__debugbreak();
					return 0;
				}
				continue;
			}
		}

		*pszSym++ = c;
	}

	*pszSym++ = '\"', *pszSym = 0;

	*pNameSpace = 0;

	if (*++pc)
	{
		if (PSTR pa = strchr(pc, '@'))
		{
			*pa++ = 0;

			if (*pa)
			{
				//__debugbreak();
				return 0;
			}

			*pNameSpace = pc;
		}
		else
		{
			//__debugbreak();
			return 0;
		}
	}

	return name;
}

PSTR unDNameEx(PSTR rawName, PSTR undName, DWORD cb, DWORD flags, PSTR* pNameSpace)
{
	if (*rawName != '?')
	{
		return rawName;
	}
	// ??_C@_ `string`
	if (rawName[1] == '?' &&
		rawName[2] == '_' &&
		rawName[3] == 'C' &&
		rawName[4] == '@' &&
		rawName[5] == '_')
	{
		if (!strcpy_s(undName, cb, rawName + 6))
		{
			if (undName = UndecorateString(undName, pNameSpace))
			{
				return undName;
			}
		}

		return rawName;
	}

	PSTR sz = _unDName(rawName, undName, cb, flags);
	return sz ? sz : rawName;
}

void undname(PWSTR name)
{
	ULONG cch = (ULONG)wcslen(name) + 1;
	PSTR psz = 0;
	ULONG len = 0;
	while (len = WideCharToMultiByte(CP_UTF8, 0, name, cch, psz, len, 0, 0))
	{
		if (psz)
		{
			char buf[0x800];
			PSTR NameSpace = 0;
			if (psz = unDNameEx(psz, buf, sizeof(buf), UNDNAME_DEFAULT, &NameSpace))
			{
				if (NameSpace)
				{
					DbgPrint("\r\n********\r\n%ws\r\n%hs // %hs\r\n\r\n", name, psz, NameSpace);
				}
				else
				{
					DbgPrint("\r\n********\r\n%ws\r\n%hs\r\n\r\n", name, psz);
				}
			}
			break;
		}

		psz = (PSTR)alloca(len);
	}
}

void WINAPI ep(PWSTR psz)
{
	{
		PrintInfo pi;
		InitPrintf();

		psz = GetCommandLineW();
		PWSTR name = 0;
		while (psz = wcschr(psz, '*'))
		{
			*psz++ = 0;
			if (name)
			{
				undname(name);
			}

			name = psz;
		}
	}

	ExitProcess(0);
}