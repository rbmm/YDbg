#pragma once

class YProcess;

class YLogFrame : public YWnd
{
	YProcess* _M_process;
	HWND _M_hwndEdit;

	virtual LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual LRESULT DefWinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void AfterLastMessage()
	{
		Detach_I();
	}

	~YLogFrame();
	YLogFrame(YProcess* process);

	BOOL OnCreate(HWND hwnd);
	void OnSize(WPARAM wParam, ULONG cx, ULONG cy);
	void Detach_I();

	enum { WM_DETACH = WM_APP };
public:

	static void Detach(HWND hwndEdit);

	static HWND CreateLog(YProcess* process);
};