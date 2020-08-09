#include <windows.h>
#include <cstdio>
#include <tlhelp32.h>
#include <psapi.h>
#include <cassert>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>

#pragma comment(lib, "Psapi.lib")
#define DPSAPI_VERSION (1)

/*************************
Proof-of-Concept. Now pulls a (address, value) pair from a file called modad.mod,
then writes the value to runtime memory.

Modad.mod is a text file with two DWORD-sized hexidecimal numbers separated by a space. 
The first DWORD is the RVA from the image base. The second is an AOB (in big-endian) used to patch the given address.
Very limiting right now without editing the source, still very much a POC.
************************/

typedef uintptr_t HEX;
typedef DWORD ADDR;

HANDLE ddHandle = NULL;

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

ADDR GetModule(wchar_t* TargetProcess, HANDLE ProcessHandle)
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

// functor that defines a virtual address based on the image base and a given offset
struct virtual_addr {
    virtual_addr(ADDR ImageBase) : base(ImageBase) {}
    ADDR operator()(uintptr_t offset) const { return base + offset; }

private:
    ADDR base;
};

int main(int argc, char* argv[])
{
    HANDLE TokenHandle = NULL;
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

    // ImageBase for dd
    ADDR ddBase = GetModule(ProcessName, ddHandle);

    // Taking values from mod file
    std::ifstream ModFile;
    ModFile.open("modad.mod", std::ios_base::in);

    std::vector<std::string> data;
    std::string temp;

    while(ModFile >> temp) {
        data.push_back(temp);
    }
    ModFile.close();

    ADDR ModTarget;
    std::stringstream Addrss;
    Addrss << data[0];
    Addrss >> std::hex >> ModTarget;
    printf("num: %d\n", ModTarget);

    virtual_addr rva(ddBase);

    ADDR TargetAddress = rva(ModTarget);
    printf("TargetAddr: %d\n", TargetAddress);

    // Convert AOB to little-endian
    assert(data[1].length() % 2 == 0);
    std::reverse(data[1].begin(), data[1].end());
    for (auto it = data[1].begin(); it != data[1].end(); it += 2) {
        std::swap(it[0], it[1]);
    }

    // Convert hex string to int typedef
    std::cout << std::hex << data[1] << std::endl;
    HEX HexBytes;
    std::stringstream Hexss;
    Hexss << std::hex << data[1];
    Hexss >> HexBytes;
    HEX HexBytes2 = HexBytes;

    // Writes to runtime memory, then reads the new value for confirmation
    int n;
    WriteProcessMemory(ddHandle, (LPVOID) TargetAddress, (LPCVOID) &HexBytes, 4, NULL);
    ReadProcessMemory(ddHandle, (LPCVOID) TargetAddress, (LPVOID) &HexBytes, 4, NULL);
    std::cout << "Final byte values: " << static_cast<int>(HexBytes) << std::endl;

    CloseHandle(ddHandle);

    getchar();
    return 0;
}