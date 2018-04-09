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


	class GuardCriticalSection
	{
	public:
		GuardCriticalSection(CRITICAL_SECTION & cs) : m_cs(cs)
		{
			::EnterCriticalSection(&m_cs);
		}
		~GuardCriticalSection()
		{
			LeaveCriticalSection(&m_cs);
		}
	private:
		GuardCriticalSection(const GuardCriticalSection  &) = delete;
		GuardCriticalSection & operator=(const GuardCriticalSection &) = delete;
		CRITICAL_SECTION& m_cs;
	};
}