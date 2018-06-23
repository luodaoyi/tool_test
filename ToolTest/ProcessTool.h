#pragma once

#include "commom_include.h"
#include <TlHelp32.h>
#include <functional>
namespace process_tool
{ 
	enum {error_process_exit_code = 100};

	BOOL IsProcessRunning(DWORD dwPid);
	BOOL InjectDll_CallFunc(DWORD dwPid,const std::wstring & dll_path,const std::wstring & dll_func_name,HMODULE  * injected_dll_module = NULL);
	BOOL FreeRemoteDll(DWORD dwPid, HMODULE hDll);
	BOOL StartProcess(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, DWORD dwCreateFlag, _Out_ DWORD * Pid = NULL ,BOOL bInherit = FALSE,_Out_ PHANDLE phProcess = NULL, _Out_ PHANDLE phThread = NULL);
	BOOL StartProcessAndInjectDllAndCallDllFunc(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, const std::wstring & dll_path, const std::wstring & dll_func_name, _Out_ DWORD & Pid, _Out_ PHANDLE phProcess = NULL, _Out_ PHANDLE phThread = NULL);
	DWORD GetPidFromExeName(const std::wstring & szExeName, const DWORD ParentPid = 0);
	std::vector<PROCESSENTRY32>  GetPidsFromExeName(const std::wstring & szExeName, const DWORD dwParendId = 0);
	BOOL IsMutiInstance(const std::wstring & name);//检测多开
	BOOL KillProcess(DWORD dwPid);
	BOOL InjectDll(HANDLE hProcess, const std::wstring & dll_path, const std::wstring & dll_func_name, HMODULE * hDllModule = NULL);
	HWND FindProcessWnd(const DWORD dwPid, LPWSTR class_name, LPWSTR caption_name);//得到进程窗口
	HWND WaitForProcessWindow(const DWORD dwPid, DWORD milli_seconds, LPWSTR class_name, LPWSTR caption_name);//等待进程窗口显示
	DWORD GetWindowThreadID(HWND hWnd);
	DWORD GetWindowProcessID(HWND hWnd);
	DWORD GetProcessCount(const std::wstring & exe_name);
	std::vector<PROCESSENTRY32> GetPidsByCondition(std::function<bool(const PROCESSENTRY32 & process_info)> fnCheck);
	DWORD StartProcessAndGetExitCode(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path = NULL);
	BOOL InjectDllNormal(HANDLE hProcess, const std::wstring & lib_name);
	uintptr_t FindRemoteDLL(DWORD pid, std::wstring libName);
	BOOL MemInjectDll(HANDLE hProcess, const std::wstring & dll_path);


	BOOL GetProcessExePath(DWORD dwPid, std::wstring & full_path);
	void SetThreadName(DWORD dwThreadID, const char* threadName);
}
