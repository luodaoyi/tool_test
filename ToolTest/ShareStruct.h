#pragma once

#include <windows.h>
#include "DebugOutput.h"
#include "SystemTool.h"
template<typename TShareType>
class CShareStruct
{
public:
	enum SHARE_TYPE{NONE, CREATOR,OPENER,};
	CShareStruct() = default;
	CShareStruct(LPCWSTR name)
	{
		CreateShare(name);
	}
	VOID CShareStruct::CloseShare()
	{
		if (m_data_ptr)
		{
			OutputDebugStr(L"free share struct");
			m_data_ptr->~TShareType();
			UnmapViewOfFile(m_data_ptr);
			m_data_ptr = NULL;
			m_is_opening = FALSE;
		}
		if (system_tool::IsValidHandle(m_map_file_handle))
		{
			::CloseHandle(m_map_file_handle);
			m_map_file_handle = INVALID_HANDLE_VALUE;
		}
	}
	~CShareStruct()
	{
		CloseShare();
	}

	
	const TShareType * ReadShare()
	{
		return m_data_ptr;
	}

	
	TShareType * GetSharePtr()
	{
		return m_data_ptr;
	}

	BOOL CreateShare(LPCWSTR map_name)
	{
		if (!map_name || wcslen(map_name) == 0)
		{
			OutputDebugStr(L"OpenShare name is invalid");
			return FALSE;
		}
		if (m_data_ptr || system_tool::IsValidHandle(m_map_file_handle))
		{
			OutputDebugStr(L"!!try to create share twice!");
			return FALSE;
		}
		m_map_file_handle = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(TShareType), map_name);
		if (system_tool::IsValidHandle(m_map_file_handle))
		{
			m_share_type = CREATOR;
			MapMemery();
			return TRUE;
		}
		else
		{
			DWORD last_error = ::GetLastError();
			OutputDebugStr(L"CreateFileMapping Failed Name:%s Error:%d", map_name, last_error);
			return FALSE;
		}
	}
	
	BOOL OpenShare(LPCWSTR map_name)
	{
		if (!map_name || wcslen(map_name) == 0)
		{
			OutputDebugStr ( L"OpenShared Failed! name is invalid");
			return FALSE;
		}
		if (m_data_ptr || system_tool::IsValidHandle(m_map_file_handle))
		{
			OutputDebugStr(L"!!try to create share twice!");
			return FALSE;
		}

		m_map_file_handle = ::OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, map_name);
		if (system_tool::IsValidHandle(m_map_file_handle))
		{
			m_share_type = OPENER;
			MapMemery();
			return TRUE;
		}
		else
		{
			DWORD last_error = ::GetLastError();
			OutputDebugStr(L"OpenFileMapping Failed Name:%s Error:%d", map_name, last_error);
			return FALSE;
		}
	}

	const BOOL & IsShareOpened() CONST{ return m_is_opening; }

	CShareStruct(const CShareStruct &) = delete;
	void operator=(const CShareStruct &) = delete;
	BOOL IsValid() const
	{
		return system_tool::IsValidHandle(m_map_file_handle);
	}
private:
	BOOL MapMemery()
	{
		if (m_data_ptr == NULL)
		{
			m_data_ptr = (TShareType*)MapViewOfFile(m_map_file_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TShareType));
			if (m_data_ptr == NULL)
			{
				DWORD last_error = ::GetLastError();
				OutputDebugStr(L"MapViewOfFile Failed!:%d", last_error);
				return FALSE;
			}
			else
			{
				if (m_share_type == CREATOR)
					new (m_data_ptr)TShareType();//调用构造函数
				m_is_opening = TRUE;
				return TRUE;
			}
		}
		else
		{
			m_is_opening = TRUE;
			return TRUE;
		}
	}

	HANDLE m_map_file_handle = INVALID_HANDLE_VALUE;
	SHARE_TYPE m_share_type = NONE;
	TShareType * m_data_ptr = NULL;
	BOOL m_is_opening = FALSE;
};

