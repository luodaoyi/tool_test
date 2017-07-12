#pragma once
#include <windows.h>
#include <functional>

namespace system_tool
{
	BOOL IsValidHandle(const HANDLE & h);
	BOOL LockMutex(HANDLE hHandle, DWORD  dwMilliseconds);
	VOID UnLockMutex(HANDLE hHandle);
	BOOL DoActionTimeOut(DWORD dwMilliseconds,std::function<BOOL(void)> fn);
}