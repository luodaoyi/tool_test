#pragma once

#include <string>

class CIniFile
{
public:
	CIniFile(const std::wstring & file_name,
		bool rewrite_value = true//���Ϊtrue����Ҫ��Ĭ�ϵ�ֵд����������ȥ��false��д
		);
	CIniFile();
	~CIniFile();

	void SetFileName(const std::wstring & file_name);

	std::wstring GetIniStr(const std::wstring & key,const std::wstring & section = L"����",const std::wstring & default_value = L"");
	void WriteIniStr(const std::wstring & key, const std::wstring & value,const std::wstring & section = L"����");

	int GetIniInt(const std::wstring & key, const std::wstring & section = L"����",int default_value = 0 );
	void WriteIniInt(const std::wstring & key, const int value, const std::wstring & section = L"����");
private:
	std::wstring m_file_path_name;
	const bool m_rewrite_value;
};

