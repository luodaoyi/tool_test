#pragma once

#include "commom_include.h"

class CIniFile
{
public:
	CIniFile(const std::wstring & file_name);
	~CIniFile();

	std::wstring GetIniStr(const std::wstring & key,const std::wstring & section,const std::wstring & default_value = L"");
	void WriteIniStr(const std::wstring & key, const std::wstring & value,const std::wstring & section);

	int GetIniInt(const std::wstring & key, const std::wstring & section,int default_value = 0 );
	void WriteIniInt(const std::wstring & key, const int value, const std::wstring & section);
private:
	std::wstring m_file_path_name;
};

