#pragma once
#include <vector>
#include <string>
#include <map>
class CCmdParse
{
public:
	void Parse(int argc, wchar_t * argv[]);
	void Parse(const std::wstring & s);
	bool OptionExist(const std::wstring & name) const;
	std::wstring GetStrVal(const std::wstring & key, const std::wstring & default_val);
	int GetIntVal(const std::wstring & key, int default_val);
	float GetFloatVal(const std::wstring & key, float default_val);
	std::wstring GetOption(int index);
private:
	std::vector<std::wstring> m_cmd_list;
	std::map<std::wstring, std::wstring> m_key_values;
};
