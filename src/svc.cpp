#include "svc.h"

void Service::Install()
{
	std::cout << "Installing service\n";

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		std::cout << std::format("OpenSCManager failed ({})\n", GetLastError());
		return;
	}

	SC_HANDLE schService = CreateService(
		schSCManager,
		SVCNAME,
		SVCNAME,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		"C:\\svc.exe",
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

std::tuple<SC_HANDLE, SC_HANDLE> Service::getHandlers()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		const DWORD lastError = GetLastError();
		if (lastError == ERROR_ACCESS_DENIED) {
			std::cout << "Make sure you launched with admins rights\n";
		}
		else {
			std::cout << std::format("OpenSCManager failed ({})\n", lastError);
		}
		return std::tuple(nullptr, nullptr);
	}

	SC_HANDLE schService = OpenService(
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

void Service::Start()
{
	auto [schSCManager, schService] = getHandlers();
	if (schSCManager == nullptr || schService == nullptr) {
		return;
	}

	std::cout << "Starting service\n";
	if (StartService(schService, 0, nullptr)) {
		std::cout << "Service started succesfully\n";
	}
	else {
		const DWORD lastError = GetLastError();
		if (lastError == ERROR_SERVICE_ALREADY_RUNNING) {
			std::cout << "Service already started\n";
		}
		else {
			std::cout << std::format("Service failed to start ({})\n", lastError);
		}
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

void Service::Stop()
{
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

void Service::Delete()
{
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

void WINAPI Service::Main()
{
	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		reinterpret_cast<LPHANDLER_FUNCTION>(ControlHandler)
	);

	if (!gSvcStatusHandle) {
		std::cout << "RegisterServiceCtrlHandler failed\n";
		return;
	}

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	Service::Init();
}

void WINAPI Service::Init()
{
	ghSvcStopEvent = CreateEvent(
		nullptr,
		TRUE,
		FALSE,
		nullptr
	);
	if (ghSvcStopEvent == nullptr) {
		ReportStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

	/* Setup service subprocess */
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	Service::StartProcess(&si, &pi);

	/* Wait until service stop request */
	while (1) {
		WaitForSingleObject(ghSvcStopEvent, INFINITE);
		ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
		break;
	}

	/* Force terminate process */
	if (pi.hProcess) {
		TerminateProcess(pi.hProcess, 0);
	}

	/* Cleanup handles */
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(si.hStdInput);
	CloseHandle(si.hStdError);
	CloseHandle(si.hStdOutput);
}

void WINAPI Service::ControlHandler(DWORD dwCtrl)
{
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
		SetEvent(ghSvcStopEvent);
		ReportStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

void Service::ReportStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING) {
		gSvcStatus.dwControlsAccepted = 0;
	}
	else {
		gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
		gSvcStatus.dwCheckPoint = 0;
	}
	else {
		gSvcStatus.dwCheckPoint = ++dwCheckPoint;
	}

	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

/*
	Source: https://stackoverflow.com/q/13480344
*/
HANDLE Service::GetToken()
{
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		std::cout << std::format("CreateToolhelp32Snapshot failed ({})\n", GetLastError());
		return nullptr;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	
	if (!Process32First(hProcessSnap, &pe32)) {
		std::cout << std::format("Process32First failed ({})\n", GetLastError());
		CloseHandle(hProcessSnap);
		return nullptr;
	}

	DWORD logonPID = 0;
	do {
		if (lstrcmpi(pe32.szExeFile, TEXT("winlogon.exe")) == 0) {
			logonPID = pe32.th32ProcessID;
			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);

	if (logonPID == 0) {
		std::cout << std::format("Process32Next failed ({})\n", GetLastError());
		return nullptr;
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, logonPID);
	if (hProcess == nullptr) {
		std::cout << std::format("OpenProcess failed ({})\n", GetLastError());
		return nullptr;
	}

	HANDLE hToken = nullptr;
	OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);
	CloseHandle(hProcess);
	if (hToken == nullptr) {
		std::cout << std::format("OpenProcessToken failed ({})\n", GetLastError());
		return nullptr;
	}

	HANDLE hTokenD = nullptr;
	DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenImpersonation, &hTokenD);
	CloseHandle(hToken);
	if (hTokenD == nullptr) {
		std::cout << std::format("DuplicateTokenEx failed ({})\n", GetLastError());
		return nullptr;
	}

	return hTokenD;
}

void Service::StartProcess(STARTUPINFO* si, PROCESS_INFORMATION* pi)
{
	HANDLE hToken = GetToken();
	if (hToken == nullptr) {
		std::cout << std::format("GetToken failed ({})\n", GetLastError());
		return;
	}

	CreateProcessAsUser(hToken,
		TEXT("C:\\winkey.exe"),
		nullptr,
		nullptr,
		nullptr,
		false,
		NORMAL_PRIORITY_CLASS | DETACHED_PROCESS,
		NULL,
		NULL,
		si,
		pi
	);

	CloseHandle(hToken);
}

/*
	__cdecl__: Stack is cleanup by the caller
	_tmain	 : Windows conversion to enable Unicode support
*/
int __cdecl _tmain(int argc, TCHAR* argv[])
{
	if (argc == 2) {
		if (lstrcmpi(argv[1], TEXT("install")) == 0) {
			Service::Install();
			return 0;
		}
		else if (lstrcmpi(argv[1], TEXT("start")) == 0) {
			Service::Start();
			return 0;
		}
		else if (lstrcmpi(argv[1], TEXT("stop")) == 0) {
			Service::Stop();
			return 0;
		}
		else if (lstrcmpi(argv[1], TEXT("delete")) == 0) {
			Service::Delete();
			return 0;
		}
	}

	SERVICE_TABLE_ENTRY DispatchTable[] = {{
			const_cast<LPSTR>(SVCNAME),
			reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(Service::Main)},
		{NULL, NULL}
	};

	StartServiceCtrlDispatcher(DispatchTable);
	return 0;
}