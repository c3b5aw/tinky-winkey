#include "sdk.h" 
#include "svc.h"

void Service::Install() {
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	TCHAR szPath[MAX_PATH + 2] = {0};

	std::cout << "Install\n";

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
		const DWORD lastError = GetLastError();
		if (lastError == 1073) {
			std::cout << "Service is already installed";
		}
		else {
			std::cout << std::format("CreateService failed ({})\n", GetLastError());
		}
		CloseServiceHandle(schSCManager);
		return;
	}
	std::cout << "Service installed successfully\n";
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::Start() {
	auto [schSCManager, schService] = getHandlers();
	if (schSCManager == nullptr || schService == nullptr) {
		return;
	}

	std::cout << "Starting service\n";
	if (StartService(schService, 0, NULL)) {
		std::cout << "Service started succesfully\n";
	}
	else {
		const DWORD lastError = GetLastError();
		if (lastError == 1056) {
			std::cout << "Service already started\n";
		}
		else {
			std::cout << std::format("Service failed to start ({})\n", lastError);
		}
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

std::tuple<SC_HANDLE, SC_HANDLE> Service::getHandlers() {
	SC_HANDLE schService;
	SC_HANDLE schSCManager;

	std::cout << "getHandlers\n";

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		std::cout << std::format("OpenSCManager failed ({})\n", GetLastError());
		return std::tuple(nullptr, nullptr);
	}

	schService = OpenService(
		schSCManager,
		SVCNAME,
		SERVICE_ALL_ACCESS
	);
	if (schService == nullptr) {
		std::cout << std::format("OpenService failed ({})\n", GetLastError());
		CloseServiceHandle(schSCManager);
		return std::tuple(nullptr, nullptr);
	}
	return std::make_tuple(schSCManager, schService);
}

void Service::Stop() {
	auto [schSCManager, schService] = getHandlers();
	if (schSCManager == nullptr || schService == nullptr) {
		return;
	}

	std::cout << "Stopping service\n";
	if (ControlService(schService, SERVICE_CONTROL_STOP, &gSvcStatus)) {
		std::cout << "Service stopped successfully\n";
	}
	else {
		const DWORD lastError = GetLastError();
		if (lastError == 1062) {
			std::cout << "Service already stopped\n";
		}
		else {
			std::cout << std::format("ControlService STOP failed ({})\n", lastError);
		}
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::Delete() {
	auto [schSCManager, schService] = getHandlers();
	if (schSCManager == nullptr || schService == nullptr) {
		return;
	}

	if (ControlService(schService, SERVICE_CONTROL_STOP, &gSvcStatus)) {
		std::cout << "Stopping service\n";
		while (QueryServiceStatus(schService, &gSvcStatus)) {
			if (gSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
				std::cout << ".";
				std::this_thread::sleep_for(std::chrono::milliseconds(250));
			}
			else {
				break;
			}
		}
	}

	if (DeleteService(schService)) {
		std::cout << "Service has been removed\n";
	}
	else {
		std::cout << std::format("DeleteService failed ({})\n", GetLastError());
	}
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

/*
	__cdecl__: Stack is cleanup by the caller
	_tmain	 : Windows conversion to enable Unicode support
*/
int __cdecl _tmain(int argc, TCHAR* argv[])
{
	if (argc == 2) {
		if (std::strcmp(argv[1], "install") == 0) {
			Service::Install();
		}
		else if (std::strcmp(argv[1], "start") == 0) {
			Service::Start();
		}
		else if (std::strcmp(argv[1], "stop") == 0) {
			Service::Stop();
		}
		else if (std::strcmp(argv[1], "delete") == 0) {
			Service::Delete();
		}
	}
}