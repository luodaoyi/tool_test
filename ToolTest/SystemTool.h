#pragma once
#include <windows.h>
#include <functional>

namespace system_tool
{
	enum ACTION_RET
	{
		RET_WAIT = FALSE,
		RET_OK = TRUE,
		RET_BREAK = 2,
	};

	BOOL IsValidHandle(const HANDLE & h);
	BOOL LockMutex(HANDLE hHandle, DWORD  dwMilliseconds);
	VOID UnLockMutex(HANDLE hHandle);
	BOOL DoActionTimeOut(DWORD dwMilliseconds, std::function<ACTION_RET(void)> fn);
}