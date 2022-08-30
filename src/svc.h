#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <cstring>
#include <thread>
#include <chrono>
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("tinky")

SERVICE_STATUS gSvcStatus;
SERVICE_STATUS_HANDLE gSvcStatusHandle;
HANDLE ghSvcStopEvent = NULL;

/*
	Docs: https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
*/
class Service {
private:
	static std::tuple<SC_HANDLE, SC_HANDLE> _getHandlers();
public:
	Service() {};

	static void	Install();
	static void	Start();
	static void	Stop();
	static void	Delete();
};