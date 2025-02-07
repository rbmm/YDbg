#pragma once

class __declspec(novtable) YWnd
{
	LONG _dwRefCount = 1;
	LONG _dwCallCount = 0;

	virtual void AfterLastMessage()
	{
	}

	LRESULT WrapperWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT WINAPI _WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static LRESULT WINAPI StartWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:

	virtual ~YWnd() = default;

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT DefWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void OnNcDestroy()
	{
		_bittestandreset(&_dwCallCount, 31);
	}

public:

	void AddRef()
	{
		InterlockedIncrementNoFence(&_dwRefCount);
	}

	void Release()
	{
		if (!InterlockedDecrement(&_dwRefCount))
		{
			delete this;
		}
	}

	HWND Create(
		DWORD dwExStyle,
		PCWSTR lpWindowName,
		DWORD dwStyle,
		int x,
		int y,
		int nWidth,
		int nHeight,
		HWND hWndParent,
		HMENU hMenu,
		PVOID lpParam = 0
		);

	HWND MdiChildCreate(
		DWORD dwExStyle,
		PCWSTR lpWindowName,
		DWORD dwStyle,
		int x,
		int y,
		int nWidth,
		int nHeight,
		HWND hWndParent,
		HMENU hMenu,
		PVOID lpParam = 0
		);

	static ATOM Register();

	static void Unregister();
};

class YDlg
{
	LONG _M_dwRef = 1;
	LONG _dwCallCount = 0;

	virtual void AfterLastMessage()
	{
	}

	virtual BOOL OnInitDialog(HWND /*hwnd*/, PVOID /*lParam*/)
	{
		return TRUE;
	}

	static INT_PTR CALLBACK _S_DlgProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam);

	static INT_PTR CALLBACK DlgProcStart(HWND hwnd, UINT umsg, WPARAM /*wParam*/, LPARAM lParam);

protected:

	void OnDestroy()
	{
		_bittestandreset(&_dwCallCount, 31);
	}

	virtual INT_PTR DlgProc(HWND /*hwnd*/, UINT umsg, WPARAM /*wParam*/, LPARAM /*lParam*/);

public:

	virtual ~YDlg()
	{
	}

	void AddRef()
	{
		InterlockedIncrementNoFence(&_M_dwRef);
	}

	void Release()
	{
		if (!InterlockedDecrement(&_M_dwRef))
		{
			delete this;
		}
	}

	INT_PTR DoModal(HINSTANCE hInstance, PCWSTR lpTemplateName, HWND hWndParent, LPARAM dwInitParam = 0);
	
	HWND Show(HINSTANCE hInstance, PCWSTR lpTemplateName, HWND hWndParent, LPARAM dwInitParam = 0);
};

void EmptyPaint(HWND hwnd);