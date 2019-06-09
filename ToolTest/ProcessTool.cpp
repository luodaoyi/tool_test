#include "stdafx.h"
#include "ProcessTool.h"
#include "StringTool.h"
#include "DebugOutput.h"
#include <TlHelp32.h>
#include <algorithm>  
#include <Shlwapi.h>
#include <psapi.h> 
#include "ResManager.h"
#include <algorithm>
#include <ctype.h>





namespace process_tool
{
	BOOL IsProcessRunning(DWORD dwPid)
	{
		if (dwPid == 0) return FALSE;
		HANDLE hProcess = ::OpenProcess(SYNCHRONIZE, FALSE, dwPid);
		if (hProcess == NULL)
			return FALSE;
		DWORD wait_ret = WaitForSingleObject(hProcess, 0);
		::CloseHandle(hProcess);
		return wait_ret == WAIT_TIMEOUT;
	}

	/***************************************************************************************************/
	//	Function: 
	//		Inject
	//	
	//	Parameters:
	//		HANDLE hProcess - The handle to the process to inject the DLL into.
	//
	//		const char* dllname - The name of the DLL to inject into the process.
	//		
	//		const char* funcname - The name of the function to call once the DLL has been injected.
	//
	//	Description:
	//		This function will inject a DLL into a process and execute an exported function
	//		from the DLL to "initialize" it. The function should be in the format shown below,
	//		not parameters and no return type. Do not forget to prefix extern "C" if you are in C++
	//
	//			__declspec(dllexport) void FunctionName(void)
	//
	//		The function that is called in the injected DLL
	//		-MUST- return, the loader waits for the thread to terminate before removing the 
	//		allocated space and returning control to the Loader. This method of DLL injection
	//		also adds error handling, so the end user knows if something went wrong.
	/***************************************************************************************************/

	BOOL InjectDll(HANDLE hProcess, const std::wstring & dll_path, const std::wstring & dll_func_name, HMODULE * hDllModule)
	{
		//------------------------------------------//
		// Function variables.						//
		//------------------------------------------//
		if (hDllModule)
			*hDllModule = NULL;
		// Main DLL we will need to load
		char dllname[MAX_PATH] = { 0 };
		strcpy_s(dllname, string_tool::WideToChar(dll_path.c_str()).c_str());
		char funcname[MAX_PATH] = { 0 };
		strcpy_s(funcname, string_tool::WideToChar(dll_func_name.c_str()).c_str());

		HMODULE kernel32 = NULL;

		// Main functions we will need to import
		FARPROC loadlibrary = NULL;
		FARPROC getprocaddress = NULL;
		FARPROC exitprocess = NULL;
		FARPROC exitthread = NULL;
		FARPROC freelibraryandexitthread = NULL;

		// The workspace we will build the codecave on locally
		LPBYTE workspace = NULL;
		DWORD workspaceIndex = 0;

		// The memory in the process we write to
		LPVOID codecaveAddress = NULL;
		DWORD dwCodecaveAddress = 0;

		// Strings we have to write into the process
		char injectDllName[MAX_PATH + 1] = { 0 };
		char injectFuncName[MAX_PATH + 1] = { 0 };
		char injectError0[MAX_PATH + 1] = { 0 };
		char injectError1[MAX_PATH + 1] = { 0 };
		char injectError2[MAX_PATH + 1] = { 0 };
		char user32Name[MAX_PATH + 1] = { 0 };
		char msgboxName[MAX_PATH + 1] = { 0 };

		// Placeholder addresses to use the strings
		DWORD user32NameAddr = 0;
		DWORD user32Addr = 0;
		DWORD msgboxNameAddr = 0;
		DWORD msgboxAddr = 0;
		DWORD dllAddr = 0;
		DWORD dllNameAddr = 0;
		DWORD funcNameAddr = 0;
		DWORD error0Addr = 0;
		DWORD error1Addr = 0;
		DWORD error2Addr = 0;

		// Where the codecave execution should begin at
		DWORD codecaveExecAddr = 0;

		// Handle to the thread we create in the process
		HANDLE hThread = NULL;

		// Temp variables
		DWORD dwTmpSize = 0;

		// Old protection on page we are writing to in the process and the bytes written
		DWORD oldProtect = 0;
		SIZE_T bytesRet = 0;

		//------------------------------------------//
		// Variable initialization.					//
		//------------------------------------------//

		// Get the address of the main DLL
		kernel32 = LoadLibraryA("kernel32.dll");

		// Get our functions
		loadlibrary = GetProcAddress(kernel32, "LoadLibraryA");
		getprocaddress = GetProcAddress(kernel32, "GetProcAddress");
		exitprocess = GetProcAddress(kernel32, "ExitProcess");
		exitthread = GetProcAddress(kernel32, "ExitThread");
		freelibraryandexitthread = GetProcAddress(kernel32, "FreeLibraryAndExitThread");

		// This section will cause compiler warnings on VS8, 
		// you can upgrade the functions or ignore them

		// Build names
		_snprintf_s(injectDllName, MAX_PATH, "%s", dllname);
		_snprintf_s(injectFuncName, MAX_PATH, "%s", funcname);
		_snprintf_s(user32Name, MAX_PATH, "user32.dll");
		_snprintf_s(msgboxName, MAX_PATH, "MessageBoxA");

		// Build error messages
		_snprintf_s(injectError0, MAX_PATH, "Error");
		_snprintf_s(injectError1, MAX_PATH, "Could not load the dll: %s", injectDllName);
		_snprintf_s(injectError2, MAX_PATH, "Could not load the function: %s", injectFuncName);

		// StartCmdSession the workspace
		workspace = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024);

