#pragma once
#include <Windows.h>
#include <winternl.h>
#include <vector>
#include <string>

typedef NTSTATUS (NTAPI* LdrLoadDll_t)(
    PWCHAR PathToFile,
    PULONG Flags,
    PUNICODE_STRING ModuleFileName,
    PHANDLE ModuleHandle
);
extern "C" FARPROC OriginalFuncs_version[16];
extern "C" LdrLoadDll_t oLdrLoadDll;

namespace Exports
{
	void Load();
}