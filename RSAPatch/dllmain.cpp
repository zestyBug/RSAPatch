#include <Windows.h>
#include <winternl.h>
#include <intrin.h>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <string>
#include "Utils.h"
#include "exports.h"
#include "../minhook/include/MinHook.h"


template <typename T>
class Array
{
	class Bounds
	{
	public:
		uintptr_t length;
		int32_t lower_bound;
	};
public:
	void* klass;
	void* monitor;
	Bounds* bounds;
	size_t max_length;
	T vector[32];

	size_t length() {
		if (bounds)
			return bounds->length;
		return max_length;
	}

};

class String
{
public:
	void* klass;
	void* monitor;
	uint32_t length;
	wchar_t chars[];

	wchar_t* c_str() {
		return chars;
	}

	size_t size() {
		return length;
	}
};


static BYTE* g_SyscallStub = NULL;
PVOID oGetPublicKey = nullptr;
PVOID oGetPrivateKey = nullptr;
PVOID oReadToEnd = nullptr;
LPCSTR gcpb = "<RSAKeyValue><Modulus>xbbx2m1feHyrQ7jP+8mtDF/pyYLrJWKWAdEv3wZrOtjOZzeLGPzsmkcgncgoRhX4dT+1itSMR9j9m0/OwsH2UoF6U32LxCOQWQD1AMgIZjAkJeJvFTrtn8fMQ1701CkbaLTVIjRMlTw8kNXvNA/A9UatoiDmi4TFG6mrxTKZpIcTInvPEpkK2A7Qsp1E4skFK8jmysy7uRhMaYHtPTsBvxP0zn3lhKB3W+HTqpneewXWHjCDfL7Nbby91jbz5EKPZXWLuhXIvR1Cu4tiruorwXJxmXaP1HQZonytECNU/UOzP6GNLdq0eFDE4b04Wjp396551G99YiFP2nqHVJ5OMQ==</Modulus><Exponent>AQAB</Exponent></RSAKeyValue>";

bool InitSyscallBypass() {
    if (g_SyscallStub) return true;
	DWORD g_SyscallNumber = 0;

    // Get syscall number
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) return false;
    
    BYTE* pFunc = (BYTE*)GetProcAddress(hNtdll, "NtProtectVirtualMemory");
    if (!pFunc) return false;

    // Find syscall number
    for (int i = 0; i < 64; i++)
        if (pFunc[i] == 0xB8) {
            g_SyscallNumber = *(DWORD*)(pFunc + i + 1);
            break;
        }

    if (g_SyscallNumber == 0)
        g_SyscallNumber = 0x0050;

    // Build the syscall stub in memory
    BYTE stub[] = {
        0x4C, 0x8B, 0xD1,              // mov r10, rcx
        0xB8, 0x00, 0x00, 0x00, 0x00,  // mov eax, XXXXXXXX
        0x0F, 0x05,                    // syscall
        0xC3                           // ret
    };

    // Put the syscall number in the stub
    *(DWORD*)(stub + 4) = g_SyscallNumber;

    SIZE_T g_StubSize = sizeof(stub);

    // Allocate executable memory
    g_SyscallStub = (BYTE*)VirtualAlloc(NULL, g_StubSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!g_SyscallStub) return false;

    // Copy the stub
    memcpy(g_SyscallStub, stub, g_StubSize);

    return true;
}

// Function type for the syscall stub
typedef NTSTATUS (NTAPI *SyscallFunc_t)(
    HANDLE, PVOID*, SIZE_T*, ULONG, PULONG
);

// Bypass function that calls the real syscall
extern "C" BOOL BypassVirtualProtect(
    LPVOID lpAddress,
    SIZE_T dwSize,
    DWORD flNewProtect,
    PDWORD lpflOldProtect
) { 
	if (!InitSyscallBypass())
        return FALSE;
    HANDLE hProcess = GetCurrentProcess();
    PVOID BaseAddress = lpAddress;
    SIZE_T RegionSize = dwSize;
    ULONG NewProtect = flNewProtect;
    ULONG OldProtect = 0;

    // Call direct syscall
	const SyscallFunc_t pFunc = (SyscallFunc_t)g_SyscallStub;
    NTSTATUS status = pFunc(hProcess, &BaseAddress, &RegionSize, NewProtect, &OldProtect);
    if (NT_SUCCESS(status)) {
        if (lpflOldProtect) {
            *lpflOldProtect = OldProtect;
        }
        return TRUE;
    }
	Utils::ConsolePrint("Error NtProtectVirtualMemory: %x\n", status);
    return FALSE;
}

