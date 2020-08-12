#pragma once

// ADDR intended for addresses, HEX intended for hexidecimal bytes
typedef uintptr_t ADDR;
typedef long long HEX;

class MiddleMan
{
private:
    // Handle used to check token privileges
    HANDLE TokenHandle = NULL;
    // Handle for the open process.
    HANDLE ddHandle;

    // Attempts to set the required token privileges.
    BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
    
    // Returns an open handle to the process matching the given string.
    HANDLE GetProcessByName(wchar_t* TargetProcess);

    // Returns process image base, given a handle.
    ADDR GetBase(wchar_t* TargetProcess, HANDLE ProcessHandle);
public:
    // Base address of the process image base.
    ADDR base;

    /* Simple wrapper for ReadProcessMemory.
     address - Pointer to base address to which data is written.
     buffer - Pointer to a buffer that receives the contents of the specified address.
     size - Number of bytes to read from the specified address.
     Returns 0 on fail*/
    BOOL read(LPCVOID address, LPVOID buffer, SIZE_T size);

    /* Simple wrapper for WriteProcessMemory.
     address - Pointer to base address to which data is written.
     buffer - Pointer to a buffer that receives the contents of the specified address.
     size - Number of bytes to read from the specified address.
     Returns 0 on fail.*/
    BOOL write(LPVOID address, LPCVOID buffer, SIZE_T size); 

    // Every MiddleMan object needs to be manually unhooked when done to close the opened process handle.
    void unhook();

    // Constructs a MiddleMan object, taking the process name as a string.
    // Sets the base member to the address of the image base.
    MiddleMan(wchar_t* ProcessName);
};