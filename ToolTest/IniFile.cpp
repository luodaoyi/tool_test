#include "stdafx.h"
#include "IniFile.h"

#include <windows.h>
#include <Shlwapi.h>
#include "FileTool.h"
#pragma comment(lib,"Shlwapi.lib")
CIniFile::CIniFile(const std::wstring & file_name)
{
	if(file_tools::IsValidFilePath(file_name))
	{
		//如果是带目录的文件路径 则先创建目录
		m_file_path_name = file_name;
		if (!PathFileExists(file_name.c_str()))
		{
			std::wstring file_path = file_tools::GetPathByPathFile(file_name);
			if (!file_path.empty() && !::PathIsDirectory(file_path.c_str()))
				file_tools::CreateDirectoryNested(file_path);
		}
	}
	else
	{
		//单个文件 加上当前路径
		m_file_path_name = file_tools::GetCurrentAppPath() + file_name;
	}

}


CIniFile::~CIniFile()
{
}

std::wstring CIniFile::GetIniStr(const std::wstring & key, const std::wstring & section, const std::wstring & default_value)
{
	WCHAR value[MAX_PATH] = { 0 };
	::GetPrivateProfileString(section.c_str(), key.c_str(), default_value.c_str(), value, MAX_PATH, m_file_path_name.c_str());
	WriteIniStr(key, value, section);
	return value;
}
void CIniFile::WriteIniStr(const std::wstring & key, const std::wstring & value, const std::wstring & section)
{
	::WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), m_file_path_name.c_str());
}

int CIniFile::GetIniInt(const std::wstring & key, const std::wstring & section,int default_value)
{
	int ret = ::GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, m_file_path_name.c_str());
	WriteIniInt(key, ret, section);
	return ret;
}
void CIniFile::WriteIniInt(const std::wstring & key, const int value, const std::wstring & section)
{
	WCHAR temp[10] = { 0 };
	_itow_s(value, temp, 10);
	WriteIniStr(key, temp, section);
}