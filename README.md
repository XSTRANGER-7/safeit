# ProcessHider

Process Hider uses Windows API Hooking to Hide a Process from TaskManager. It utilises <a href = "https://github.com/TsudaKageyu/minhook">Minhook Library</a> to hook NtQuerySystemInformation function so whenever NtQuerySystemInformation is called our function executes which removes the chosen process from Process List returned by the original function. 

There are 2 projects inside the repo, the main one is ProcessHider which produces the DLL that is injected inside Task Manager. The second one is the DLL injector whose main job is to inject the DLL and pass the name of the process to hide.

## To Compile:
  ``` Clone the repo and open the solution file in Visual Studio```

## How to Run:

### Step 1: Build the DLL Injector
Navigate to the DLL_Injector directory and compile the project:
```
cd DLL_Injector
cl.exe /EHsc /nologo /Fe:main.exe main.cpp /link /SUBSYSTEM:WINDOWS /MANIFESTUAC:"level='requireAdministrator' uiAccess='false'"
```

### Step 2: Run the Executable with Administrator Privileges
Execute the compiled main.exe file with administrative rights:
```
.\main.exe
```
*Note: The application requires administrator privileges to inject the DLL into TaskManager.*

## Process Management Commands:

### See Running Processes
To view all running processes and search for a specific one:
```
tasklist | findstr main.exe
```
This will display the process ID and memory usage if the process is running.

### Kill a Process
To terminate a running process:
```
taskkill /F /IM main.exe
```
- `/F` - Forces termination of the process
- `/IM` - Specifies the image name (executable name) of the process to terminate
