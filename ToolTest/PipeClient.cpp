#include "stdafx.h"
#include "PipeClient.h"
#include "DebugOutput.h"

CPipeClient::CPipeClient(LPCTSTR pipe_name)
{
	wcscpy_s(m_name, pipe_name);
}


CPipeClient::~CPipeClient()
{
	Close();
}

void CPipeClient::Close()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}
}

BOOL CPipeClient::RawSendBuffer(unsigned char* buffer, size_t size, BOOL with_connect )
{
	DWORD dwWritten = 0;
	if (!WriteFile(m_handle, buffer, size, &dwWritten, NULL))
	{
		if (with_connect)
		{
			if (Connect())
				return RawSendBuffer(buffer, size, FALSE);
			else
				return FALSE;
		}
		else
			return  FALSE;
	}
	else
		return TRUE;
}

BOOL CPipeClient::Connect()
{
	if (m_handle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_handle);
		m_handle = INVALID_HANDLE_VALUE;
	}

	HANDLE hPipe = CreateFile(m_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPipe != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = PIPE_READMODE_MESSAGE;
		if (SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
		{
			m_handle = hPipe;
			return TRUE;
		}
		else
			return FALSE;
	}
	else
	{
		DWORD last_error = ::GetLastError();
		if (last_error != ERROR_PIPE_BUSY)
			return FALSE;
		if (m_conn_time_out > 0)
		{
			if (!WaitNamedPipe(m_name, m_conn_time_out))
			{
				if (GetLastError() == ERROR_SEM_TIMEOUT)
					return FALSE;//Á¬½Ó³¬Ê±
				else
					return FALSE;
			}
			else
			{
				hPipe = CreateFile(m_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hPipe == INVALID_HANDLE_VALUE)
					return FALSE;
				else
				{
					
					DWORD dwMode = PIPE_READMODE_MESSAGE;
					if (SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
					{
						m_handle = hPipe;
						return TRUE;
					}
					else
						return FALSE;
				}
			}
		}
		else
			return FALSE;
	}
}

BOOL CPipeClient::SendBuff(unsigned char* buff, size_t size)
{
	if (m_handle == INVALID_HANDLE_VALUE)
	{
		if (Connect())
			return RawSendBuffer(buff, size, FALSE);
		else
			return FALSE;
	}
	else
		return RawSendBuffer(buff, size, TRUE);
}

BOOL CPipeClient::RecvBuff(unsigned char * buff, size_t size, size_t * out_size)
{
	DWORD dwReaded = 0;
	if (::ReadFile(m_handle, buff, size, &dwReaded, NULL))
	{
		*out_size = dwReaded;
		return TRUE;

	}
	else
	{
		DWORD last_error = ::GetLastError();
		OutputDebugStr(L"pipe client read failed:%d", last_error);
		return FALSE;
	}

}
BOOL CPipeClient::SendAndRecv(unsigned char * buff, size_t send_size, unsigned char * buff_recv, size_t recv_size, size_t * pout_size)
{
	if (SendBuff(buff, send_size) && RecvBuff(buff_recv, recv_size, pout_size))
		return TRUE;
	else
		return FALSE;
}