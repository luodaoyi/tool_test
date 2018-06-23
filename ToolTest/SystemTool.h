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


	class CriticalSection
	{
	public:
		CriticalSection()
		{
			::InitializeCriticalSection(&m_cs);
		}
		~CriticalSection()
		{
			::DeleteCriticalSection(&m_cs);
		}
		void Lock()
		{
			::EnterCriticalSection(&m_cs);
		}
		void UnLock()
		{
			::LeaveCriticalSection(&m_cs);
		}
		bool TryLock()
		{
			return ::TryEnterCriticalSection(&m_cs) == TRUE;
		}
	private:
		::CRITICAL_SECTION m_cs;
	};

	class GuardCriticalSection
	{
	public:
		GuardCriticalSection(CriticalSection & cs) : m_cs(cs)
		{
			m_cs.Lock();
		}
		~GuardCriticalSection()
		{
			m_cs.UnLock();
		}
	private:
		GuardCriticalSection(const GuardCriticalSection  &) = delete;
		GuardCriticalSection & operator=(const GuardCriticalSection &) = delete;
		CriticalSection& m_cs;
	};
}
#define CONCAT2(a, b) a##b
#define CONCAT(a, b) CONCAT2(a, b)
#define SCOPED_LOCK(cs) system_tool::GuardCriticalSection CONCAT(scopedlock, __LINE__)(cs);