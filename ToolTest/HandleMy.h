#pragma once
#include <windows.h>
#include "NonCopyable.h"
//auto CloseHandle 
class CAutoCloseHandle : public CNonCopyable
{
public:
	CAutoCloseHandle();
	CAutoCloseHandle(HANDLE handle) : m_Handle(handle) {}
	~CAutoCloseHandle();

	CAutoCloseHandle(const CAutoCloseHandle &) = delete;
	CAutoCloseHandle & operator=(const CAutoCloseHandle &) = delete;

	BOOL Attach(HANDLE hHandle)
	{
		if (hHandle != INVALID_HANDLE_VALUE && hHandle != NULL)
		{
			m_Handle = hHandle;
			return TRUE;
		}
		else
			return FALSE;
	}
	BOOL IsValid() const {
		return (m_Handle != INVALID_HANDLE_VALUE && m_Handle != NULL);
	}
	operator HANDLE()  {
		return m_Handle;
	}

	operator PHANDLE() { return &m_Handle; }
private:
	HANDLE m_Handle;
};

