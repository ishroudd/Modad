#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "monad.h"

namespace fs = std::filesystem;

HANDLE ddHandle = NULL;

// Functor that defines a virtual address based on a image base and a given offset.
struct virtual_addr {
    virtual_addr(ADDR ImageBase) : base(ImageBase) {}
    ADDR operator()(uintptr_t offset) const { return base + offset; }

private:
    ADDR base;
};

void WriteMods(MiddleMan ExeBase)
{
    virtual_addr rva(ExeBase.base);

    ADDR TargetAddress;
    HEX Bytes;

    std::string offset;
    std::string towrite;

    std::string DirectoryPath = ".\\mods";
    for (const auto& entry : fs::directory_iterator(DirectoryPath)) {
        std::cout << entry.path() << ": " << std::endl;

        std::ifstream ModFile;
        ModFile.open(entry.path(), std::ios_base::in);
        while (ModFile >> offset >> towrite) {
            std::cout << "**line**" << "**" << std::endl;
            std::cout << "sOffset: " << offset << std::endl;
            std::cout << "sBytes: " << towrite << std::endl;

            TargetAddress = rva(std::stoi(offset, nullptr, 16));
            Bytes = std::stoll(towrite, nullptr, 16);
            //printf("iOffset: %d\n", TargetAddress);
            //printf("iBytes: %lld\n", Bytes);
            HEX TEMPBytes;
            TEMPBytes = Bytes;

            // Writes to runtime memory, then reads the new value for confirmation
            ExeBase.read((LPCVOID)TargetAddress, (LPVOID)&TEMPBytes, towrite.length() / 2);
            //printf("PRE: %lld\n", TEMPBytes);

            ExeBase.write((LPVOID)TargetAddress, (LPCVOID)&Bytes, towrite.length() / 2);


            ExeBase.read((LPCVOID)TargetAddress, (LPVOID)&Bytes, towrite.length() / 2);

            //printf("FINAL: %lld\n", Bytes);
        }

        ModFile.close();
    }
}

int main(int argc, char* argv[])
{
    wchar_t ProcessName[7] = L"dd.exe";

    try
    {
        MiddleMan ddBase(ProcessName);

        WriteMods(ddBase);

        ddBase.unhook();
    }
    catch (const std::logic_error& e)
    {
        std::cerr << "No open process found." << std::endl;
        return 1;
    }

    return 0;
}