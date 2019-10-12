#include "stdafx.h"
#include "SimpleLog.h"
#include "DebugOutput.h"
#include <ctime>
#include  <iostream>
#include <iomanip>
#include <sstream>

CSimpleLog::CSimpleLog()
{
	m_file_handle = INVALID_HANDLE_VALUE;
}

CSimpleLog::~CSimpleLog()
{
	::CloseHandle(m_file_handle);
	m_file_handle =  INVALID_HANDLE_VALUE;
}

std::wstring CSimpleLog::GetFileLineHead()
{
	tm tm = { 0 };
	auto t = time(NULL);
	localtime_s(&tm, &t);
	std::wostringstream oss;
	oss << std::put_time(&tm, L"%F %X");
	return std::wstring(L"[") + oss.str() + L"][" + std::to_wstring(::GetCurrentThreadId()) + L"]:";
}
std::wstring CSimpleLog::GetPipeLineHead()
{
	auto t = std::time(nullptr);
	tm cur_tm;
	localtime_s(&cur_tm,&t);
	std::wostringstream oss;
	oss << std::put_time(&cur_tm, L"%X");
	return L"[" + oss.str() + L"]";
}
void CSimpleLog::Log(const std::wstring & str)
{
	std::wstring dest_str = GetFileLineHead() +  str;
	std::lock_guard<std::mutex> l(m_mutex);
	WriteFile(dest_str.c_str(), dest_str.length() * sizeof(wchar_t));
	if (m_pipe_switch)
		SendPipe(GetPipeLineHead() + str);
}

void CSimpleLog::WriteFile(LPCVOID  pData, size_t size)
{
	if (m_file_handle == INVALID_HANDLE_VALUE)
	{
		if (!OpenFile())
		{
			OutputDebugStr(L"Open File Failed");
			return;
		}
	}

	DWORD writed = 0;
	if (!::WriteFile(m_file_handle, pData, size, &writed, NULL))
	{
		DWORD error = ::GetLastError();
		OutputDebugStr(L"WriteFile Failed:%d", error);
		::CloseHandle(m_file_handle);
		m_file_handle = INVALID_HANDLE_VALUE;
		return;
	}
	::WriteFile(m_file_handle, L"\r\n", 2 * 2, &writed, NULL);
}

#define MAX_DEBUG_STRING 5120
void CSimpleLog::LogFmt(const wchar_t * buffer,...)
{
	WCHAR temp[MAX_DEBUG_STRING] = { 0 };
	va_list pArgList;
	va_start(pArgList, buffer);
	vswprintf_s(temp, MAX_DEBUG_STRING,buffer, pArgList);
	va_end(pArgList);

	Log(temp);
}

void CSimpleLog::SetFile(const std::wstring & file_name)
{
	std::lock_guard<std::mutex> l(m_mutex);
	m_file_name = file_name;
}

std::wstring CSimpleLog::GetFileName() const
{
	return m_file_name;
}

bool CSimpleLog::OpenFile()
{
	if(m_file_handle && m_file_handle != INVALID_HANDLE_VALUE)
		::CloseHandle(m_file_handle);


	m_file_handle = ::CreateFile(m_file_name.c_str(),
								GENERIC_WRITE,
								FILE_SHARE_READ,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);
	if(m_file_handle != INVALID_HANDLE_VALUE)
	{
		//read already exsits
		LARGE_INTEGER file_size;
		file_size.QuadPart = 0;
		GetFileSizeEx(m_file_handle, &file_size);
		if (file_size.QuadPart > (1024 * 1024 * 10))
		{
			::CloseHandle(m_file_handle);
			m_file_handle = INVALID_HANDLE_VALUE;
			::DeleteFile(m_file_name.c_str());
		}
		else
			SetFilePointer(m_file_handle, NULL, NULL, FILE_END)  ;
	}
	if (!m_file_handle || m_file_handle == INVALID_HANDLE_VALUE)
	{
		m_file_handle = ::CreateFile(m_file_name.c_str(),
			GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			SetFilePointer(m_file_handle, NULL, NULL, FILE_END);
		}
		else
		{
			WCHAR flag = 0xFEFF;
			DWORD writeed = 0;
			::WriteFile(m_file_handle, &flag, sizeof(WCHAR), &writeed, NULL);
		}
	}
	return m_file_handle != INVALID_HANDLE_VALUE;
}

void CSimpleLog::SetPipe(int index,const std::wstring & host)
{
	std::lock_guard<std::mutex> l(m_mutex);
	m_pipe_name = L"\\\\" + host + L"\\pipe\\zds_debug_" + std::to_wstring(index);
	m_pipe_switch = true;
}
bool CSimpleLog::ConnectPipe()
{
	static DWORD last_connect = 0;
	if (GetTickCount() - last_connect > 10000)
		last_connect = GetTickCount();
	else
		return false;


	//先关闭之前的pipe
	if (m_pipe && m_pipe != INVALID_HANDLE_VALUE)
		::CloseHandle(m_pipe);

	m_pipe = CreateFile(m_pipe_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, NULL);
	if (m_pipe != INVALID_HANDLE_VALUE)
	{
		DWORD dwMode = PIPE_READMODE_MESSAGE;
		if (!SetNamedPipeHandleState(m_pipe, &dwMode, NULL, NULL))
		{
			OutputDebugStr(L"SetNamedPipeHandleState failed!");
			::CloseHandle(m_pipe);
			m_pipe = INVALID_HANDLE_VALUE;
			m_is_connected = false;
		}
		else
			m_is_connected = true;
	}
	else
	{
		auto last_error = ::GetLastError();
		if (last_error == ERROR_PIPE_BUSY)
			OutputDebugStr(L"ERROR_PIPE_BUSY  busy!");
		m_is_connected = false;
	}

	return m_pipe != INVALID_HANDLE_VALUE;
}
void CSimpleLog::SendPipe(const std::wstring & s)
{
	bool can_write = false;
	if (!m_is_connected)
	{
		if (ConnectPipe())
			can_write = true;
		else
			can_write = false;
	}
	else
		can_write = true;

	if (!can_write)
		return;

	DWORD temp = 0;
	if (!::WriteFile(m_pipe, s.c_str(), static_cast<DWORD>(s.size() * sizeof(wchar_t)), &temp, NULL))
		m_is_connected = false;
}



CSimpleLog::CRecordPump MakeRecordPump(CSimpleLog & log, CSimpleLog::severity_level l)
{
	return CSimpleLog::CRecordPump(log,l);
}