std::string ReadFile(std::string path)
{
	std::ifstream ifs(std::filesystem::current_path() / path);
	if (!ifs.good())
	{
		Utils::ConsolePrint("Failed to Open: %s\n", path.c_str());
		return {};
	}

	std::string result;
	ifs >> result;
	return result;
}

Array<BYTE>* __fastcall hkGetRSAKey()
{
	static PVOID privateKeyRet = nullptr;
	static PVOID publicKeyRet = nullptr;

	auto ret = __builtin_return_address(0);

	// it will always called for private key first then public key
	if (!privateKeyRet)
		privateKeyRet = ret;
	else if (!publicKeyRet)
		publicKeyRet = ret;

	bool isPrivate = ret == privateKeyRet;
	auto data = decltype(&hkGetRSAKey)(isPrivate ? oGetPrivateKey : oGetPublicKey)();
	std::string customKey{};

	if (isPrivate)
	{
		Utils::ConsolePrint("private\n");
		customKey = ReadFile("PrivateKey.txt");
	}
	else
	{
		Utils::ConsolePrint("public\n");
		customKey = ReadFile("PublicKey.txt");
		if (customKey.empty())
		{
			Utils::ConsolePrint("using grasscutter public key\n");
			customKey = gcpb;
		}
	}

	if (!customKey.empty())
	{
		if (customKey.size() <= data->length())
		{
			ZeroMemory(data->vector, data->length());
			memcpy_s(data->vector, data->length(), customKey.data(), customKey.size());
		}
		else
		{
			Utils::ConsolePrint("custom key longer than original\n");
		}
	}

	for (int i = 0; i < data->length(); i++)
		Utils::ConsolePrint("%c", data->vector[i]);
	Utils::ConsolePrint("\n");

	return data;
}

String* __fastcall hkReadToEnd(void* rcx, void* rdx)
{
	auto result = decltype(&hkReadToEnd)(oReadToEnd)(rcx, rdx);
	if (!result)
		return result;

	if (!wcsstr(result->c_str(), L"<RSAKeyValue>"))
		return result;

	bool isPrivate = wcsstr(result->c_str(), L"<InverseQ>");
	std::string customKey{};

	if (isPrivate)
	{
		Utils::ConsolePrint("private\n");
		customKey = ReadFile("PrivateKey.txt");
	}
	else
	{
		Utils::ConsolePrint("public\n");
		customKey = ReadFile("PublicKey.txt");
		if (customKey.empty())
		{
			Utils::ConsolePrint("original:\n");
			Utils::ConsolePrint("%S\n\n", result->c_str());

			Utils::ConsolePrint("using grasscutter public key\n");
			customKey = gcpb;
		}
	}

	if (!customKey.empty())
	{
		if (customKey.size() <= result->size())
		{
			ZeroMemory(result->chars, result->size() * 2);
			std::wstring wstr = std::wstring(customKey.begin(), customKey.end()); // idc
			memcpy_s(result->chars, result->size() * 2, wstr.data(), wstr.size() * 2);
		}
		else
		{
			Utils::ConsolePrint("custom key longer than original\n");
		}
	}

	for (int i = 0; i < result->size(); i++)
		Utils::ConsolePrint("%C", result->chars[i]);
	Utils::ConsolePrint("\n\n");

	return result;
}

