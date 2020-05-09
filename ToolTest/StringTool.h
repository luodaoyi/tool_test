#pragma once

#include <string>
using std::wstring;
using std::string;
#include <vector>

#include <stdio.h>
#include <windows.h>

using std::vector;

namespace string_tool
{
	template<typename T,typename T2>
	T lexical_cast(const T2 & s);
	wstring CharToWide(const char * szBuf);
	string WideToChar(const wchar_t * szWBuf);

	wstring CharToWide(const std::string & s);
	string WideToChar(const std::wstring & s);

	template<typename T = std::wstring>
	std::vector<T> SplitStrByFlag(const T & str, const T& sep)
	{
		std::vector<T> vec;
		size_t sep_size = sep.size();
		size_t pos1 = 0;
		size_t pos2 = str.find(sep);
		while (T::npos != pos2)
		{
			vec.emplace_back(str.substr(pos1, pos2 - pos1));
			pos1 = pos2 + sep_size;
			pos2 = str.find(sep, pos1);
		}
		vec.emplace_back(str.substr(pos1));
		return vec;
	}
	std::wstring utf8_to_wstring(const std::string& str);
	std::string wstring_to_utf8(const std::wstring& str);
	std::string gpk_to_utf8(const std::string & str);

	size_t   GetHash(const std::string& str);	
	size_t	GetHash(const std::wstring& wstr);

	bool IsUTF8String(const char* str, int length);
	std::wstring GetBufferMd5(const char * buffer, size_t len);

	wchar_t *  wstrcpy_my(wchar_t * strDest, const wchar_t * strSrc, size_t len = sizeof(wchar_t) * 1024);
	char * strcpy_my(char * szDest, const char * szSrc, size_t len = 1024);
	template<typename T>
	void ReplaceStr(T &szContent,const T &szSrc,const T &szDst)
	{
		size_t pos = 0;
		while ((pos = szContent.find(szSrc, pos)) != T::npos) {
			szContent.replace(pos, szSrc.size(), szDst);
			pos++;
		}
	}

	bool ComparStringArray(const std::vector<std::wstring> & s1, const std::vector<std::wstring > & s2);


	std::string strlower(const std::string &str);
	std::wstring strlower(const std::wstring &str);

	DWORD GetRand_For_DWORD();

	std::wstring SprintfStr(const wchar_t * fmt, ...);

}

