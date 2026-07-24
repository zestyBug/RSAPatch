#pragma once
#include <Windows.h>
#include <vector>
#include <string>

extern "C" FARPROC OriginalFuncs_version[16];


namespace Exports
{
	void Load();
}