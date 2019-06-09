#include "stdafx.h"

#include "IniFile.h"
#include <windows.h>
#include <Shlwapi.h>
#include "FileTool.h"
#pragma comment(lib,"Shlwapi.lib")

#include "DebugOutput.h"


CIniFile::CIniFile() : m_rewrite_value(true)
{

}

void CIniFile::SetFileName(const std::wstring & file_name)
{
	if (file_name.empty())
	{
		OutputDebugStr(L"!!!file_name is empty");
		return;
	}
	if (file_tools::IsValidFilePath(file_name))
	{
		//如果是带目录的文件路径 则先创建目录
		m_file_path_name = file_name;
		if (!PathFileExists(file_name.c_str()))
		{
			std::wstring file_path = file_tools::GetPathByPathFile(file_name);
			if (!file_path.empty() && !::PathIsDirectory(file_path.c_str()))
				file_tools::CreateDirectoryNested(file_path);

			file_tools::CreateUnicodeTextFile(file_name);
		}
	}
	else
	{
		//单个文件 加上当前路径
		m_file_path_name = file_tools::GetCurrentPath() + file_name;
		if (!PathFileExists(m_file_path_name.c_str()))
			file_tools::CreateUnicodeTextFile(m_file_path_name);
	}
}

CIniFile::CIniFile(const std::wstring & file_name, bool br) :m_rewrite_value(br)
{
	SetFileName(file_name);
}


CIniFile::~CIniFile()
{
}

std::wstring CIniFile::GetIniStr(const std::wstring & key, const std::wstring & section, const std::wstring & default_value)
{
	if (m_file_path_name.empty())
		return L"";
	WCHAR value[MAX_PATH] = { 0 };
	::GetPrivateProfileString(section.c_str(), key.c_str(), default_value.c_str(), value, MAX_PATH, m_file_path_name.c_str());
	if (m_rewrite_value)
		WriteIniStr(key, value, section);
	return value;
}
void CIniFile::WriteIniStr(const std::wstring & key, const std::wstring & value, const std::wstring & section)
{
	if (m_file_path_name.empty())
		return;
	::WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), m_file_path_name.c_str());
}

int CIniFile::GetIniInt(const std::wstring & key, const std::wstring & section,int default_value)
{
	if (m_file_path_name.empty())
		return 0;
	int ret = ::GetPrivateProfileInt(section.c_str(), key.c_str(), default_value, m_file_path_name.c_str());
	if (m_rewrite_value)
		WriteIniInt(key, ret, section);
	return ret;
}
void CIniFile::WriteIniInt(const std::wstring & key, const int value, const std::wstring & section)
{
	if (m_file_path_name.empty())
		return;
	WCHAR temp[10] = { 0 };
	_itow_s(value, temp, 10);
	WriteIniStr(key, temp, section);
}