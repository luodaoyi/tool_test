#pragma once

#include "commom_include.h"

namespace process_tool
{ 
	BOOL IsProcessRunning(DWORD dwPid);
	BOOL InjectDll_CallFunc(DWORD dwPid,const std::wstring & dll_path,const std::wstring & dll_func_name,HMODULE  * injected_dll_module = NULL);
	BOOL FreeRemoteDll(DWORD dwPid, HMODULE hDll);
	BOOL StartProcess(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, DWORD dwCreateFlag, _Out_ DWORD * Pid = NULL ,BOOL bInherit = FALSE,_Out_ PHANDLE phProcess = NULL, _Out_ PHANDLE phThread = NULL);
	BOOL StartProcessAndInjectDllAndCallDllFunc(LPCWSTR app_name, LPCWSTR cmd_line, LPCWSTR cur_path, const std::wstring & dll_path, const std::wstring & dll_func_name, _Out_ DWORD & Pid, _Out_ PHANDLE phProcess = NULL, _Out_ PHANDLE phThread = NULL);
	DWORD GetPidFromExeName(const std::wstring & szExeName, const DWORD ParentPid = 0);
	std::vector<DWORD> GetPidsFromExeName(const std::wstring & szExeName, const DWORD dwParendId = 0);
	BOOL IsMutiInstance(const std::wstring & name);//���࿪
	BOOL KillProcess(DWORD dwPid);
	BOOL InjectDll(HANDLE hProcess, const std::wstring & dll_path, const std::wstring & dll_func_name, HMODULE * hDllModule = NULL);
	HWND FindProcessWnd(const DWORD dwPid, LPWSTR class_name, LPWSTR caption_name);//�õ����̴���
	HWND WaitForProcessWindow(const DWORD dwPid, DWORD milli_seconds, LPWSTR class_name, LPWSTR caption_name);//�ȴ����̴�����ʾ
	DWORD GetWindowThreadID(HWND hWnd);
	DWORD GetWindowProcessID(HWND hWnd);
	DWORD GetProcessCount(const std::wstring & exe_name);


	namespace mem_inject
	{
		DWORD MemLoadLibraryA(const char *FilePath, HANDLE hTargetHandle);
	}


	BOOL MemInjectDll(HANDLE hProcess, const std::wstring & dll_path);


	BOOL GetProcessExePath(DWORD dwPid, std::wstring & full_path);
}
