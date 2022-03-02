#include <iostream>
#include <windows.h>
#include <string>
#include <Wbemidl.h>
#include <comdef.h>
#include <tlhelp32.h>

#pragma comment(lib, "wbemuuid.lib")
#define _WIN32_DCOM

using namespace std;

void printLogicalDrivesInfo();
void printDriveType(string& drive);
void printDiskFreeSpace(string& disk);
void printVolumeInformation(string& disk);
void printAudioCardInfo();
VOID PrintModuleList(HANDLE CONST hStdOut, DWORD CONST dwProcessId);
VOID PrintProcessList(HANDLE CONST hStdOut);

int main() {
	cout << "\n====================Information about available drives====================\n\n";
	printLogicalDrivesInfo();
    cout << "\n====================Information about sound device====================\n\n";
    printAudioCardInfo();
    cout << "\n====================Information about processes====================\n\n";
    HANDLE CONST hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    PrintProcessList(hStdOut);
}

void printLogicalDrivesInfo() {
	int n;
	string drive;
	DWORD dr = GetLogicalDrives();

	for (int i = 0; i < 26; i++)
	{
		n = ((dr >> i) & 0x00000001);
		if (n == 1)
		{
			drive.push_back(char(65 + i));
			drive.push_back(':');
			drive.push_back('\\');
			drive.push_back(0);
			cout << "Available disk drive : " << drive << endl;
			printDriveType(drive);
			printDiskFreeSpace(drive);
			printVolumeInformation(drive);
			cout << "\n";
		}
		drive.clear();
	}
}

void printDriveType(string& drive)
{
	int driveType;
	driveType = GetDriveTypeA(drive.c_str());
	if (driveType == DRIVE_UNKNOWN) cout << "\tUNKNOWN" << endl;
	if (driveType == DRIVE_NO_ROOT_DIR) cout << "\tDRIVE NO ROOT DIR" << endl;
	if (driveType == DRIVE_REMOVABLE) cout << "\tREMOVABLE" << endl;
	if (driveType == DRIVE_FIXED) cout << "\tFIXED DISK" << endl;
	if (driveType == DRIVE_REMOTE) cout << "\tREMOTE" << endl;
	if (driveType == DRIVE_CDROM) cout << "\tCDROM" << endl;
	if (driveType == DRIVE_RAMDISK) cout << "\tRAMDISK" << endl;
}

void printDiskFreeSpace(string& disk)
{
	long long FreeBytesAvailable = NULL;
	long long TotalNumberOfBytes = NULL;
	long long TotalNumberOfFreeBytes = NULL;

	BOOL GetDiskFreeSpaceFlag = GetDiskFreeSpaceExA(
		disk.c_str(),					  // directory name
		(PULARGE_INTEGER)&FreeBytesAvailable,     // bytes available to caller
		(PULARGE_INTEGER)&TotalNumberOfBytes,     // bytes on disk
		(PULARGE_INTEGER)&TotalNumberOfFreeBytes  // free bytes on disk
	);

	if (GetDiskFreeSpaceFlag != 0)
	{
		cout << "\tTotal Space = " << double(TotalNumberOfBytes) / 1024 / 1024 / 1024 << " GB " << endl;
		cout << "\tFree Space = " << double(TotalNumberOfFreeBytes) / 1024 / 1024 / 1024 << " GB " << endl;
		cout << "\tOccupied Space = " << double(TotalNumberOfBytes - TotalNumberOfFreeBytes) / 1024 / 1024 / 1024 << " GB " << endl;
	}
	else	cout << "	Not Present (GetDiskFreeSpace)" << endl;
}

void printVolumeInformation(string& disk)
{
	char FileSystemNameBuffer[100];
	unsigned long VolumeSerialNumber;

	BOOL GetVolumeInformationFlag = GetVolumeInformationA(
		disk.c_str(),
		NULL,
		100,
		&VolumeSerialNumber,
		NULL, //&MaximumComponentLength,
		NULL, //&FileSystemFlags,
		FileSystemNameBuffer,
		100
	);

	if (GetVolumeInformationFlag != 0)
	{
		cout << "\tVolume Serial Number is " << VolumeSerialNumber << endl;
		cout << "\tFile System is " << FileSystemNameBuffer << endl;
	}
	else cout << "	Not Present (GetVolumeInformation)" << endl;
}

void printAudioCardInfo()
{
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        cout << "Failed to initialize COM library. Error code = 0x" << hex << hres << endl;
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );


    if (FAILED(hres))
    {
        cout << "Failed to initialize security. Error code = 0x" << hex << hres << endl;
        CoUninitialize();
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        cout << "Failed to create IWbemLocator object. Err code = 0x" << hex << hres << endl;
        CoUninitialize();
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;
    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (for example, Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        cout << "Could not connect. Error code = 0x" << hex << hres << endl;
        pLoc->Release();
        CoUninitialize();
    }

    cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        cout << "Could not set proxy blanket. Error code = 0x" << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_SoundDevice"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        cout << "Query for sound device failed. Error code = 0x" << hex << hres << endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp;

        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        wcout << " Sound Device name : " << vtProp.bstrVal << endl;
        VariantClear(&vtProp);
 
        hr = pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        wcout << " Sound Device manufacturer: " << vtProp.bstrVal << endl;
        VariantClear(&vtProp);

        hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
        wcout << " Sound Device ID: " << vtProp.bstrVal << endl;
        VariantClear(&vtProp);

        hr = pclsObj->Get(L"Status", 0, &vtProp, 0, 0);
        wcout << " Sound Device status: " << vtProp.bstrVal << endl;
        VariantClear(&vtProp);

        pclsObj->Release();
    }

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
}


VOID PrintModuleList(HANDLE CONST hStdOut, DWORD CONST dwProcessId) {
    MODULEENTRY32 meModuleEntry;
    TCHAR szBuff[1024];
    DWORD dwTemp;
    HANDLE CONST hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE, dwProcessId);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return;
    }

    meModuleEntry.dwSize = sizeof(MODULEENTRY32);
    Module32First(hSnapshot, &meModuleEntry);
    do {
        wsprintf(szBuff, L"  ba: %08X, bs: %08X, %s\r\n",
            meModuleEntry.modBaseAddr, meModuleEntry.modBaseSize,
            meModuleEntry.szModule);
        WriteConsole(hStdOut, szBuff, lstrlen(szBuff), &dwTemp, NULL);
    } while (Module32Next(hSnapshot, &meModuleEntry));

    CloseHandle(hSnapshot);
}

VOID PrintProcessList(HANDLE CONST hStdOut) {
    PROCESSENTRY32 peProcessEntry;
    TCHAR szBuff[1024];
    DWORD dwTemp;
    HANDLE CONST hSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return;
    }

    peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
    Process32First(hSnapshot, &peProcessEntry);
    int processCount = 0;
    do {
        wsprintf(szBuff, L"=== %08X %s ===\r\n",
            peProcessEntry.th32ProcessID, peProcessEntry.szExeFile);
        WriteConsole(hStdOut, szBuff, lstrlen(szBuff), &dwTemp, NULL);
        //PrintModuleList(hStdOut, peProcessEntry.th32ProcessID);
        processCount++;
    } while (Process32Next(hSnapshot, &peProcessEntry));
    CloseHandle(hSnapshot);
    cout << "\nProcess count: " << processCount << endl;
}