#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>

using namespace std;

#define SERVICE_NAME TEXT("A Lab2 service")
#define COMMAND_INSTALL "install"
#define COMMAND_REMOVE "remove"
#define COMMAND_START "start"
#define COMMAND_STOP "stop"

#pragma warning(disable: 4996)

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE stopServiceEvent = 0;

int InstallService();
int RemoveService();
int RunService();
int StopService();

VOID WINAPI ServiceMain(DWORD argc, TCHAR* argv[]);
VOID WINAPI ServiceControlHandler(DWORD controlCode);
VOID WINAPI Thread();
static float CalculateCPULoad();
static unsigned long long FileTimeToInt64();
float GetCPULoad();

int main(int argc, char* argv[]) {
    if (argc - 1 == 0) {
        SERVICE_TABLE_ENTRY ServiceTable[] = {
            {(LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
            {NULL, NULL}
        };

        if (!StartServiceCtrlDispatcher(ServiceTable)) {
            cout << "Error: StartServiceCtrlDispatcher" << endl;
            cout << GetLastError() << endl;
        }
    }
    else if (argc > 1 && strcmp(argv[1], COMMAND_INSTALL) == 0) {
        cout << "Installing service..." << endl;
        InstallService();
    }
    else if (argc > 1 && strcmp(argv[1], COMMAND_REMOVE) == 0) {
        cout << "Removing service..." << endl;
        RemoveService();
    }
    else if (argc > 1 && strcmp(argv[1], COMMAND_START) == 0) {
        cout << "Starting service..." << endl;
        RunService();
    }
    else if (argc > 1 && strcmp(argv[1], COMMAND_STOP) == 0) {
        cout << "Stopping service..." << endl;
        StopService();
    }
}

int InstallService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) {
        cout << "Error: Can't open Service Control Manager" << endl;
        return -1;
    }

    TCHAR path[_MAX_PATH + 1];
    if (GetModuleFileName(0, path, sizeof(path) / sizeof(*path)) > 0) {
        cout << path << endl;
        SC_HANDLE hService = CreateServiceW(
            hSCManager,
            SERVICE_NAME,
            SERVICE_NAME,
            SERVICE_ALL_ACCESS,
            SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
            SERVICE_DEMAND_START,
            SERVICE_ERROR_NORMAL,
            path,
            NULL, NULL, NULL, NULL, NULL
        );

        if (!hService) {
            int err = GetLastError();
            switch (err) {
            case ERROR_ACCESS_DENIED:
                cout << "Error: ERROR_ACCESS_DENIED" << endl;
                break;
            case ERROR_CIRCULAR_DEPENDENCY:
                cout << "Error: ERROR_CIRCULAR_DEPENDENCY" << endl;
                break;
            case ERROR_DUPLICATE_SERVICE_NAME:
                cout << "Error: ERROR_DUPLICATE_SERVICE_NAME" << endl;
                break;
            case ERROR_INVALID_HANDLE:
                cout << "Error: ERROR_INVALID_HANDLE" << endl;
                break;
            case ERROR_INVALID_NAME:
                cout << "Error: ERROR_INVALID_NAME" << endl;
                break;
            case ERROR_INVALID_PARAMETER:
                cout << "Error: ERROR_INVALID_PARAMETER" << endl;
                break;
            case ERROR_INVALID_SERVICE_ACCOUNT:
                cout << "Error: ERROR_INVALID_SERVICE_ACCOUNT" << endl;
                break;
            case ERROR_SERVICE_EXISTS:
                cout << "Error: ERROR_SERVICE_EXISTS" << endl;
                break;
            default:
                cout << "Error: Undefined" << endl;
            }
            CloseServiceHandle(hSCManager);
            return -1;
        }
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);

        cout << "Success install service!" << endl;
    }
    return 0;
}

int RemoveService() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        cout << "Error: Can't connect to Service Control Manager" << endl;
        return -1;
    }
    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_QUERY_STATUS | DELETE);
    if (!hService) {
        cout << "Error: Can't remove service" << endl;
        CloseServiceHandle(hSCManager);
        return -1;
    }
    SERVICE_STATUS svStatus;
    if (QueryServiceStatus(hService, &svStatus))
    {
        if (svStatus.dwCurrentState == SERVICE_STOPPED)
        {
            if (DeleteService(hService)) {
                cout << "Success remove service!" << endl;
            } else {
                cout << GetLastError() << endl;
                return -1;
            }
        } else {
            cout << "Service is running." << endl;
            return -1;
        }
    } else {
        cout << GetLastError() << endl;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}

