#include "stdafx.h"
#include "HandleMy.h"


CAutoCloseHandle::CAutoCloseHandle()
{
	m_Handle = INVALID_HANDLE_VALUE;
}


CAutoCloseHandle::~CAutoCloseHandle()
{
	if (IsValid())
	{
		::CloseHandle(m_Handle);
		m_Handle = INVALID_HANDLE_VALUE;
	}
}