void DisableLogReport()
{
	char szProcessPath[MAX_PATH]{};
	GetModuleFileNameA(nullptr, szProcessPath, MAX_PATH);

	auto path = std::filesystem::path(szProcessPath);
	auto ProcessName = path.filename().string();
	ProcessName = ProcessName.substr(0, ProcessName.find_last_of('.'));

	auto Astrolabe = path.parent_path() / (ProcessName + "_Data\\Plugins\\Astrolabe.dll");
	auto MiHoYoMTRSDK = path.parent_path() / (ProcessName + "_Data\\Plugins\\MiHoYoMTRSDK.dll");

	// open exclusive access to these two dlls
	// so they cannot be loaded
	HANDLE hFile = CreateFileA(Astrolabe.string().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	hFile = CreateFileA(MiHoYoMTRSDK.string().c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
}

uintptr_t FindEntry(uintptr_t addr)
{
	try {
		while (true)
		{
			// walk back until we find function entry
			uint32_t code = *(uint32_t*)addr;
			code &= ~0xFF000000;

			if (_byteswap_ulong(code) == 0x4883EC00) // sub rsp, ??
				return addr;

			addr--;
		}
	}
	catch (...) {}

	return 0;
}

void OldVersion(HMODULE ModuleHandle) // <= 3.5.0 
{
	Utils::ConsolePrint("Using old method (v <= 3.5.0)\n");

	const char *func;
	MH_STATUS status;
	auto GetPublicKey = Utils::PatternScan(ModuleHandle, "48 BA 45 78 70 6F 6E 65 6E 74 48 89 90 ? ? ? ? 48 BA 3E 3C 2F 52 53 41 4B 65"); // 'Exponent></RSAKe'
	auto GetPrivateKey = Utils::PatternScan(ModuleHandle, "2F 49 6E 76 65 72 73 65"); // '/Inverse'

	GetPublicKey = FindEntry(GetPublicKey);
	GetPrivateKey = FindEntry(GetPrivateKey);

	Utils::ConsolePrint("GetPublicKey: %p\n", GetPublicKey);
	Utils::ConsolePrint("GetPrivateKey: %p\n", GetPrivateKey);

	// check for null and alignment
	if (!GetPublicKey || GetPublicKey % 16 > 0)
		Utils::ConsolePrint("Failed to find GetPublicKey - Need to update\n");
	if (!GetPrivateKey || GetPrivateKey % 16 > 0)
		Utils::ConsolePrint("Failed to find GetPrivateKey - Need to update\n");


	status = MH_CreateHook((PVOID)GetPublicKey, (void*)hkGetRSAKey, &oGetPublicKey);
	if (status != MH_OK) { func = "MH_CreateHook";goto err; }

	status = MH_CreateHook((PVOID)GetPrivateKey, (void*)hkGetRSAKey, &oGetPrivateKey);
	if (status != MH_OK) { func = "MH_CreateHook";goto err; }

	status = MH_EnableHook((PVOID)GetPublicKey);
	if (status != MH_OK) {
		MH_RemoveHook((PVOID)GetPublicKey);
		func = "MH_EnableHook";
		goto err;
	}

	status = MH_EnableHook((PVOID)GetPrivateKey);
	if (status != MH_OK) {
		MH_RemoveHook((PVOID)GetPublicKey);
		MH_RemoveHook((PVOID)GetPrivateKey);
		func = "MH_EnableHook";
		goto err;
	}

	Utils::ConsolePrint("Hooked GetPublicKey - Original at: %p\n", oGetPublicKey);
	Utils::ConsolePrint("Hooked GetPrivateKey - Original at: %p\n", oGetPrivateKey);
	return;
err:
	oReadToEnd = nullptr;
	Utils::ConsolePrint("%s: %s (%d)\n",func,MH_StatusToString(status),status);
}
void NewVersion(HMODULE ModuleHandle)
{
	const char *func;
	MH_STATUS status;
	Utils::ConsolePrint("Using new method (3.5.0 < v <= 4.0.0)\n");

	auto ReadToEnd = Utils::PatternScan(ModuleHandle, "48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 20 48 83 79 ? ? 48 8B D9 75 05");
	if (!ReadToEnd){
		Utils::ConsolePrint("Failed to find ReadToEnd - Need to update\n");
		return;
	}

	status = MH_CreateHook((PVOID)ReadToEnd, (void*)hkReadToEnd, &oReadToEnd);
	if (status != MH_OK) { func = "MH_CreateHook";goto err; }

	Utils::ConsolePrint("Target=%p Hook=%p Original=%p\n",ReadToEnd,hkReadToEnd,oReadToEnd);

	status = MH_EnableHook((PVOID)ReadToEnd);
	if (status != MH_OK) {
		MH_RemoveHook((PVOID)ReadToEnd);
		func = "MH_EnableHook";
		goto err;
	}

	Utils::ConsolePrint("Hooked ReadToEnd - Original at: %p\n", oReadToEnd);
	return;
err:
	oReadToEnd = nullptr;
	Utils::ConsolePrint("%s: %s (%d)\n",func,MH_StatusToString(status),status);
}

NTSTATUS NTAPI hkLdrLoadDll(PWCHAR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle)
{
    NTSTATUS result = oLdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);

	if (NT_SUCCESS(result) && ModuleFileName && ModuleFileName->Buffer)
	{
		const std::wstring name(ModuleFileName->Buffer, ModuleFileName->Length / sizeof(WCHAR));
		if(name.find(L"UserAssembly.dll") != std::wstring::npos || name.find(L"userassembly.dll") != std::wstring::npos)
		{
			Utils::ConsolePrint("found UserAssembly\n");
			auto UserAssembly = (HMODULE)*ModuleHandle;
			PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)UserAssembly;
			PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)UserAssembly + dos->e_lfanew);
			DWORD timestamp = nt->FileHeader.TimeDateStamp;

			if (timestamp <= 0x63ECA960) {
				OldVersion(UserAssembly);
			} else {
				NewVersion(UserAssembly);
			}
		}
	}

    return result;
}

