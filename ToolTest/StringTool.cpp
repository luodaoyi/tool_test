#include "stdafx.h"

#include "StringTool.h"

#include <windows.h>
#include <locale>
#include <codecvt>

namespace string_tool
{


	template<>
	int lexical_cast<int,std::wstring>(const std::wstring & s)
	{
		return std::stoi(s);
	}

	template<>
	unsigned long lexical_cast<unsigned long,std::wstring>(const std::wstring &  s)
	{
		return std::stoul(s);
	}

	template<>
	std::wstring lexical_cast<std::wstring , DWORD>(const DWORD  & value)
	{
		return std::to_wstring(value);
	}




	wstring CharToWide(const char * szBuf)
	{
		int nLenBytesRequire = ::MultiByteToWideChar(CP_ACP,
			0,
			szBuf,
			-1,
			NULL,
			0);
		wchar_t * szWbuf = new wchar_t[nLenBytesRequire];
		::MultiByteToWideChar(CP_ACP,
			0,
			szBuf,
			-1,
			szWbuf,
			nLenBytesRequire * (sizeof(wchar_t)));

		wstring ret_str = szWbuf;
		delete[] szWbuf;
		return ret_str;
	}

	string WideToChar(const wchar_t * szWBuf)
	{
		BOOL bOk = FALSE;
		int nLen = ::WideCharToMultiByte(CP_ACP,
			0,
			szWBuf,
			-1,
			NULL,
			0,
			NULL,
			&bOk
			);
		char * szBuf = new char[nLen];
		::WideCharToMultiByte(CP_ACP,
			0,
			szWBuf,
			-1,
			szBuf,
			nLen,
			NULL,
			&bOk);
		string str = szBuf;
		delete[] szBuf;
		return str;
	}




	std::wstring utf8_to_wstring(const std::string& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
		return myconv.from_bytes(str);
	}
	std::string wstring_to_utf8(const std::wstring& str)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
		return myconv.to_bytes(str);
	}

	std::string gpk_to_utf8(const std::string & str)
	{
		return wstring_to_utf8(CharToWide(str.c_str()));
	}

	size_t GetHash(_In_ CONST std::string& str)
	{
		size_t result = 0;
		for (auto it = str.cbegin(); it != str.cend(); ++it) {
			result = (result * 131) + *it;
		}
		return result;
	}

	size_t GetHash(_In_ CONST std::wstring& wstr)
	{
		size_t result = 0;
		for (auto it = wstr.cbegin(); it != wstr.cend(); ++it) {
			result = (result * 131) + *it;
		}
		return result;
	}

	bool IsUTF8String(const char* str, int length)
	{
		int i = 0;
		int nBytes = 0;//UTF8可用1-6个字节编码,ASCII用一个字节
		unsigned char chr = 0;
		bool bAllAscii = true;//如果全部都是ASCII,说明不是UTF-8

		while (i < length)
		{
			chr = *(str + i);
			if ((chr & 0x80) != 0)//第一位不是0说明不是一个ascii
				bAllAscii = false;
			if (nBytes == 0)//计算字节数
			{
				if ((chr & 0x80) != 0)
				{
					while ((chr & 0x80) != 0)
					{
						chr <<= 1;
						nBytes++;
					}
					if (nBytes < 2 || nBytes > 6)
						return false;//第一个字节最少为110x xxxx
					nBytes--;//减去自身占的一个字节
				}
			}
			else//多字节除了第一个字节外剩下的字节
			{
				if ((chr & 0xc0) != 0x80)
					return false;//剩下的字节都是10xx xxxx的形式
				nBytes--;
			}
			++i;
		}
		if (bAllAscii)
			return false;
		return nBytes == 0;
	}




}