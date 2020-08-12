#include <windows.h>
#include <cstdio>
#include <tlhelp32.h>
#include <psapi.h>
#include <stdexcept>
#include "monad.h"

#pragma comment(lib, "Psapi.lib")
#define DPSAPI_VERSION (1)

/*************************
Proof-of-Concept. Now pulls a (address, value) pair from a file called modad.mod,
then writes the value to runtime memory.

Modad.mod is a text file with two DWORD-sized hexidecimal numbers separated by a space.
The first DWORD is the RVA from the image base. The second is an AOB (in big-endian) used to patch the given address.
Very limiting right now without editing the source, still very much a POC.
************************/

MiddleMan::MiddleMan(wchar_t* ProcessName) 
{
    if (!(ddHandle = GetProcessByName(ProcessName))) {
        throw std::logic_error("No open process found.");
    }

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle)) {
        printf("OpenProcessToken() failed, error %u\n", GetLastError());
    }

    if (!SetPrivilege(TokenHandle, SE_DEBUG_NAME, TRUE)) {
        printf("Failed to enable privilege, error %u\n", GetLastError());
    }

    CloseHandle(TokenHandle);

    // ImageBase for dd
    base = GetBase(ProcessName, ddHandle);
    printf("base: %p\n", base);
}

void MiddleMan::unhook()
{
    CloseHandle(ddHandle);
}

BOOL MiddleMan::SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
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

HANDLE MiddleMan::GetProcessByName(wchar_t* TargetProcess)
{
    DWORD pid = 0;

    // Create toolhelp snapshot.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    // Walk through all processes.
    if (Process32First(snapshot, &process)) {
        do {
            if (!wcscmp(process.szExeFile, TargetProcess)) {
                pid = process.th32ProcessID;
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

ADDR MiddleMan::GetBase(wchar_t* TargetProcess, HANDLE ProcessHandle)
{
    HMODULE hMods[1024];
    DWORD cbNeeded;
    wchar_t szModPath[MAX_PATH];

    if (EnumProcessModules(ProcessHandle, hMods, sizeof(hMods), &cbNeeded)) {
        for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            if (GetModuleFileNameEx(ProcessHandle, hMods[i], szModPath, sizeof(szModPath) / sizeof(TCHAR))) {
                if (wcsstr(szModPath, TargetProcess)) {

                    return (ADDR)hMods[i];
                }
            }
        }
    }

    return NULL;
}

BOOL MiddleMan::read(LPCVOID address, LPVOID buffer, SIZE_T size)
{
    return ReadProcessMemory(ddHandle, address, buffer, 4, NULL);
}

BOOL MiddleMan::write(LPVOID address, LPCVOID buffer, SIZE_T size)
{
    if (!WriteProcessMemory(ddHandle, address, buffer, 4, NULL)) {
        printf("ERROR: %d\n", GetLastError());
        return 1;
    }
    else {
        return 0;
    }
}