int RunService() {
    SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) {
        cout << "Error: Can't connect to Service Control Manage" << endl;
        return -1;
    }
    SC_HANDLE hService = OpenServiceW(hSCManager, SERVICE_NAME, SERVICE_START);
    if (!hService) {
        cout << "Error: Can't open the service" << endl;
        return -1;
    }
    if (!StartServiceW(hService, 0, NULL)) {
        CloseServiceHandle(hSCManager);
        cout << "Error: Can't start service" << endl;
        cout << GetLastError() << endl;
        return -1;
    } else {
        cout << "Service started!" << endl;
    }
   
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}

int StopService()
{
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) {
        cout << "Error: Can't connect to Service Control Manager" << endl;
        return -1;
    }
    SC_HANDLE hService = OpenService(hSCManager, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!hService) {
        cout << "Error: Can't remove service" << endl;
        CloseServiceHandle(hSCManager);
        return -1;
    }
    DWORD bytesNeeded;
    SERVICE_STATUS_PROCESS svStatus;
    if (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&svStatus, sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded))
    {
        if (svStatus.dwCurrentState == SERVICE_RUNNING)
        {
            if (ControlService(hService, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&svStatus)) {
                cout << "Service stopped!\n";
            } else {
                cout << "Error: ";
                cout << GetLastError() << endl;
            }
        }
    }
    else {
        cout << GetLastError() << endl;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return 0;
}

VOID WINAPI ServiceMain(DWORD argc, TCHAR* argv[]) {
    // initialise service status
    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    serviceStatus.dwWin32ExitCode = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceControlHandler);

    if (!serviceStatusHandle)
    {
        return;
    }
    serviceStatus.dwCurrentState = SERVICE_START_PENDING;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    stopServiceEvent = CreateEventW(0, FALSE, FALSE, 0);

    serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Thread, NULL, 0, NULL);
    WaitForSingleObject(stopServiceEvent, INFINITE);

    serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    CloseHandle(stopServiceEvent);
    stopServiceEvent = 0;

    serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

VOID WINAPI ServiceControlHandler(DWORD controlCode) {
    switch (controlCode)
    {
    case SERVICE_CONTROL_INTERROGATE:
        break;

    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(serviceStatusHandle, &serviceStatus);

        SetEvent(stopServiceEvent);
        return;

    case SERVICE_CONTROL_PAUSE:
        break;

    case SERVICE_CONTROL_CONTINUE:
        break;

    default:
        if (controlCode >= 128 && controlCode <= 255)
        {
            break;
        }
        else
        {
            break;
        }
    }

    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

VOID WINAPI Thread()
{
    //init();
    ofstream fout;
    while (true) {
        fout.open("C:/Users/User/Desktop/monitor.txt", ios_base::out);
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys / 1024 / 1024;
        DWORDLONG physMemUsed = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / 1024 / 1024;
        DWORDLONG availableMem = memInfo.ullAvailPhys / 1024 /1024;
        fout << "====RAM Monitor==================\n";
        fout << "Time: ";
        fout << currentDateTime();
        fout << "\nTotal RAM: ";
        fout << totalPhysMem;
        fout << "MB\n";
        fout << "RAM used: ";
        fout << physMemUsed;
        fout << "MB\n";
        fout << "Available RAM: ";
        fout << availableMem;
        fout << "MB\n";
        fout << "CPU load: ";
        fout << GetCPULoad() * 100 << "\n";
        fout << "\n===============================\n";
        fout.close();
        Sleep(1000);
    }
}

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
    static unsigned long long _previousTotalTicks = 0;
    static unsigned long long _previousIdleTicks = 0;

    unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
    unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;


    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

    _previousTotalTicks = totalTicks;
    _previousIdleTicks = idleTicks;
    return ret;
}

static unsigned long long FileTimeToInt64(const FILETIME& ft)
{
    return (((unsigned long long)(ft.dwHighDateTime)) << 32) | ((unsigned long long)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad()
{
    FILETIME idleTime, kernelTime, userTime;
    return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime)) : -1.0f;
}