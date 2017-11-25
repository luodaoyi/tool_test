#include "stdafx.h"
#include "SystemTool.h"


namespace system_tool
{
	BOOL IsValidHandle(const HANDLE & h)
	{
		if (h == NULL || h == INVALID_HANDLE_VALUE)
			return FALSE;
		else
			return TRUE;
	}

	BOOL LockMutex(HANDLE hHandle, DWORD dwMilliseconds)
	{
		DWORD dwRet = ::WaitForSingleObject(hHandle, dwMilliseconds);
		if (dwRet == WAIT_ABANDONED || dwRet == WAIT_OBJECT_0)
			return TRUE;
		else
			return FALSE;
	}
	VOID UnLockMutex(HANDLE hHandle)
	{
		::ReleaseMutex(hHandle);
	}

	BOOL DoActionTimeOut(DWORD dwMilliseconds, std::function<ACTION_RET(void)> fn)
	{
		DWORD begin_time = GetTickCount();
		while (true)
		{
			Sleep(500);
			if (GetTickCount() - begin_time > dwMilliseconds)
				return FALSE;	
			auto ret = fn();
			if (ret == RET_OK)
				return TRUE;
			else if (ret == RET_WAIT)
				continue;
			else if (ret == RET_BREAK)
				return FALSE;
		}
		return FALSE;
	}
}