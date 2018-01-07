#include "stdafx.h"
#include "ProcessTool.h"
#include "StringTool.h"
#include "DebugOutput.h"
#include <TlHelp32.h>
#include <algorithm>  
#include <Shlwapi.h>
#include <psapi.h> 
#include "ResManager.h"
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

		// Create the workspace
		workspace = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024);

		// Allocate space for the codecave in the process
		codecaveAddress = VirtualAllocEx(hProcess, 0, 1024, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		OutputDebugStr(L"Allocated code Addr:%x", codecaveAddress);
		if (!codecaveAddress)
			return FALSE;
		dwCodecaveAddress = PtrToUlong(codecaveAddress);

		//Allocate space for save loadlbiray ret
		LPVOID dll_handle_address = VirtualAllocEx(hProcess, 0, sizeof(HMODULE), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		OutputDebugStr(L"Allocated DllHandleAddr Addr:%x", dll_handle_address);
		if (!dll_handle_address)
			return FALSE;
		
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
		WriteProcessMemory(hProcess, codecaveAddress, workspace, workspaceIndex, &bytesRet);

		// Restore page protection
		VirtualProtectEx(hProcess, codecaveAddress, workspaceIndex, oldProtect, &oldProtect);

		// Make sure our changes are written right away
		FlushInstructionCache(hProcess, codecaveAddress, workspaceIndex);

		// Free the workspace memory
		HeapFree(GetProcessHeap(), 0, workspace);

		// Execute the thread now and wait for it to exit, note we execute where the code starts, and not the codecave start
		// (since we wrote strings at the start of the codecave) -- NOTE: void* used for VC6 compatibility instead of UlongToPtr
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((void*)codecaveExecAddr), 0, 0, NULL);
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


	BOOL StartProcess(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, DWORD dwCreateFlag, _Out_ DWORD & Pid,BOOL bInherit, _Out_ PHANDLE phProcess , _Out_ PHANDLE phThread )
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(STARTUPINFO);
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
			Pid = pi.dwProcessId;
			return TRUE;
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
			return pid_list[0];
		else
			return NULL;
	}

	std::vector<DWORD> GetPidsFromExeName(const std::wstring & szExeName,const  DWORD ParentId )
	{
		std::vector<DWORD> ret_pid_list;
		HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		SetResDeleter(handle, [](HANDLE & h){CloseHandle(h); });
		BOOL ret = FALSE;
		PROCESSENTRY32 info;//声明进程信息变量
		info.dwSize = sizeof(PROCESSENTRY32);
		int i = 0;
		wstring strExeNameTerminate = std::wstring(szExeName);
		std::transform(strExeNameTerminate.begin(), strExeNameTerminate.end(), strExeNameTerminate.begin(), tolower);

		if (Process32First(handle, &info))
		{
			wstring strExeFileName = std::wstring(info.szExeFile);
			transform(strExeFileName.begin(), strExeFileName.end(), strExeFileName.begin(), tolower);
			if (strExeFileName == strExeNameTerminate )
			{
				if (ParentId == 0 || ParentId == info.th32ParentProcessID)
					ret_pid_list.push_back(info.th32ProcessID);
			}
			else
			{
				while (Process32Next(handle, &info) != FALSE)
				{
					strExeFileName = std::wstring(info.szExeFile);
					transform(strExeFileName.begin(), strExeFileName.end(), strExeFileName.begin(), tolower);
					if (strExeFileName == strExeNameTerminate)
					{
						if (ParentId == 0 || ParentId == info.th32ParentProcessID)
							ret_pid_list.push_back(info.th32ProcessID);
					}
				}
			}
		}
		return ret_pid_list;
	}

	BOOL IsMutiInstance(const std::wstring & name)
	{
		HANDLE hMutex = ::CreateMutex(NULL, TRUE, name.c_str());
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







	namespace mem_inject
	{
		typedef struct ParamData    //参数结构 
		{
			DWORD Param1;
			DWORD Param2;
			DWORD Param3;
			DWORD Param4;
			DWORD dwCallAddr;
		}ParamData, *Paramp;


		void CurrentProcess_RemoteCall(LPVOID lParam)
		{
			ParamData * lp;
			lp = (ParamData *)lParam;
			DWORD lp1 = (DWORD)lp->Param1;
			DWORD lp2 = (DWORD)lp->Param2;
			DWORD lp3 = (DWORD)lp->Param3;
			DWORD dwAddr = lp->dwCallAddr;
			_asm
			{
				pushad
					push lp3
					push lp2
					push lp1
					call dwAddr
					popad
			}
		}


		typedef struct ParamData2    //参数结构 
		{
			DWORD GetContextAddr;
			HANDLE hThread;
			CONTEXT ctx;
			DWORD nEax;
			DWORD nEbx;
		}ParamData2, *PParamData2;


		void CurrentProcess_InfusionFunc(HANDLE hProcess, LPVOID mFunc, LPVOID Param, DWORD ParamSize)
		{
			DWORD mFuncAddr;//申请函数内存地址         
			LPVOID ParamAddr;//申请参数内存地址 
			HANDLE hThread;    //线程句柄 
			DWORD NumberOfByte; //辅助返回值 
			DWORD local_oldprotect;

			//申请内存 
			////MessageBoxA(NULL, "11111111111", "2222222222222", MB_OK);
			BYTE bbackup[80] = { 0 };

			HMODULE hMod = GetModuleHandleA("ntdll.dll"); //(HMODULE)GetProcessModuleBase("kernel32.dll",GetProcessId(hProcess));
			mFuncAddr = (DWORD)GetProcAddress(hMod, "LdrLoadDll");//GetProcAddressEx(hProcess,hMod,"TerminateProcess");//(LPVOID)GetModuleHandleA("kernel32.dll");//VirtualAllocEx(hProcess,NULL,128,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE); 
			if (mFuncAddr == 0)
			{
				//MessageBoxA(NULL, "11111111111", "2222222222222", MB_OK);
			}
			mFuncAddr = mFuncAddr - 5;
			DWORD local_Proc = (DWORD)VirtualAllocEx(hProcess, NULL, 0X1000, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			VirtualProtectEx((HANDLE)hProcess, (LPVOID)mFuncAddr, 5, PAGE_EXECUTE_READWRITE, &local_oldprotect);

			//*(BYTE*)mFuncAddr = 0xE9;

			//*(DWORD*)(mFuncAddr + 1) = (DWORD)local_Proc - mFuncAddr - 5;

			char local_jmpbuffer[6] = { 0 };
			*(BYTE*)((DWORD)local_jmpbuffer) = 0xE9;
			*(DWORD*)((DWORD)local_jmpbuffer + 1) = (DWORD)local_Proc - mFuncAddr - 5;

			BOOL bWrite = WriteProcessMemory(hProcess, (LPVOID)mFuncAddr, local_jmpbuffer, 5, &NumberOfByte);

			VirtualProtectEx((HANDLE)hProcess, (LPVOID)mFuncAddr, 5, PAGE_EXECUTE_READWRITE, &local_oldprotect);

			ParamAddr = VirtualAllocEx(hProcess, NULL, ParamSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			int memsize = 80;

			bWrite =WriteProcessMemory(hProcess, (LPVOID)local_Proc, mFunc, 0X1000, &NumberOfByte);
			WriteProcessMemory(hProcess, ParamAddr, Param, ParamSize, &NumberOfByte);

			hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)mFuncAddr, ParamAddr, 0, NULL);

			if (!hThread)
			{
				int ierr = GetLastError();
			}

			WaitForSingleObject(hThread, INFINITE); //等待线程结束 
			VirtualFreeEx(hProcess, ParamAddr, 0, MEM_RELEASE);

			//释放远程句柄 
			CloseHandle(hThread);
		}


		BOOL MySetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
		{
			TOKEN_PRIVILEGES tp;
			HANDLE hToken;
			LUID luid;
			if (!OpenProcessToken(GetCurrentProcess(),
				TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				&hToken))
			{
				//_tprintf("OpenProcessToken error: %u/n", GetLastError());
				return FALSE;
			}
			if (!LookupPrivilegeValue(NULL,
				lpszPrivilege,
				&luid))
			{
				//_tprintf("LookupPrivilegeValue error: %u/n", GetLastError() ); 
				return FALSE;
			}
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			if (bEnablePrivilege)
				tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			else
				tp.Privileges[0].Attributes = 0;
			if (!AdjustTokenPrivileges(hToken,
				FALSE,
				&tp,
				sizeof(TOKEN_PRIVILEGES),
				(PTOKEN_PRIVILEGES)NULL,
				(PDWORD)NULL))
			{
				//_tprintf("AdjustTokenPrivileges error: %u/n", GetLastError() ); 
				return FALSE;
			}
			if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
			{
				//_tprintf("The token does not have the specified privilege. /n");
				return FALSE;
			}
			return TRUE;
		}


		int CurrentProcess_GetAlignedSize(int nOrigin, int nAlignment)
		{
			return (nOrigin + nAlignment - 1) / nAlignment * nAlignment;
		}

		//计算整个dll映像文件的尺寸
		int CurrentProcess_CalcTotalImageSize(DWORD filedatabase)
		{
			//VMProtectBegin("dsifeklc");
			int nSize = 0;

			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)filedatabase;  // DOS头

			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)filedatabase + *(DWORD*)((DWORD)m_pDosHeader + 0x3C));

			PIMAGE_SECTION_HEADER m_pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)m_pNTHeader + sizeof(IMAGE_NT_HEADERS));

			if (m_pNTHeader == NULL)
			{
				return 0;
			}

			int nAlign = m_pNTHeader->OptionalHeader.SectionAlignment; //段对齐字节数

			// 计算所有头的尺寸。包括dos, coff, pe头 和 段表的大小
			nSize = CurrentProcess_GetAlignedSize(m_pNTHeader->OptionalHeader.SizeOfHeaders, nAlign);
			// 计算所有节的大小
			for (int i = 0; i < m_pNTHeader->FileHeader.NumberOfSections; ++i)
			{
				//得到该节的大小
				int nCodeSize = m_pSectionHeader[i].Misc.VirtualSize;
				int nLoadSize = m_pSectionHeader[i].SizeOfRawData;
				int nMaxSize = (nLoadSize > nCodeSize) ? (nLoadSize) : (nCodeSize);
				int nSectionSize = CurrentProcess_GetAlignedSize(m_pSectionHeader[i].VirtualAddress + nMaxSize, nAlign);

				if (nSize < nSectionSize)
				{
					nSize = nSectionSize;  //Use the Max;
				}
			}

			//VMProtectEnd();
			return nSize;
		}


		void CurrentProcess_CopyDllDatas(void* pDest, void* pSrc)
		{

			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)pSrc;  // DOS头

			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pSrc + *(DWORD*)((DWORD)m_pDosHeader + 0x3C));

			PIMAGE_SECTION_HEADER m_pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)m_pNTHeader + sizeof(IMAGE_NT_HEADERS));

			int  nHeaderSize = m_pNTHeader->OptionalHeader.SizeOfHeaders;
			int  nSectionSize = m_pNTHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
			int  nMoveSize = nHeaderSize + nSectionSize;
			//复制头和段信息
			memcpy(pDest, pSrc, nMoveSize);

			//复制每个节
			for (int i = 0; i < m_pNTHeader->FileHeader.NumberOfSections; ++i)
			{
				if (m_pSectionHeader[i].VirtualAddress == 0 || m_pSectionHeader[i].SizeOfRawData == 0)
				{
					continue;
				}
				// 定位该节在内存中的位置
				void *pSectionAddress = (void *)((PBYTE)pDest + m_pSectionHeader[i].VirtualAddress);
				// 复制段数据到虚拟内存
				memcpy((void *)pSectionAddress, (void *)((PBYTE)pSrc + m_pSectionHeader[i].PointerToRawData),
					m_pSectionHeader[i].SizeOfRawData);
			}

		}

		void CurrentProcess_DoNewRelocation(void *pNewBase, void *pGameBase)
		{
			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)pNewBase;
			//新的pe头地址
			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pNewBase + (*(DWORD*)((DWORD)m_pDosHeader + 0x3C)));

			PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)((unsigned long)pNewBase
				+ m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

			while ((pLoc->VirtualAddress + pLoc->SizeOfBlock) != 0) //开始扫描重定位表
			{
				WORD *pLocData = (WORD *)((PBYTE)pLoc + sizeof(IMAGE_BASE_RELOCATION));
				//计算本节需要修正的重定位项（地址）的数目
				int nNumberOfReloc = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

				for (int i = 0; i < nNumberOfReloc; i++)
				{
					// 每个WORD由两部分组成。高4位指出了重定位的类型，WINNT.H中的一系列IMAGE_REL_BASED_xxx定义了重定位类型的取值。
					// 低12位是相对于VirtualAddress域的偏移，指出了必须进行重定位的位置。
					if ((DWORD)(pLocData[i] & 0x0000F000) == 0x0000A000)
					{
						// 64位dll重定位，IMAGE_REL_BASED_DIR64
						// 对于IA-64的可执行文件，重定位似乎总是IMAGE_REL_BASED_DIR64类型的。
#ifdef _WIN64
						ULONGLONG* pAddress = (ULONGLONG *)((PBYTE)pNewBase + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
						ULONGLONG ullDelta = (ULONGLONG)pNewBase - m_pNTHeader->OptionalHeader.ImageBase;
						*pAddress += ullDelta;
#endif
					}
					else if ((DWORD)(pLocData[i] & 0x0000F000) == 0x00003000) //这是一个需要修正的地址
					{
						// 32位dll重定位，IMAGE_REL_BASED_HIGHLOW
						// 对于x86的可执行文件，所有的基址重定位都是IMAGE_REL_BASED_HIGHLOW类型的。
#ifndef _WIN64
						DWORD* pAddress = (DWORD *)((PBYTE)pNewBase + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
						DWORD dwDelta = (DWORD)pGameBase - m_pNTHeader->OptionalHeader.ImageBase;
						*pAddress += dwDelta;
						//OutputDebugStringA("[33333] 已有地址修正");
#endif
					}
				}
				//转移到下一个节进行处理
				pLoc = (PIMAGE_BASE_RELOCATION)((PBYTE)pLoc + pLoc->SizeOfBlock);
			}
		}



		//两个参数
		BOOL __stdcall CurrentProcess_FillRavAddress(LPVOID param)
		{
			// 引入表实际上是一个 IMAGE_IMPORT_DESCRIPTOR 结构数组，全部是0表示结束
			// 数组定义如下：
			// 
			// DWORD   OriginalFirstThunk;         // 0表示结束，否则指向未绑定的IAT结构数组
			// DWORD   TimeDateStamp; 
			// DWORD   ForwarderChain;             // -1 if no forwarders
			// DWORD   Name;                       // 给出dll的名字
			// DWORD   FirstThunk;                 // 指向IAT结构数组的地址(绑定后，这些IAT里面就是实际的函数地址)

			void *pImageBase = (void*)param;

			//新的dos头
			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)param;

			DWORD NumOfBytes = 0;


			//新的pe头地址
			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)param + (*(DWORD*)((DWORD)m_pDosHeader + 0x3C)));

			//PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)(*(DWORD*)((DWORD)param+4));

			unsigned long nOffset = m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

			if (nOffset == 0)
			{
				return TRUE; //No Import Table
			}

			PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)pImageBase + nOffset);

			while (pID->Characteristics != 0)
			{
				PIMAGE_THUNK_DATA pRealIAT = (PIMAGE_THUNK_DATA)((PBYTE)pImageBase + pID->FirstThunk);
				PIMAGE_THUNK_DATA pOriginalIAT = (PIMAGE_THUNK_DATA)((PBYTE)pImageBase + pID->OriginalFirstThunk);

				//获取dll的名字
#define NAME_BUF_SIZE 256

				char szBuf[NAME_BUF_SIZE] = ""; //dll name;
				BYTE* pName = (BYTE*)((PBYTE)pImageBase + pID->Name);
				int i = 0;

				for (i = 0; i<NAME_BUF_SIZE; i++)
				{
					if (pName[i] == 0)
					{
						break;
					}
					szBuf[i] = pName[i];
				}
				if (i >= NAME_BUF_SIZE)
				{
					return FALSE;  // bad dll name
				}
				else
				{
					szBuf[i] = 0;
				}

				HMODULE hDll = (HMODULE)GetModuleHandleA(szBuf);

				if (hDll == NULL)
				{
					//MessageBoxA(NULL,szBuf,"dll没有加载",MB_OK);
					return FALSE; //NOT FOUND DLL
				}



				//获取DLL中每个导出函数的地址，填入IAT
				//每个IAT结构是 ：
				// union { PBYTE  ForwarderString;
				//   PDWORD Function;
				//   DWORD Ordinal;
				//   PIMAGE_IMPORT_BY_NAME  AddressOfData;
				// } u1;
				// 长度是一个DWORD ，正好容纳一个地址。
				for (i = 0;; i++)
				{
					if (pOriginalIAT[i].u1.Function == 0)
					{

						break;
					}

					FARPROC lpFunction = NULL;

					if (pOriginalIAT[i].u1.Ordinal & IMAGE_ORDINAL_FLAG) //这里的值给出的是导出序号
					{
						lpFunction = (FARPROC)GetProcAddress(hDll, (LPCSTR)(pOriginalIAT[i].u1.Ordinal & 0x0000FFFF));
						////MessageBoxA(NULL,(char*)lpFunction,"序号",MB_OK);
					}
					else //按照名字导入
					{
						//获取此IAT项所描述的函数名称
						PIMAGE_IMPORT_BY_NAME pByName = (PIMAGE_IMPORT_BY_NAME)((DWORD)pImageBase + (DWORD)(pOriginalIAT[i].u1.AddressOfData));
						lpFunction = (FARPROC)GetProcAddress(hDll, (char *)pByName->Name);

					}
					if (lpFunction != NULL)   //找到了！
					{
						pRealIAT[i].u1.Function = (DWORD)lpFunction;
					}
					else
					{
						return FALSE;
					}
				}

				//move to next 
				pID = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)pID + sizeof(IMAGE_IMPORT_DESCRIPTOR));
			}


			return TRUE;
		}



