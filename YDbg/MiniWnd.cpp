#include "stdafx.h"

#include "MiniWnd.h"

LRESULT YWnd::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCDESTROY:
		OnNcDestroy();
		break;
	}

	return DefWinProc(hwnd, uMsg, wParam, lParam);
}

struct Y_CREATE_PARAMS 
{
	union {
		YDlg* ThisDlg;
		YWnd* ThisWnd;
	};
	union {
		LPARAM lParam;
		PVOID pvParam;
	};

	BOOL bMdi;
};

YWnd* FixCreateParams(_Inout_ CREATESTRUCT* lpcs, _Out_ LPARAM** pplParam, _Out_ LPARAM* plParam)
{
	MDICREATESTRUCT* mcs = reinterpret_cast<MDICREATESTRUCT*>(lpcs->lpCreateParams);
	Y_CREATE_PARAMS* params = reinterpret_cast<Y_CREATE_PARAMS*>(mcs->lParam);

	LPARAM lParam = params->lParam;

	if (params->bMdi)
	{
		*plParam = (LPARAM)params;
		*pplParam = &mcs->lParam;
		mcs->lParam = lParam;
	}
	else
	{
		*plParam = (LPARAM)mcs;
		*pplParam = (LPARAM*)&lpcs->lpCreateParams;
		lpcs->lpCreateParams = (PVOID)lParam;
	}

	return params->ThisWnd;
}

LRESULT YWnd::WrapperWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	_dwCallCount++;

	if (WM_CREATE == uMsg) [[unlikely]]
	{
		LPARAM *ppl, pl;
		if (this != FixCreateParams(reinterpret_cast<CREATESTRUCT*>(lParam), &ppl, &pl))
		{
			__debugbreak();
		}
	}

	lParam = WindowProc(hwnd, uMsg, wParam, lParam);

	if (!--_dwCallCount)
	{
		AfterLastMessage();
		Release();
	}

	return lParam;
}

LRESULT WINAPI YWnd::_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return reinterpret_cast<YWnd*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))->WrapperWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT WINAPI YWnd::StartWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_NCCREATE)
	{
		LPARAM *ppl, pl;

		YWnd* This = FixCreateParams(reinterpret_cast<CREATESTRUCT*>(lParam), &ppl, &pl);
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)This);
		SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)_WindowProc);
		This->AddRef();
		This->_dwCallCount = 1 << 31;
		lParam = This->WrapperWindowProc(hwnd, uMsg, wParam, lParam);
		*ppl = pl;
		return lParam;
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

static const WCHAR cls_name[] = L"93C8AF95BC114ed5BE9F09CC5626CCF3";

HWND YWnd::Create(DWORD dwExStyle,
				  PCWSTR lpWindowName,
				  DWORD dwStyle,
				  int x,
				  int y,
				  int nWidth,
				  int nHeight,
				  HWND hWndParent,
				  HMENU hMenu,
				  PVOID lpParam)
{
	Y_CREATE_PARAMS params;
	params.ThisWnd = this;
	params.pvParam = lpParam;
	params.bMdi = FALSE;
	MDICREATESTRUCT mcs = {};
	mcs.lParam = (LPARAM)&params;

	return CreateWindowExW(dwExStyle, cls_name, lpWindowName, dwStyle, x, y, 
		nWidth, nHeight, hWndParent, hMenu, (HINSTANCE)&__ImageBase, &mcs);
}

HWND YWnd::MdiChildCreate(DWORD dwExStyle,
						  PCWSTR lpWindowName,
						  DWORD dwStyle,
						  int x,
						  int y,
						  int nWidth,
						  int nHeight,
						  HWND hWndParent,
						  HMENU hMenu,
						  PVOID lpParam)
{
	Y_CREATE_PARAMS params;
	params.ThisWnd = this;
	params.pvParam = lpParam;
	params.bMdi = TRUE;

	return CreateWindowExW(dwExStyle, cls_name, lpWindowName, dwStyle, x, y, 
		nWidth, nHeight, hWndParent, hMenu, (HINSTANCE)&__ImageBase, &params);
}

ATOM YWnd::Register()
{
	WNDCLASSW cls = {
		0, StartWindowProc, 0, 0, (HINSTANCE)&__ImageBase, 0, 0, 0, 0, cls_name
	};

	return RegisterClassW(&cls);
}

void YWnd::Unregister()
{
	if (!UnregisterClassW(cls_name, (HINSTANCE)&__ImageBase)) __debugbreak();
}

//////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK YDlg::_S_DlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam)
{
	YDlg* dlg = reinterpret_cast<YDlg*>(GetWindowLongPtrW(hwnd, DWLP_USER));

	dlg->_dwCallCount++;

	lParam = dlg->DlgProc(hwnd, umsg, wParam, lParam);

	if (!--dlg->_dwCallCount)
	{
		dlg->AfterLastMessage();
		dlg->Release();
	}

	return lParam;
}

INT_PTR CALLBACK YDlg::DlgProcStart(HWND hwnd, UINT umsg, WPARAM /*wParam*/, LPARAM lParam)
{
	if (WM_INITDIALOG == umsg)
	{
		Y_CREATE_PARAMS* params = reinterpret_cast<Y_CREATE_PARAMS*>(lParam);
		
		YDlg* This = params->ThisDlg;

		SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)This);
		SetWindowLongPtrW(hwnd, DWLP_DLGPROC, (LPARAM)_S_DlgProc);
		This->AddRef();
		This->_dwCallCount = 1 << 31;
		return This->OnInitDialog(hwnd, params->pvParam);
	}

	return 0;
}

INT_PTR YDlg::DoModal(HINSTANCE hInstance, PCWSTR lpTemplateName, HWND hWndParent, LPARAM dwInitParam)
{
	Y_CREATE_PARAMS params = { this, dwInitParam };
	return DialogBoxParam(hInstance, lpTemplateName, hWndParent, DlgProcStart, (LPARAM)&params);
}

HWND YDlg::Show(HINSTANCE hInstance, PCWSTR lpTemplateName, HWND hWndParent, LPARAM dwInitParam)
{
	Y_CREATE_PARAMS params = { this, dwInitParam };
	return CreateDialogParamW(hInstance, lpTemplateName, hWndParent, DlgProcStart, (LPARAM)&params);
}

INT_PTR CALLBACK YDlg::DlgProc(HWND /*hwnd*/, UINT umsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	switch (umsg)
	{
	case WM_NCDESTROY:
		OnDestroy();
		break;
	}

	return 0;
}