#pragma once

#include <string>

class CIniFile
{
public:
	CIniFile(const std::wstring & file_name,
		bool rewrite_value = true//这个为true就是要将默认的值写到配置里面去，false则不写
		);
	CIniFile();
	~CIniFile();

	void SetFileName(const std::wstring & file_name);

	std::wstring GetIniStr(const std::wstring & key,const std::wstring & section = L"设置",const std::wstring & default_value = L"");
	void WriteIniStr(const std::wstring & key, const std::wstring & value,const std::wstring & section = L"设置");

	int GetIniInt(const std::wstring & key, const std::wstring & section = L"设置",int default_value = 0 );
	void WriteIniInt(const std::wstring & key, const int value, const std::wstring & section = L"设置");
private:
	std::wstring m_file_path_name;
	const bool m_rewrite_value;
};

