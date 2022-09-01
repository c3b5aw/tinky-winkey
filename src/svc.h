#pragma once

#include "sdk.h"

#include <strsafe.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <tlhelp32.h>

#define SVCNAME TEXT("tinky")

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = nullptr;

/*
	Docs: https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
*/
class Service {
private:
	static std::tuple<SC_HANDLE, SC_HANDLE> getHandlers();
public:
	Service() {};

	static void Install();
	static void	Start();
	static void	Stop();
	static void	Delete();
	static void WINAPI Main();
	static void WINAPI Init();
	static void WINAPI ControlHandler(DWORD);
	static void ReportStatus(DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwWaitHint);
	static HANDLE GetToken();
	static void StartProcess(STARTUPINFO *si, PROCESS_INFORMATION *pi);
};