#include <stdio.h>

		DWORD MemLoadLibraryA(const char *FilePath, HANDLE hTargetHandle)
		{

			//	void *pMemoryAddress
			//HANDLE hTargetHandle 
			//char *pszDllData
			////MessageBoxA(NULL, "1111111", "2222222222", MB_OK);
			//读取dll文件
			FILE *pFile = NULL;
			fopen_s(&pFile,FilePath, "rb");
			if (!pFile)return 0;
			fseek(pFile, 0, SEEK_END);

			int filesize = ftell(pFile);

			fseek(pFile, 0, SEEK_SET);

			if (filesize<1000 || filesize == 0xFFFFFFFF)
			{
				fclose(pFile);
				return 0;
			}

			LPVOID pszDllData = VirtualAllocEx((HANDLE)0xFFFFFFFF, NULL, filesize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//new char[filesize];

			OVERLAPPED olp;

			memset(&olp, 0, sizeof(OVERLAPPED));

			olp.Offset = 0;

			fread(pszDllData, 1, filesize, pFile);

			fclose(pFile);


			//进行加载
			DWORD newsize = CurrentProcess_CalcTotalImageSize((DWORD)pszDllData);

			void *pMemoryAddress = VirtualAllocEx((HANDLE)0xFFFFFFFF, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			CurrentProcess_CopyDllDatas(pMemoryAddress, pszDllData);


			//新的dos头
			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)pMemoryAddress;

			//新的pe头地址
			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pMemoryAddress + (*(DWORD*)((DWORD)m_pDosHeader + 0x3C)));
			//新的节表地址
			PIMAGE_SECTION_HEADER m_pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)m_pNTHeader + sizeof(IMAGE_NT_HEADERS));

			//DWORD ofset = (DWORD)&*(BYTE*)((DWORD)m_pDosHeader+0x3C) - (DWORD)m_pDosHeader;


			LPVOID lpGameMemory = VirtualAllocEx(hTargetHandle, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (!lpGameMemory)
			{
				DWORD dwLastError = ::GetLastError();
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);

				return 0;
			}




			if (m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > 0
				&& m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
			{
				//DoRelocation(pMemoryAddress);
				//DoGameRelocation(hTargetHandle,pMemoryAddress,lpGameMemory);
				////MessageBoxA(NULL,"有重定位信息","info",MB_OK);
				CurrentProcess_DoNewRelocation(pMemoryAddress, lpGameMemory);
			}

			//本地处理
			CurrentProcess_FillRavAddress(pMemoryAddress);

			BOOL bIn = WriteProcessMemory(hTargetHandle, lpGameMemory, pMemoryAddress, newsize, NULL);

			if (!bIn)
			{

				VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);
				return 0;
			}

			//OutLog("已修复输入表",(DWORD)lpGameMemory);

			ParamData CallParam;
			CallParam.dwCallAddr = (DWORD)(lpGameMemory)+m_pNTHeader->OptionalHeader.AddressOfEntryPoint;
			CallParam.Param1 = (DWORD)(lpGameMemory);
			CallParam.Param2 = 1;
			CallParam.Param3 = 0;


			//OutLog("入口",CallParam.dwCallAddr);

			CurrentProcess_InfusionFunc(hTargetHandle, CurrentProcess_RemoteCall, &CallParam, sizeof(CallParam));

			VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
			VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);

			//OutLog("注入成功",(DWORD)lpGameMemory);

			return 0;

		}


		DWORD MemLoadLibraryA(HINSTANCE hInstance, DWORD dwResourceId, HANDLE hTargetHandle)
		{

			//	void *pMemoryAddress
			//HANDLE hTargetHandle 
			//char *pszDllData
			////MessageBoxA(NULL, "1111111", "2222222222", MB_OK);
			//读取dll文件

			DWORD NumOfBytes = 0;
			HRSRC hRsrc = FindResourceA(hInstance, MAKEINTRESOURCEA(dwResourceId), "SVCHOST_DATA");

			DWORD filesize = SizeofResource(hInstance, hRsrc);

			HGLOBAL hGlobal = LoadResource(hInstance, hRsrc);

			LPVOID pBuffer = LockResource(hGlobal);

			if (filesize<1000 || filesize == 0xFFFFFFFF)
			{
				return 0;
			}

			LPVOID pszDllData = VirtualAllocEx((HANDLE)0xFFFFFFFF, NULL, filesize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);//new char[filesize];

			memcpy(pszDllData, pBuffer, filesize);
			UnlockResource(hGlobal);


			//进行加载
			DWORD newsize = CurrentProcess_CalcTotalImageSize((DWORD)pszDllData);

			void *pMemoryAddress = VirtualAllocEx((HANDLE)0xFFFFFFFF, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			CurrentProcess_CopyDllDatas(pMemoryAddress, pszDllData);


			//新的dos头
			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)pMemoryAddress;

			//新的pe头地址
			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pMemoryAddress + (*(DWORD*)((DWORD)m_pDosHeader + 0x3C)));
			//新的节表地址
			PIMAGE_SECTION_HEADER m_pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)m_pNTHeader + sizeof(IMAGE_NT_HEADERS));

			//DWORD ofset = (DWORD)&*(BYTE*)((DWORD)m_pDosHeader+0x3C) - (DWORD)m_pDosHeader;

			LPVOID lpGameMemory00 = VirtualAllocEx(hTargetHandle, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if (!lpGameMemory00)
			{
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);
				return 0;
			}


			WriteProcessMemory(hTargetHandle, lpGameMemory00, pMemoryAddress, newsize, NULL);


			LPVOID lpGameMemory = VirtualAllocEx(hTargetHandle, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (!lpGameMemory)
			{

				VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);

				return 0;
			}




			if (m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > 0
				&& m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
			{
				//DoRelocation(pMemoryAddress);
				//DoGameRelocation(hTargetHandle,pMemoryAddress,lpGameMemory);
				////MessageBoxA(NULL,"有重定位信息","info",MB_OK);
				CurrentProcess_DoNewRelocation(pMemoryAddress, lpGameMemory);
			}

			//本地处理
			CurrentProcess_FillRavAddress(pMemoryAddress);

			BOOL bIn = WriteProcessMemory(hTargetHandle, lpGameMemory, pMemoryAddress, newsize, NULL);

			if (!bIn)
			{

				VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);
				return 0;
			}

			//OutLog("已修复输入表",(DWORD)lpGameMemory);

			ParamData CallParam;
			CallParam.dwCallAddr = (DWORD)(lpGameMemory)+m_pNTHeader->OptionalHeader.AddressOfEntryPoint;
			CallParam.Param1 = (DWORD)(lpGameMemory);
			CallParam.Param2 = 1;
			CallParam.Param3 = (DWORD)lpGameMemory00;


			//OutLog("入口",CallParam.dwCallAddr);

			CurrentProcess_InfusionFunc(hTargetHandle, CurrentProcess_RemoteCall, &CallParam, sizeof(CallParam));

			VirtualFreeEx((HANDLE)0xFFFFFFFF, pszDllData, 0, MEM_RELEASE);
			VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);

			//OutLog("注入成功",(DWORD)lpGameMemory);

			return 0;

		}


		DWORD MemLoadLibrary2A(DWORD DllMemory, HANDLE hTargetHandle)
		{
			DWORD NumOfBytes = 0;
			LPVOID pszDllData = (LPVOID)DllMemory;


			//进行加载
			DWORD newsize = CurrentProcess_CalcTotalImageSize((DWORD)pszDllData);

			void *pMemoryAddress = VirtualAllocEx((HANDLE)0xFFFFFFFF, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			CurrentProcess_CopyDllDatas(pMemoryAddress, pszDllData);


			//新的dos头
			PIMAGE_DOS_HEADER m_pDosHeader = (PIMAGE_DOS_HEADER)pMemoryAddress;

			//新的pe头地址
			PIMAGE_NT_HEADERS m_pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pMemoryAddress + (*(DWORD*)((DWORD)m_pDosHeader + 0x3C)));
			//新的节表地址
			PIMAGE_SECTION_HEADER m_pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)m_pNTHeader + sizeof(IMAGE_NT_HEADERS));

			//DWORD ofset = (DWORD)&*(BYTE*)((DWORD)m_pDosHeader+0x3C) - (DWORD)m_pDosHeader;

			LPVOID lpGameMemory = VirtualAllocEx(hTargetHandle, NULL, newsize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);

			if (!lpGameMemory)
			{

				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);

				return 0;
			}




			if (m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > 0
				&& m_pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
			{
				//DoRelocation(pMemoryAddress);
				//DoGameRelocation(hTargetHandle,pMemoryAddress,lpGameMemory);
				////MessageBoxA(NULL,"有重定位信息","info",MB_OK);
				CurrentProcess_DoNewRelocation(pMemoryAddress, lpGameMemory);
			}

			//本地处理
			CurrentProcess_FillRavAddress(pMemoryAddress);

			BOOL bIn = WriteProcessMemory(hTargetHandle, lpGameMemory, pMemoryAddress, newsize, NULL);

			if (!bIn)
			{


				VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);
				return 0;
			}

			//OutLog("已修复输入表",(DWORD)lpGameMemory);

			ParamData CallParam;
			CallParam.dwCallAddr = (DWORD)(lpGameMemory)+m_pNTHeader->OptionalHeader.AddressOfEntryPoint;
			CallParam.Param1 = (DWORD)(lpGameMemory);
			CallParam.Param2 = 1;
			CallParam.Param3 = (DWORD)0;


			//OutLog("入口",CallParam.dwCallAddr);

			CurrentProcess_InfusionFunc(hTargetHandle, CurrentProcess_RemoteCall, &CallParam, sizeof(CallParam));


			VirtualFreeEx((HANDLE)0xFFFFFFFF, pMemoryAddress, 0, MEM_RELEASE);


			return 0;
		}
	}

	// Loadlibrary template
	typedef HMODULE(__stdcall* pLoadLibraryA)(LPCSTR);
	// getprocaddress template
	typedef FARPROC(__stdcall* pGetProcAddress)(HMODULE, LPCSTR);

	// dll entrypoint template
	typedef INT(__stdcall* dllmain)(HMODULE, DWORD32, LPVOID);

	// parameters for "libraryloader()" 
	struct loaderdata {
		LPVOID ImageBase; //base address of dll 
		PIMAGE_NT_HEADERS NtHeaders;
		PIMAGE_BASE_RELOCATION BaseReloc;
		PIMAGE_IMPORT_DESCRIPTOR ImportDir;

		pLoadLibraryA fnLoadLibraryA;
		pGetProcAddress fnGetProcAddress;
	};


	// code responsible for loading the dll in remote process:
	//(this will be copied to and executed in the target process)
	DWORD __stdcall LibraryLoader(LPVOID Memory)
	{
		loaderdata* LoaderParams = (loaderdata*)Memory;

		PIMAGE_BASE_RELOCATION ImageRelocation = LoaderParams->BaseReloc;

		DWORD delta = (DWORD)((LPBYTE)LoaderParams->ImageBase - LoaderParams->NtHeaders->OptionalHeader.ImageBase); // Calculate the delta

		while (ImageRelocation->VirtualAddress) {
			if (ImageRelocation->SizeOfBlock >= sizeof(PIMAGE_BASE_RELOCATION))
			{
				int count = (ImageRelocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				PWORD list = (PWORD)(ImageRelocation + 1);

				for (int i = 0; i < count; i++)
				{
					if (list[i])
					{
						PDWORD ptr = (PDWORD)((LPBYTE)LoaderParams->ImageBase + (ImageRelocation->VirtualAddress + (list[i] & 0xFFF)));
						*ptr += delta;
					}
				}
				ImageRelocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)ImageRelocation + ImageRelocation->SizeOfBlock);
			}
		}

		PIMAGE_IMPORT_DESCRIPTOR ImportDesc = LoaderParams->ImportDir;
		while (ImportDesc->Characteristics) {
			PIMAGE_THUNK_DATA OrigFirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + ImportDesc->OriginalFirstThunk);
			PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + ImportDesc->FirstThunk);

			HMODULE hModule = LoaderParams->fnLoadLibraryA((LPCSTR)LoaderParams->ImageBase + ImportDesc->Name);

			if (!hModule)
				return FALSE;

			while (OrigFirstThunk->u1.AddressOfData)
			{
				if (OrigFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
				{
					// Import by ordinal
					DWORD Function = (DWORD)LoaderParams->fnGetProcAddress(hModule,
						(LPCSTR)(OrigFirstThunk->u1.Ordinal & 0xFFFF));

					if (!Function)
						return FALSE;

					FirstThunk->u1.Function = Function;
				}
				else
				{
					// Import by name
					PIMAGE_IMPORT_BY_NAME pIBN = (PIMAGE_IMPORT_BY_NAME)((LPBYTE)LoaderParams->ImageBase + OrigFirstThunk->u1.AddressOfData);
					DWORD Function = (DWORD)LoaderParams->fnGetProcAddress(hModule, (LPCSTR)pIBN->Name);
					if (!Function)
						return FALSE;

					FirstThunk->u1.Function = Function;
				}
				OrigFirstThunk++;
				FirstThunk++;
			}
			ImportDesc++;
		}

		// if the dll has an entrypoint: 
		if (LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint)
		{
			dllmain EntryPoint = (dllmain)((LPBYTE)LoaderParams->ImageBase + LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint);
			return EntryPoint((HMODULE)LoaderParams->ImageBase, DLL_PROCESS_ATTACH, NULL); // Call the entry point 
		}

		return true;
	}

	// this is used to calculate the size of libraryloader function
	DWORD WINAPI stub()
	{
		return 0;
	}
	BOOL MemInjectDll(HANDLE hProcess, const std::wstring & dll_path)
	{
		HANDLE hDll = CreateFile(dll_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if (hDll == INVALID_HANDLE_VALUE || hDll == NULL)
			return FALSE;
		DWORD FileSize = GetFileSize(hDll, NULL);
		LPVOID FileBuffer = VirtualAlloc(NULL, FileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		DWORD lpNumberOfBytesRead = 0;
		ReadFile(hDll, FileBuffer, FileSize, &lpNumberOfBytesRead, NULL);
		CloseHandle(hDll);


		// Target Dll's headers:
		PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
		PIMAGE_NT_HEADERS Ntheaders = (PIMAGE_NT_HEADERS)((LPBYTE)FileBuffer + DosHeader->e_lfanew);



		// Allocate memory for the dll in target process: 
		LPVOID Executablelmage = VirtualAllocEx(hProcess, 0, Ntheaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		// copy headers to target process:
		WriteProcessMemory(hProcess, Executablelmage, FileBuffer, Ntheaders->OptionalHeader.SizeOfHeaders, NULL);

		PIMAGE_SECTION_HEADER SectHeader = (PIMAGE_SECTION_HEADER)(Ntheaders + 1);

		// copy sections of the dll to target process:
		for (int i = 0; i < Ntheaders->FileHeader.NumberOfSections; i++)
		{
			WriteProcessMemory(
				hProcess,
				(PVOID)((LPBYTE)Executablelmage + SectHeader[i].VirtualAddress),
				(PVOID)((LPBYTE)FileBuffer + SectHeader[i].PointerToRawData),
				SectHeader[i].SizeOfRawData,
				NULL
				);
		}

		// initialize the parameters for LibraryLoader():
		loaderdata LoaderParams;
		LoaderParams.ImageBase = Executablelmage;
		LoaderParams.NtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)Executablelmage + DosHeader->e_lfanew);

		LoaderParams.BaseReloc = (PIMAGE_BASE_RELOCATION)((LPBYTE)Executablelmage +
			Ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		LoaderParams.ImportDir = (PIMAGE_IMPORT_DESCRIPTOR)((LPBYTE)Executablelmage +
			Ntheaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

		LoaderParams.fnLoadLibraryA = LoadLibraryA;
		LoaderParams.fnGetProcAddress = GetProcAddress;

		//TODO: Does not work with debug build as the incrmental linker creates a lot of trampoline functions hence the pointer to the 
		//TODO: function points to the trampoline and not the'actual' function. This causes the wrong code and size to be copied!
		// Allocate Memory for the loader code:
		DWORD LoaderCodeSize = (DWORD)stub - (DWORD)LibraryLoader;
		DWORD LoaderTotalSize = LoaderCodeSize + sizeof(loaderdata);
		LPVOID LoaderMemory = VirtualAllocEx(hProcess, NULL, LoaderTotalSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		// Write the loader parameters to the process
		WriteProcessMemory(hProcess, LoaderMemory, &LoaderParams, sizeof(loaderdata), 0);

		// write the loader code to target process
		WriteProcessMemory(hProcess, (PVOID)((loaderdata*)LoaderMemory + 1), LibraryLoader, LoaderCodeSize, NULL);

		// create remote thread to execute the loader code:
		//	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)((loaderdata*)LoaderMemory + 1), LoaderMemory, 0, NULL);
		HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((loaderdata*)LoaderMemory + 1), LoaderMemory, 0, NULL);

		// 		std::cout << "Address of Loader: " << std::hex << LoaderMemory << std::endl;
		// 		std::cout << "Address of Image: " << std::hex << Executablelmage << std::endl;
		// 		std::cout << "Press any key, to exit!" << std::endl;

		// Wait for the loader to finish executing
		WaitForSingleObject(hThread, INFINITE);
		VirtualFreeEx(hProcess, LoaderMemory, 0, MEM_RELEASE);
		return TRUE;
	}



	}


