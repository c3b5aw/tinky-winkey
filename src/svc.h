#pragma once

#include "sdk.h"

#include <strsafe.h>
#include <cstring>
#include <thread>
#include <chrono>
#include <tlhelp32.h>

#define SVCNAME TEXT("tinky")

/*
	Docs: https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
*/
namespace Service
{
	SERVICE_STATUS gSvcStatus;
	SERVICE_STATUS_HANDLE gSvcStatusHandle;
	HANDLE ghSvcStopEvent = nullptr;

	std::tuple<SC_HANDLE, SC_HANDLE> getHandlers();
	void Install();
	void	Start();
	void	Stop();
	void	Delete();
	void WINAPI Main();
	void WINAPI Init();
	void WINAPI ControlHandler(DWORD dwCtrl);
	void ReportStatus(DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwWaitHint);
	HANDLE GetToken();
	void StartProcess(STARTUPINFO *si,
		PROCESS_INFORMATION *pi
	);
};