		// Allocate space for the codecave in the process
		codecaveAddress = VirtualAllocEx(hProcess, 0, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		OutputDebugStr(L"Allocated code Addr:%x", codecaveAddress);
		if (!codecaveAddress)
		{
			OutputDebugStr(L"!!!Allocated code Addr Failed");
			return FALSE;
		}
			
		dwCodecaveAddress = PtrToUlong(codecaveAddress);

		//Allocate space for save loadlbiray ret
		LPVOID dll_handle_address = VirtualAllocEx(hProcess, 0, sizeof(HMODULE), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		OutputDebugStr(L"Allocated DllHandleAddr Addr:%x", dll_handle_address);
		if (!dll_handle_address)
		{
			OutputDebugStr(L"!!!Allocated DllHandleAddr Addr Failed");
			return FALSE;
		}
			
		
// 		dwTmpSize = 0x12345678;//test write memory
// 		::WriteProcessMemory(hProcess, dll_handle_address, &dwTmpSize, sizeof(HMODULE), NULL);

		// Note there is no error checking done above for any functions that return a pointer/handle.
		// I could have added them, but it'd just add more messiness to the code and not provide any real
		// benefit. It's up to you though in your final code if you want it there or not.

		//------------------------------------------//
		// Data and string writing.					//
		//------------------------------------------//

		// Write out the address for the user32 dll address
		user32Addr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = 0;
		memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
		workspaceIndex += 4;

		// Write out the address for the MessageBoxA address
		msgboxAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = 0;
		memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
		workspaceIndex += 4;

		// Write out the address for the injected DLL's module
		dllAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = 0;
		memcpy(workspace + workspaceIndex, &dwTmpSize, 4);
		workspaceIndex += 4;

		// User32 Dll Name
		user32NameAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(user32Name) + 1;
		memcpy(workspace + workspaceIndex, user32Name, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// MessageBoxA name
		msgboxNameAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(msgboxName) + 1;
		memcpy(workspace + workspaceIndex, msgboxName, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Dll Name
		dllNameAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(injectDllName) + 1;
		memcpy(workspace + workspaceIndex, injectDllName, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Function Name
		funcNameAddr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(injectFuncName) + 1;
		memcpy(workspace + workspaceIndex, injectFuncName, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Error Message 1
		error0Addr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(injectError0) + 1;
		memcpy(workspace + workspaceIndex, injectError0, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Error Message 2
		error1Addr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(injectError1) + 1;
		memcpy(workspace + workspaceIndex, injectError1, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Error Message 3
		error2Addr = workspaceIndex + dwCodecaveAddress;
		dwTmpSize = (DWORD)strlen(injectError2) + 1;
		memcpy(workspace + workspaceIndex, injectError2, dwTmpSize);
		workspaceIndex += dwTmpSize;

		// Pad a few INT3s after string data is written for seperation
		workspace[workspaceIndex++] = 0xCC;
		workspace[workspaceIndex++] = 0xCC;
		workspace[workspaceIndex++] = 0xCC;

		// Store where the codecave execution should begin
		codecaveExecAddr = workspaceIndex + dwCodecaveAddress;
		OutputDebugStr(L"codecaveExecAddr Addr:%x", codecaveExecAddr);

		// For debugging - infinite loop, attach onto process and step over
		//workspace[workspaceIndex++] = 0xEB;
		//workspace[workspaceIndex++] = 0xFE;

		//------------------------------------------//
		// User32.dll loading.						//
		//------------------------------------------//

		// User32 DLL Loading
		// PUSH 0x00000000 - Push the address of the DLL name to use in LoadLibraryA
		workspace[workspaceIndex++] = 0x68;
		memcpy(workspace + workspaceIndex, &user32NameAddr, 4);
		workspaceIndex += 4;

		// MOV EAX, ADDRESS - Move the address of LoadLibraryA into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &loadlibrary, 4);
		workspaceIndex += 4;

		// CALL EAX - Call LoadLibraryA
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;

		// MessageBoxA Loading
		// PUSH 0x000000 - Push the address of the function name to load
		workspace[workspaceIndex++] = 0x68;
		memcpy(workspace + workspaceIndex, &msgboxNameAddr, 4);
		workspaceIndex += 4;

		// Push EAX, module to use in GetProcAddress
		workspace[workspaceIndex++] = 0x50;

		// MOV EAX, ADDRESS - Move the address of GetProcAddress into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &getprocaddress, 4);
		workspaceIndex += 4;

		// CALL EAX - Call GetProcAddress
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;

		// MOV [ADDRESS], EAX - Save the address to our variable
		workspace[workspaceIndex++] = 0xA3;
		memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
		workspaceIndex += 4;

		//------------------------------------------//
		// Injected dll loading.					//
		//------------------------------------------//

		/*
		// This is the way the following assembly code would look like in C/C++

		// Load the injected DLL into this process
		HMODULE h = LoadLibrary("mydll.dll");
		if(!h)
		{
		MessageBox(0, "Could not load the dll: mydll.dll", "Error", MB_ICONERROR);
		ExitProcess(0);
		}

		// Get the address of the export function
		FARPROC p = GetProcAddress(h, "Initialize");
		if(!p)
		{
		MessageBox(0, "Could not load the function: Initialize", "Error", MB_ICONERROR);
		ExitProcess(0);
		}

		// So we do not need a function pointer interface
		__asm call p

		// Exit the thread so the loader continues
		ExitThread(0);
		*/

		// DLL Loading
		// PUSH 0x00000000 - Push the address of the DLL name to use in LoadLibraryA
		workspace[workspaceIndex++] = 0x68;
		memcpy(workspace + workspaceIndex, &dllNameAddr, 4);
		workspaceIndex += 4;

		// MOV EAX, ADDRESS - Move the address of LoadLibraryA into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &loadlibrary, 4);
		workspaceIndex += 4;

		// CALL EAX - Call LoadLibraryA
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;

		//Mov dll_handle_address,eax
		workspace[workspaceIndex++] = 0xA3;
		memcpy(workspace + workspaceIndex, &dll_handle_address, 4);
		workspaceIndex += 4;

		// Error Checking
		// CMP EAX, 0
		workspace[workspaceIndex++] = 0x83;
		workspace[workspaceIndex++] = 0xF8;
		workspace[workspaceIndex++] = 0x00;

		// JNZ EIP + 0x1E to skip over eror code
		workspace[workspaceIndex++] = 0x75;
		workspace[workspaceIndex++] = 0x9; //后面注释了MessageBox所以这将1E改为9
		/*
		// Error Code 1
		// MessageBox
		// PUSH 0x10 (MB_ICONHAND)
		workspace[workspaceIndex++] = 0x6A;
		workspace[workspaceIndex++] = 0x10;

		// PUSH 0x000000 - Push the address of the MessageBox title
		workspace[workspaceIndex++] = 0x68;
		memcpy(workspace + workspaceIndex, &error0Addr, 4);
		workspaceIndex += 4;

		// PUSH 0x000000 - Push the address of the MessageBox message
		workspace[workspaceIndex++] = 0x68;
		memcpy(workspace + workspaceIndex, &error1Addr, 4);
		workspaceIndex += 4;

		// Push 0
		workspace[workspaceIndex++] = 0x6A;
		workspace[workspaceIndex++] = 0x00;

		// MOV EAX, [ADDRESS] - Move the address of MessageBoxA into EAX
		workspace[workspaceIndex++] = 0xA1;
		memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
		workspaceIndex += 4;

		// CALL EAX - Call MessageBoxA
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;
		*/

		// ExitProcess
		// Push 0
		workspace[workspaceIndex++] = 0x6A;
		workspace[workspaceIndex++] = 0x00;

		// MOV EAX, ADDRESS - Move the address of ExitProcess into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &exitprocess, 4);
		workspaceIndex += 4;

		// CALL EAX - Call ExitProcess
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;

		//	Now we have the address of the injected DLL, so save the handle
		if (funcname && strlen(funcname) > 0)
		{
			// MOV [ADDRESS], EAX - Save the address to our variable
			workspace[workspaceIndex++] = 0xA3;
			memcpy(workspace + workspaceIndex, &dllAddr, 4);
			workspaceIndex += 4;

			// Load the initilize function from it

			// PUSH 0x000000 - Push the address of the function name to load
			workspace[workspaceIndex++] = 0x68;
			memcpy(workspace + workspaceIndex, &funcNameAddr, 4);
			workspaceIndex += 4;

			// Push EAX, module to use in GetProcAddress
			workspace[workspaceIndex++] = 0x50;

			// MOV EAX, ADDRESS - Move the address of GetProcAddress into EAX
			workspace[workspaceIndex++] = 0xB8;
			memcpy(workspace + workspaceIndex, &getprocaddress, 4);
			workspaceIndex += 4;

			// CALL EAX - Call GetProcAddress
			workspace[workspaceIndex++] = 0xFF;
			workspace[workspaceIndex++] = 0xD0;

			// Error Checking
			// CMP EAX, 0
			workspace[workspaceIndex++] = 0x83;
			workspace[workspaceIndex++] = 0xF8;
			workspace[workspaceIndex++] = 0x00;

			// JNZ EIP + 0x1C to skip eror code
			workspace[workspaceIndex++] = 0x75;
			workspace[workspaceIndex++] = 0x9; //将原来1C改为9，由于注释了下面的MessageBox

			/*
			// Error Code 2
			// MessageBox
			// PUSH 0x10 (MB_ICONHAND)
			workspace[workspaceIndex++] = 0x6A;
			workspace[workspaceIndex++] = 0x10;

			// PUSH 0x000000 - Push the address of the MessageBox title
			workspace[workspaceIndex++] = 0x68;
			memcpy(workspace + workspaceIndex, &error0Addr, 4);
			workspaceIndex += 4;

			// PUSH 0x000000 - Push the address of the MessageBox message
			workspace[workspaceIndex++] = 0x68;
			memcpy(workspace + workspaceIndex, &error2Addr, 4);
			workspaceIndex += 4;

			// Push 0
			workspace[workspaceIndex++] = 0x6A;
			workspace[workspaceIndex++] = 0x00;

			// MOV EAX, ADDRESS - Move the address of MessageBoxA into EAX
			workspace[workspaceIndex++] = 0xA1;
			memcpy(workspace + workspaceIndex, &msgboxAddr, 4);
			workspaceIndex += 4;

			*/
			// CALL EAX - Call MessageBoxA Or Call dll Func
			workspace[workspaceIndex++] = 0xFF;
			workspace[workspaceIndex++] = 0xD0;
		

			// ExitProcess
			// Push 0
			workspace[workspaceIndex++] = 0x6A;
			workspace[workspaceIndex++] = 0x00;

			// MOV EAX, ADDRESS - Move the address of ExitProcess into EAX
			workspace[workspaceIndex++] = 0xB8;
			memcpy(workspace + workspaceIndex, &exitprocess, 4);
			workspaceIndex += 4;

			//	Now that we have the address of the function, we cam call it, 
			// if there was an error, the messagebox would be called as well.

			// CALL EAX - Call ExitProcess -or- the Initialize function
			workspace[workspaceIndex++] = 0xFF;
			workspace[workspaceIndex++] = 0xD0;
		}

		// If we get here, the Initialize function has been called, 
		// so it's time to close this thread and optionally unload the DLL.

		//------------------------------------------//
		// Exiting from the injected dll.			//
		//------------------------------------------//

		// Call ExitThread to leave the DLL loaded
#if 1
		// Push 0 (exit code)
		workspace[workspaceIndex++] = 0x6A;
		workspace[workspaceIndex++] = 0x00;

		// MOV EAX, ADDRESS - Move the address of ExitThread into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &exitthread, 4);
		workspaceIndex += 4;

		// CALL EAX - Call ExitThread
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;
#endif

		// Call FreeLibraryAndExitThread to unload DLL
#if 0
		// Push 0 (exit code)
		workspace[workspaceIndex++] = 0x6A;
		workspace[workspaceIndex++] = 0x00;

		// PUSH [0x000000] - Push the address of the DLL module to unload
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0x35;
		memcpy(workspace + workspaceIndex, &dllAddr, 4);
		workspaceIndex += 4;

		// MOV EAX, ADDRESS - Move the address of FreeLibraryAndExitThread into EAX
		workspace[workspaceIndex++] = 0xB8;
		memcpy(workspace + workspaceIndex, &freelibraryandexitthread, 4);
		workspaceIndex += 4;

		// CALL EAX - Call FreeLibraryAndExitThread
		workspace[workspaceIndex++] = 0xFF;
		workspace[workspaceIndex++] = 0xD0;
#endif

		//------------------------------------------//
		// Code injection and cleanup.				//
		//------------------------------------------//

		// Change page protection so we can write executable code
		VirtualProtectEx(hProcess, codecaveAddress, workspaceIndex, PAGE_EXECUTE_READWRITE, &oldProtect);

		// Write out the patch
		if (!WriteProcessMemory(hProcess, codecaveAddress, workspace, workspaceIndex, &bytesRet))
			OutputDebugStr(L"!!! WriteProcessMemory  codecaveAddress, workspace Failed!");

		// Restore page protection
		VirtualProtectEx(hProcess, codecaveAddress, workspaceIndex, oldProtect, &oldProtect);

		// Make sure our changes are written right away
		FlushInstructionCache(hProcess, codecaveAddress, workspaceIndex);

		// Free the workspace memory
		HeapFree(GetProcessHeap(), 0, workspace);

		// Execute the thread now and wait for it to exit, note we execute where the code starts, and not the codecave start
		// (since we wrote strings at the start of the codecave) -- NOTE: void* used for VC6 compatibility instead of UlongToPtr
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((void*)codecaveExecAddr), 0, 0, NULL);
		if(!hThread)
			OutputDebugStr(L"!!!CreateRemoteThread Failed!");
		WaitForSingleObject(hThread, INFINITE);

		//Get DLl Handle
		HMODULE RemoteHandle = 0;
		::ReadProcessMemory(hProcess, dll_handle_address, &RemoteHandle, 4, NULL);
		VirtualFreeEx(hProcess, dll_handle_address, 0, MEM_RELEASE);


		// Free the memory in the process that we allocated
		VirtualFreeEx(hProcess, codecaveAddress, 0, MEM_RELEASE);
		if (RemoteHandle > 0)
		{
			if (hDllModule)
				*hDllModule = RemoteHandle;
			return TRUE;
		}
		else
			return FALSE;

	}

	BOOL InjectDll_CallFunc(DWORD dwPid, const std::wstring & dll_path, const std::wstring & dll_func_name, HMODULE  * injected_dll_module)
	{
		HANDLE hProcess = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwPid);
		if (hProcess == NULL)
			return FALSE;
		HMODULE remote_moudule = 0;
		InjectDll(hProcess,dll_path,dll_func_name,&remote_moudule);
		::CloseHandle(hProcess);
		if (injected_dll_module)
			*injected_dll_module = remote_moudule;
		return remote_moudule > 0;
	}

	BOOL FreeRemoteDll(DWORD dwPid, HMODULE hDll)
	{
		if (!hDll)
		{
			OutputDebugStr(L"FreeRemoteDll hDll Is NULL ErrID:%d", ::GetLastError());
			return FALSE;
		}
		HANDLE hProcess = ::OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwPid);
		if (hProcess == NULL)
		{
			OutputDebugStr(L"FreeRemoteDll OpenProcess Failed! %d", ::GetLastError());
			return FALSE;
		}

		HANDLE hThread = ::CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) ::GetProcAddress(GetModuleHandle(L"Kernel32"), "FreeLibrary"), (void*)hDll, 0, NULL);
		if (hThread == NULL)
		{
			OutputDebugStr(L"FreeRemoteDll CreateRemoteThread Failed errorid:%d", ::GetLastError());
			return FALSE;
		}
		::WaitForSingleObject(hThread, INFINITE);
		DWORD dwRet = 0;
		GetExitCodeThread(hThread, &dwRet);
		::CloseHandle(hThread);
		if (dwRet == 0)
		{
			OutputDebugStr(L"FreeRemoteDll FreeLibray Failed!");
			return FALSE;
		}
		else
			return TRUE;
	}


	BOOL StartProcess(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, DWORD dwCreateFlag, _Out_ DWORD * Pid,BOOL bInherit, _Out_ PHANDLE phProcess , _Out_ PHANDLE phThread ,BOOL isHide)
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(STARTUPINFO);

		if (isHide)
			si.dwFlags = STARTF_USESHOWWINDOW;

		BOOL bOk = ::CreateProcess(app_name, (LPWSTR)cmd_line, NULL, NULL, bInherit, dwCreateFlag, NULL, cur_path, &si, &pi);
		if (!bOk)
			return FALSE;
		else
		{
			if (!phProcess)
				::CloseHandle(pi.hProcess);
			else
				*phProcess = pi.hProcess;
			if (!phThread)
				::CloseHandle(pi.hThread);
			else
				*phThread = pi.hThread;
			if (Pid)
				*Pid = pi.dwProcessId;
			return TRUE;
		}
	}

	BOOL StartProcessWithToken(HANDLE hToken, LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, DWORD dwCreateFlag, _Out_ DWORD * Pid , BOOL bInherit , _Out_ PHANDLE phProcess , _Out_ PHANDLE phThread,BOOL ishide)
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(STARTUPINFO);
		if (ishide)
			si.dwFlags = STARTF_USESHOWWINDOW;
		BOOL bOk = ::CreateProcessAsUser(hToken,app_name, (LPWSTR)cmd_line, NULL, NULL, bInherit, dwCreateFlag, NULL, cur_path, &si, &pi);
		if (!bOk)
			return FALSE;
		else
		{
			if (!phProcess)
				::CloseHandle(pi.hProcess);
			else
				*phProcess = pi.hProcess;
			if (!phThread)
				::CloseHandle(pi.hThread);
			else
				*phThread = pi.hThread;
			if (Pid)
				*Pid = pi.dwProcessId;
			return TRUE;
		}
	}