void ACheckForThoseWhoCannotFollowInstructions(LPVOID instance)
{
	if (!instance)
	{
		// this shouldn't happen
		return;
	}

	char szModulePath[MAX_PATH]{};
	GetModuleFileNameA((HMODULE)instance, szModulePath, MAX_PATH);
	
	std::filesystem::path ModulePath = szModulePath;
	std::string ModuleName = ModulePath.filename().string();
	std::transform(ModuleName.begin(), ModuleName.end(), ModuleName.begin(), ::tolower);

	if (ModuleName == "version.dll")
	{
		// check mhypbase.dll
		auto mhypbase = GetModuleHandleA("mhypbase.dll");
		if (!mhypbase)
			return;

		PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)mhypbase;
		PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((uintptr_t)mhypbase + dosHeader->e_lfanew);
		auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
		
		// over 1MB
		if (sizeOfImage > 1 * 1024 * 1024) 
			return;

		// uh oh
	}
	else
	{
		// check version.dll
		auto version = GetModuleHandleA("version.dll");
		if (!version)
			return; // this shouldn't happen

		ZeroMemory(szModulePath, MAX_PATH);
		GetModuleFileNameA((HMODULE)version, szModulePath, MAX_PATH);
		ModuleName = szModulePath;
		std::transform(ModuleName.begin(), ModuleName.end(), ModuleName.begin(), ::tolower);

		if (ModuleName.find("system32") != std::string::npos)
			return;

		// uh oh
	}

	// https://www.youtube.com/watch?v=9a_3wQHcm_Y
	MessageBoxA(nullptr, "You may have more than one RSAPatch installed.\nPlease only use one RSAPatch to avoid instability.", "RSAPatch", MB_ICONWARNING);
}

DWORD __stdcall Thread(LPVOID p)
{
	HMODULE ntdll;
	PVOID addr;
	const char *func;
    MH_STATUS status;
	ACheckForThoseWhoCannotFollowInstructions(p);

    ntdll = GetModuleHandleA("ntdll.dll");
    addr = (PVOID)GetProcAddress(ntdll, "LdrLoadDll");
    status = MH_Initialize();
	if(status != MH_OK){
		func = "MH_Initialize";
		goto err;
	}
    status = MH_CreateHook(addr, (PVOID)hkLdrLoadDll, (void**)&oLdrLoadDll);
	if(status != MH_OK){
		func = "MH_CreateHook";
		goto err;
	}
    status = MH_EnableHook(addr);
	if(status != MH_OK){
		func = "MH_EnableHook";
		goto err;
	}
	Utils::ConsolePrint("hooked LdrLoadDll\n");
	return 0;
err:;
	Utils::ConsolePrint("%s: %s (%d)\n", func, MH_StatusToString(status),status);
	return 1;
}

DWORD __stdcall DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpReserved)
{
	if (hInstance)
		DisableThreadLibraryCalls(hInstance);

	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		HANDLE hThread = CreateThread(nullptr, 0, Thread, hInstance, 0, nullptr);
		if (!hThread)
			Utils::ConsolePrint("CreateThread failed: %lu\n", GetLastError());
		else
			CloseHandle(hThread);
	}

	return TRUE;
}

// this runs way before dllmain
void __stdcall TlsCallback(PVOID hModule, DWORD fdwReason, PVOID pContext)
{
	if (fdwReason != DLL_PROCESS_ATTACH) 
	    return;
		DisableLogReport();
		// for version.dll proxy
		// load exports as early as possible
		Utils::AttachConsole();
		Exports::Load();
}

extern "C" PIMAGE_TLS_CALLBACK tls_callback_func __attribute__((section(".CRT$XLB"), used)) = TlsCallback;