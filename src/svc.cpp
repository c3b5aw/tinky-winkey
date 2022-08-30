#include "sdk.h" 
#include "svc.h"

void Service::Install() {
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH + 2] = {0};

	if (!GetModuleFileName(NULL, szPath + 1, MAX_PATH)) {
		std::cout << std::format("Cannot install service({})\n", GetLastError());
		return;
	}

	szPath[0] = '"';
	szPath[std::strlen(szPath)] = '"';

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		std::cout << std::format("OpenSCManager failed ({})\n", GetLastError());
		return;
	}
	schService = CreateService(
		schSCManager,
		SVCNAME,
		SVCNAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		szPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);

	if (schService == nullptr) {
		std::cout << std::format("CreateServiec failed ({})\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return;
	}
	std::cout << "Service installed successfully\n";
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::Start() {}

void Service::Stop() {}

void Service::Delete() {}

/*
	__cdecl__: Stack is cleanup by the caller
	_tmain	 : Windows conversion to enable Unicode support
*/
int __cdecl _tmain(int argc, TCHAR* argv[])
{
	if (argc == 2) {
		if (std::strcmp(argv[1], "install")) {
			Service::Install();
		}
		else if (std::strcmp(argv[1], "start")) {
			Service::Start();
		}
		else if (std::strcmp(argv[1], "stop")) {
			Service::Stop();
		}
		else if (std::strcmp(argv[1], " delete")) {
			Service::Delete();
		}
	}
	std::cout << __cplusplus << std::endl;
}