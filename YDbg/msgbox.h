#pragma once

HMODULE GetNtMod();
HRESULT GetLastErrorEx(ULONG dwError = GetLastError());

int CustomMessageBox(HWND hWnd, PCWSTR lpText, PCWSTR lpszCaption, UINT uType);
int ShowErrorBox(HWND hwnd, HRESULT dwError, PCWSTR lpCaption, UINT uType);
