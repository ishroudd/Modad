#include <windows.h>
#include <cstdio>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>

#pragma comment(lib, "Psapi.lib")
#define DPSAPI_VERSION (1)

/*************************
Proof-of-Concept. Currently only proves the ability to successfully write to devil daggers memory space during runtime.
Excuse any sloppy code.
************************/

BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{

    TOKEN_PRIVILEGES tp;
    LUID luid; 

    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) { // Check privilege on local system
        printf("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = NULL;

    // Enable the privilege or disable all privileges.
    if (!AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES)NULL,
        (PDWORD)NULL))
    {
        printf("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}

HANDLE GetProcessByName(wchar_t* TargetProcess)
{
    DWORD pid = 0;

    // Create toolhelp snapshot.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    // Walk through all processes.
    if (Process32First(snapshot, &process)){
        do {
            if (!wcscmp(process.szExeFile, TargetProcess)){              
                pid = process.th32ProcessID;
                //wprintf(L"%ls \n%ls \n%d \n", TargetProcess, process.szExeFile, pid);
                break;
            }
        } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    if (pid != 0)
    {
        return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    // Not found

    return NULL;
}

DWORD GetModule(wchar_t* TargetProcess, HANDLE ProcessHandle)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;
    wchar_t szModPath[MAX_PATH];

    if (EnumProcessModules(ProcessHandle, hMods, sizeof(hMods), &cbNeeded)) {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            if (GetModuleFileNameEx(ProcessHandle, hMods[i], szModPath, sizeof(szModPath) / sizeof(TCHAR))) {
                if (wcsstr(szModPath, TargetProcess)) {
                    //wprintf(L"%ls, %ls\n", szModPath, TargetProcess);

                    return (DWORD)hMods[i];
                }
            }
        }
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    HANDLE TokenHandle = NULL;
    HANDLE ddHandle = NULL;
    wchar_t ProcessName[7] = L"dd.exe";
    
    if (!(ddHandle = GetProcessByName(ProcessName))) {
        return 1;
    }

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle)) {
        std::cout << "OpenProcessToken() failed, error %u\n" << GetLastError() << std::endl;
        return FALSE;
    }

    if (!SetPrivilege(TokenHandle, SE_DEBUG_NAME, TRUE)) {
        std::cout << "Failed to enable privilege, error %u\n" << GetLastError() << std::endl;
        return FALSE;
    }

    DWORD ddBase = GetModule(ProcessName, ddHandle);
    printf("%d\n", ddBase);

    // Offset from base address to address used for proof-of-concept.
    DWORD POCAddress = ddBase + 618755;
    printf("%d\n", POCAddress);
    
    // little-endian '83 E4 FF' in decimal form.
    int POCOffset = 16770179;

    WriteProcessMemory(ddHandle, (LPVOID) POCAddress, &POCOffset, 3, NULL);
    ReadProcessMemory(ddHandle, (LPCVOID)POCAddress, &POCOffset, 3, NULL);
    printf("%d", POCOffset);

    CloseHandle(ddHandle);
    return 0;
}