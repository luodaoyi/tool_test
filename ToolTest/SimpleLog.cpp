


#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

#include "SimpleLog.h"
#include "DebugOutput.h"
#include <ctime>
#include  <iostream>
#include <iomanip>
#include <sstream>
#include <ws2tcpip.h>
CSimpleLog::CSimpleLog()
{
	m_file_handle = INVALID_HANDLE_VALUE;
}

CSimpleLog::~CSimpleLog()
{
	::CloseHandle(m_file_handle);
	m_file_handle =  INVALID_HANDLE_VALUE;
	m_udp_switch = false;
	m_pipe_switch = false;
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
	if (m_udp_switch)
		SendUdp(GetPipeLineHead() + str);
	if (m_is_cmd_output)
		std::wcout << GetPipeLineHead() + str << std::endl;
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

	LARGE_INTEGER file_size;
	GetFileSizeEx(m_file_handle, &file_size);
	if (file_size.QuadPart > m_max_log_size)
	{
		::CloseHandle(m_file_handle);
		m_file_handle = INVALID_HANDLE_VALUE;
	}
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

void CSimpleLog::SetUdp(int index, const std::string & ip)
{
	if (index == -1)
	{
		m_udp_switch = false;
		return;
	}
		

	m_udp_switch = true;

	static bool init_sock = false;
	if (!init_sock)
	{
		init_sock = true;
		WSADATA wsaData = { 0 };
		WSAStartup(MAKEWORD(2, 2), &wsaData);
	}

	udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ZeroMemory(&recv_addr, sizeof(recv_addr));
	recv_addr.sin_family = AF_INET;
	recv_addr.sin_port = htons(10120+index);

	inet_pton(AF_INET, ip.c_str(), &recv_addr.sin_addr.s_addr);
}

void CSimpleLog::SetCmd(bool b)
{
	m_is_cmd_output = b;
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
		if (file_size.QuadPart > (m_max_log_size))
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
void CSimpleLog::SendUdp(const std::wstring & s)
{
	auto ret = sendto(udp_socket, (char*)s.c_str(), s.length() * sizeof(wchar_t), 0, (SOCKADDR *)& recv_addr, sizeof(recv_addr));
	if (ret == SOCKET_ERROR) {
		wchar_t error_buffer[256] = { 0 };
		swprintf_s(error_buffer, L"udp sendto error ret == SOCKET_ERROR :lasterror:%d", ::WSAGetLastError());
		WriteFile(error_buffer, (wcslen(error_buffer) + 1) * sizeof(wchar_t) );
	}
}

void CSimpleLog::SetLogMaxSize(LONGLONG max_size)
{
	m_max_log_size = max_size;
}


CSimpleLog::CRecordPump MakeRecordPump(CSimpleLog & log, CSimpleLog::severity_level l)
{
	return CSimpleLog::CRecordPump(log,l);
}
