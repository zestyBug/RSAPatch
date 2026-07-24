#include "exports.h"
#include "Utils.h"
#include <filesystem>

FARPROC OriginalFuncs_version[16];
FARPROC OriginalFuncs_version[17];

inline constexpr const char* ExportNames_version[] = {
		"GetFileVersionInfoA",
		"GetFileVersionInfoExA",
		"GetFileVersionInfoExW",
		"GetFileVersionInfoSizeA",
		"GetFileVersionInfoSizeExA",
		"GetFileVersionInfoSizeExW",
		"GetFileVersionInfoSizeW",
		"GetFileVersionInfoW",
		"VerFindFileA",
		"VerFindFileW",
		"VerInstallFileA",
		"VerInstallFileW",
		"VerLanguageNameA",
		"VerLanguageNameW",
		"VerQueryValueA",
		"VerQueryValueW"
};

void Exports::Load()
{
	char szSystemDirectory[MAX_PATH]{};
	GetSystemDirectoryA(szSystemDirectory, MAX_PATH);

	std::string OriginalPath = szSystemDirectory;
	OriginalPath += "\\version.dll";

	HMODULE version = LoadLibraryA(OriginalPath.c_str());
	// load version.dll from system32
	if (!version)
	{
		Utils::ConsolePrint("Failed to load version.dll from system32\n");
		return;
	}

	// get addresses of original functions
	for (int i = 0; i < 16; i++)
	{
		OriginalFuncs_version[i] = GetProcAddress(version, ExportNames_version[i]);
		if (!OriginalFuncs_version[i])
		{
			Utils::ConsolePrint("Failed to get address of %s\n", ExportNames_version[i]);
			return;
		}
	}
	
	Utils::ConsolePrint("Loaded %s\n", OriginalPath.c_str());
}
