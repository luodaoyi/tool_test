#pragma once

#include <string>
using std::wstring;
using std::string;
#include <vector>
using std::vector;

namespace string_tool
{
	template<typename T,typename T2>
	T lexical_cast(const T2 & s);
	wstring CharToWide(const char * szBuf);
	string WideToChar(const wchar_t * szWBuf);
	std::vector<wstring> SplitStrByFlag(const std::wstring & str, const std::wstring& strFlag);

	std::wstring utf8_to_wstring(const std::string& str);
	std::string wstring_to_utf8(const std::wstring& str);

	size_t   GetHash(const std::string& str);	
	size_t	GetHash(const std::wstring& wstr);

	bool IsUTF8String(const char* str, int length);


}