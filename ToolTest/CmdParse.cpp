#include "stdafx.h"
#include "CmdParse.h"


#include <vector>
#include <string>
#include <map>

void CCmdParse::Parse(int argc, wchar_t * argv[])
{
	for (int i = 0; i < argc; i++)
		m_cmd_list.push_back(argv[i]);
	for (const std::wstring & option : m_cmd_list)
	{
		auto equal_pos = option.find(L'=');
		if (equal_pos != std::wstring::npos && equal_pos < option.length() - 1)
		{
			std::wstring key = option.substr(0, equal_pos);
			std::wstring val = option.substr(equal_pos + 1);
			m_key_values[key] = val;
		}
	}
}
bool CCmdParse::OptionExist(const std::wstring & name) const
{
	for (const std::wstring & s : m_cmd_list)
	{
		if (s == name)
			return true;
	}

	for (auto iter = m_key_values.begin(); iter != m_key_values.end(); iter++)
		if (iter->first == name)
			return true;
	return false;
}

std::wstring CCmdParse::GetStrVal(const std::wstring & key, const std::wstring & default_val)
{
	auto iter = m_key_values.find(key);
	if (iter != m_key_values.end())
		return iter->second;
	return default_val;
}

int CCmdParse::GetIntVal(const std::wstring & key, int default_val)
{
	return std::stoi(GetStrVal(key, std::to_wstring(default_val)));
}
float CCmdParse::GetFloatVal(const std::wstring & key, float default_val)
{
	return std::stof(GetStrVal(key, std::to_wstring(default_val)));
}