	DWORD StartProcessAndGetExitCode(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path,DWORD max_time,BOOL isHide )
	{
		HANDLE process_handle = INVALID_HANDLE_VALUE;
		HANDLE thread_handle = INVALID_HANDLE_VALUE;
		std::wstring process_name = app_name ? app_name : cmd_line;
		if (!StartProcess(app_name, cmd_line, cur_path, 0, 0, 0, &process_handle, &thread_handle,isHide))
		{
			DWORD last_error = ::GetLastError();
			OutputDebugStr(L"创建进程%s出错,错误ID:%d", process_name.c_str(), last_error);
			return error_process_exit_code;
		}

		SetResDeleter(process_handle, [](HANDLE & h){if(h && h != INVALID_HANDLE_VALUE) CloseHandle(h); });
		SetResDeleter(thread_handle, [](HANDLE & h){if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h); });

		//LOGW(notice) << L"等待进程结束。。。";
		DWORD wait_ret = ::WaitForSingleObject(process_handle, max_time);
		if (wait_ret == WAIT_OBJECT_0)
		{
			//LOGW(notice) << L"进程结束";
			DWORD ret_code = error_process_exit_code;
			if (!GetExitCodeProcess(process_handle, &ret_code))
			{
				DWORD last_error = 0;
				//LOGW(error)<<L"StartProcessAndGetExitCode GetExitCodeProcess Failed:" << GetLastError();
				return error_process_exit_code;
			}
			else
				return ret_code;
		}
		else
		{
			//LOGW_FMT(error,L"Wait Process %s Failed", process_name.c_str());
			return error_process_exit_code;
		}
	}


	BOOL StartProcessAndInjectDllAndCallDllFunc(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, const std::wstring & dll_path, const std::wstring & dll_func_name, _Out_ DWORD & Pid, _Out_ PHANDLE phProcess , _Out_ PHANDLE phThread )
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(STARTUPINFO);
		if (!::CreateProcess(app_name, (LPWSTR)cmd_line, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, cur_path, &si, &pi))
		{
			DWORD nError = GetLastError();
			OutputDebugStr(L"创建进程失败[%d]%s %s", nError, app_name ? app_name : L"", cmd_line ? cmd_line : L"");
			return FALSE;
		}
		else
		{
			//Inject dll and call a dll function
			HMODULE remote_module = 0;
			InjectDll(pi.hProcess, dll_path, dll_func_name,&remote_module);
			ResumeThread(pi.hThread);
			if (phProcess)
				*phProcess = pi.hProcess;
			else
				::CloseHandle(pi.hProcess);
			if (phThread)
				*phThread = pi.hThread;
			else
				::CloseHandle(pi.hThread);
			Pid = pi.dwProcessId;
			return remote_module > 0;
		}
	}


	DWORD GetPidFromExeName(const std::wstring & szExeName,const  DWORD ParentId)
	{
		auto pid_list = GetPidsFromExeName(szExeName, ParentId);
		if (pid_list.size() > 0)
			return pid_list[0].th32ProcessID;
		else
			return NULL;
	}

	DWORD GetParentPid(DWORD dwPid)
	{
		auto pid_list =  GetPidsByCondition([dwPid](const PROCESSENTRY32 & process_info) {
			return process_info.th32ProcessID == dwPid;
		});
		if (pid_list.size() > 0)
			return pid_list[0].th32ParentProcessID;
		return 0;
	}

	std::vector<PROCESSENTRY32> GetPidsFromExeName(const std::wstring & szExeName, const  DWORD ParentId)
	{
		std::wstring search_exe_name_lower;
		std::transform(szExeName.begin(), szExeName.end(), std::back_inserter(search_exe_name_lower) , towlower);
		return GetPidsByCondition([& search_exe_name_lower, ParentId](const PROCESSENTRY32 & process_info){
			std::wstring process_exe_name = process_info.szExeFile;
			std::transform(process_exe_name.begin(), process_exe_name.end(), process_exe_name.begin(), towlower);
			return  process_exe_name == search_exe_name_lower && (ParentId == 0 || process_info.th32ParentProcessID == ParentId || GetParentPid(process_info.th32ParentProcessID) == ParentId);
		});
	}

	std::vector<PROCESSENTRY32> GetPidsByCondition(std::function<bool(const PROCESSENTRY32 & process_info)> fnCheck)
	{
		std::vector<PROCESSENTRY32> ret_pid_list;
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		SetResDeleter(handle, [](HANDLE & h){CloseHandle(h); });
		BOOL ret = FALSE;
		PROCESSENTRY32 info;//声明进程信息变量
		info.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(handle, &info))
		{
			if (fnCheck(info))
				ret_pid_list.push_back(info);
			else
			{
				while (Process32Next(handle, &info) != FALSE)
				{
					if (fnCheck(info))
						ret_pid_list.push_back(info);
				}
			}
		}
		return ret_pid_list;
	}

	BOOL IsMutiInstance(const std::wstring & name)
	{
		CreateMutex(NULL, TRUE, name.c_str());
		if (::GetLastError() == ERROR_ALREADY_EXISTS)
			return TRUE;
		else
			return FALSE;
	}

	BOOL KillProcess(DWORD dwPid)
	{
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
		if (hProcess == NULL)
			return FALSE;
		if (!::TerminateProcess(hProcess,0))
		{
			OutputDebugStr(L"Terminate Process Failed:%d", dwPid);
			return FALSE;
		}
		else
			return TRUE;
	}

	HWND FindProcessWnd(const DWORD dwPid, LPWSTR class_name, LPWSTR caption_name)
	{
		for (HWND pre_window = NULL;;)
		{
			HWND cur_window = ::FindWindowEx(NULL, pre_window, class_name, caption_name);
			if (cur_window == NULL)
				break;
			DWORD cur_window_pid = 0;
			::GetWindowThreadProcessId(cur_window, &cur_window_pid);
			if (cur_window_pid == dwPid)
				return cur_window;
			pre_window = cur_window;
		}
		return NULL;
	}
	
	HWND WaitForProcessWindow(const DWORD dwPid, DWORD milli_seconds, LPWSTR class_name, LPWSTR caption_name)
	{
		DWORD begin_time = GetTickCount();
		HWND wnd_ret = NULL;
		for (;;)
		{
			if (!wnd_ret)
				wnd_ret = FindProcessWnd(dwPid, class_name, caption_name);
			else
				if (::IsWindowVisible(wnd_ret))
					return wnd_ret;
			if (GetTickCount() - begin_time > milli_seconds)
				return NULL;
			if (!IsProcessRunning(dwPid))
				return NULL;
			Sleep(1000);
		}
		return NULL;
	}

	DWORD GetWindowThreadID(HWND hWnd)
	{
		return GetWindowThreadProcessId(hWnd, NULL);
	}

	DWORD GetWindowProcessID(HWND hWnd)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(hWnd,&pid);
		return pid;
	}








	BOOL GetProcessExePath(DWORD dwPid,std::wstring & full_path)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
		if (!hProcess)
			return FALSE;
		TCHAR file_name[MAX_PATH] = { 0 };
		if (GetModuleFileNameEx(hProcess, NULL, file_name, MAX_PATH))
		{
			full_path = file_name;
			return TRUE;
		}
		else
			return FALSE;
	}

	BOOL InjectDllNormal(HANDLE hProcess, const std::wstring & lib_name)
	{
		wchar_t dllPath[MAX_PATH + 1] = { 0 };
		wcscpy_s(dllPath, lib_name.c_str());

		static HMODULE kernel32 = GetModuleHandleA("kernel32.dll");

		if (kernel32 == NULL)
		{
			OutputDebugStr(L"Couldn't get handle for kernel32.dll");
			return FALSE;
		}

		void *remoteMem =
			VirtualAllocEx(hProcess, NULL, sizeof(dllPath), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (remoteMem)
		{
			if (!WriteProcessMemory(hProcess, remoteMem, (void *)dllPath, sizeof(dllPath), NULL))
				return FALSE;

			HANDLE hThread = CreateRemoteThread(
				hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryW"),
				remoteMem, 0, NULL);
			if (hThread)
			{
				WaitForSingleObject(hThread, INFINITE);
				CloseHandle(hThread);
			}
			VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
			return TRUE;
		}
		else
		{
			OutputDebugStr(L"Couldn't allocate remote memory for DLL '%s'", lib_name.c_str());
			return FALSE;
		}
	}

	uintptr_t FindRemoteDLL(DWORD pid, wstring libNameSrc)
	{
		HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
		std::wstring libName(libNameSrc);
		std::transform(libName.begin(), libName.end(), libName.begin(), towlower);

		// up to 10 retries
		for (int i = 0; i < 10; i++)
		{
			hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

			if (hModuleSnap == INVALID_HANDLE_VALUE)
			{
				DWORD err = GetLastError();

				OutputDebugStr(L"CreateToolhelp32Snapshot(%u) -> 0x%08x", pid, err);

				// retry if error is ERROR_BAD_LENGTH
				if (err == ERROR_BAD_LENGTH)
					continue;
			}

			// didn't retry, or succeeded
			break;
		}

		if (hModuleSnap == INVALID_HANDLE_VALUE)
		{
			OutputDebugStr(L"Couldn't create toolhelp dump of modules in process %u", pid);
			return 0;
		}

		MODULEENTRY32 me32 = { 0 };
		me32.dwSize = sizeof(MODULEENTRY32);

		BOOL success = Module32First(hModuleSnap, &me32);

		if (success == FALSE)
		{
			DWORD err = GetLastError();

			OutputDebugStr(L"Couldn't get first module in process %u: 0x%08x", pid, err);
			CloseHandle(hModuleSnap);
			return 0;
		}

		uintptr_t ret = 0;

		int numModules = 0;

		do
		{
			wchar_t modnameLower[MAX_MODULE_NAME32 + 1] = { 0 };
			wcsncpy_s(modnameLower, me32.szModule, MAX_MODULE_NAME32);

			wchar_t *wc = &modnameLower[0];
			while (*wc)
			{
				*wc = towlower(*wc);
				wc++;
			}

			numModules++;

			if (wcsstr(modnameLower, libName.c_str()) == modnameLower)
			{
				ret = (uintptr_t)me32.modBaseAddr;
			}
		} while (ret == 0 && Module32Next(hModuleSnap, &me32));

		if (ret == 0)
		{
			HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

			DWORD exitCode = 0;

			if (h)
				GetExitCodeProcess(h, &exitCode);

			if (h == NULL || exitCode != STILL_ACTIVE)
			{
				OutputDebugStr(
					L"Error injecting into remote process with PID %u which is no longer available.\n"
					"Possibly the process has crashed during early startup, or is missing DLLs to run?",
					pid);
			}
			else
			{
				OutputDebugStr(L"Couldn't find module '%s' among %d modules", libName.c_str(), numModules);
			}

			if (h)
				CloseHandle(h);
		}

		CloseHandle(hModuleSnap);

		return ret;
	}





	
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)  
	typedef struct tagTHREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	} THREADNAME_INFO;
#pragma pack(pop)  


	void SetThreadName(DWORD dwThreadID, const char* threadName) {
		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = threadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
#pragma warning(push)  
#pragma warning(disable: 6320 6322)  
		__try{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
		}
#pragma warning(pop)  
	}
}


