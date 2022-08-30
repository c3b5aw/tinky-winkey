#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <cstring>
#pragma comment(lib, "advapi32.lib")

#define SVCNAME TEXT("tinky")

/*
	Docs: https://docs.microsoft.com/en-us/windows/win32/services/svc-cpp
*/
class Service {
public:
	Service() {};

	static void	Install();
	static void	Start();
	static void	Stop();
	static void	Delete();
};