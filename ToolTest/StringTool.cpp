#include "stdafx.h"

#include "StringTool.h"

#include <windows.h>
#include <locale>
#include <codecvt>
#include <wincrypt.h>
#include "DebugOutput.h"
#include <memory>
#include <algorithm>
#include <cctype>
namespace string_tool
{


	template<>
	int lexical_cast<int,std::wstring>(const std::wstring & s)
	{
		return std::stoi(s);
	}

	template<>
	int lexical_cast<int, std::string>(const std::string & s)
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

#define BUFMD5SIZE 1024
#define MD5LEN  16
	std::wstring GetBufferMd5(const char * buffer, size_t len)
	{
		std::wstring ret = L"";
		if (len <= 0)
			throw std::length_error("buffer size is error!");
		
		// Get handle to the crypto provider
		HCRYPTPROV hProv = 0;
		if (!CryptAcquireContext(&hProv,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT))
		{
			DWORD dwStatus = GetLastError();
			throw std::exception("CryptAcquireContext failed");
		}


		HCRYPTHASH hHash = 0;
		if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		{
			DWORD dwStatus = GetLastError();
			CryptReleaseContext(hProv, 0);
			throw std::exception("CryptAcquireContext failed");
			return FALSE;
		}



		int nRemain = static_cast<int>( len);
		DWORD cbRead = 0;
		BYTE rgbFile[BUFMD5SIZE];
		while (nRemain > 0)
		{
			//bResult = ReadFile(hFile, rgbFile, BUFMD5SIZE, &cbRead, NULL)
			if (nRemain > BUFMD5SIZE)
			{
				memcpy(rgbFile, buffer, BUFMD5SIZE);
				nRemain = nRemain - BUFMD5SIZE;
				cbRead = BUFMD5SIZE;
			}
			else
			{
				memcpy(rgbFile, buffer, nRemain);
				cbRead = nRemain;
				nRemain = 0;
			}

			if (!CryptHashData(hHash, rgbFile, cbRead, 0))
			{
				DWORD dwStatus = GetLastError();
				CryptDestroyHash(hHash);
				CryptReleaseContext(hProv, 0);
				throw std::exception("CryptHashData failed!");
			}
		}

		DWORD cbHash = MD5LEN;
		BYTE rgbHash[MD5LEN];
		if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		{
			//printf("MD5 hash of file %s is: ", filename);
			CHAR rgbDigits[] = "0123456789abcdef";
			for (DWORD i = 0; i < cbHash; i++)
			{
				//printf("%c%c", rgbDigits[rgbHash[i] >> 4],rgbDigits[rgbHash[i] & 0xf]);
				wchar_t szBuf[10] = { 0 };
				swprintf_s(szBuf, 10, L"%c%c", rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
				ret += szBuf;
			}
		}
		else
		{
			DWORD dwStatus = GetLastError();
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			throw std::exception("CryptGetHashParam failed!");
		}

		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);

		return ret;
	}


	wchar_t *  wstrcpy_my(wchar_t * strDest, const wchar_t * strSrc, size_t len)
	{
		size_t	i = 0;
		LPWSTR	pDest = strDest;

		__try
		{
			while (*strSrc != '\0' && i++ < len)//空格是32,结束是0
				*strDest++ = *strSrc++;

			*strDest = '\0';//将'\0'放入到里面
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			//LOG_CF(CLog::em_Log_Type::em_Log_Type_Exception, L"wstrcpy_my发生了异常");
			OutputDebugStr(L"!!!wstrcpy_my异常");
		}
		return pDest;
	}
	char * strcpy_my(char * szDest, const char * szSrc, size_t len )
	{
		size_t i = 0;
		char * p = szDest;
		try
		{
			while (*szSrc != '\0' && i++ < len-1)
				*szDest++ = *szSrc++;
			*szDest = '\0';
		}
		catch (...)
		{
			OutputDebugStr(L"!!!wstrcpy_my异常");
		}
		return p;
	}
	/*
	std::string string_format(const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		int result = vsnprintf_s(NULL, 0, fmt, args);
		std::unique_ptr<char[]> buffer(new char[result + 1]);
		vsnprintf(buffer.get(), result + 1, fmt, args);
		va_end(args);=
		return std::string(buffer.get());
	}
	*/

	bool ComparStringArray(const std::vector<std::wstring> & s1, const std::vector<std::wstring > & s2)
	{
		if (s1.size() != s2.size())
			return false;
		for (auto & iter : s1)
		{
			if (std::find(s2.begin(), s2.end(), iter) == s2.end())
				return false;
		}
		return true;
	}

	std::string strlower(const std::string &str)
	{
		std::string newstr(str);
		std::transform(newstr.begin(), newstr.end(), newstr.begin(), tolower);
		return newstr;
	}

	std::wstring strlower(const std::wstring &str)
	{
		std::wstring newstr(str);
		std::transform(newstr.begin(), newstr.end(), newstr.begin(), towlower);
		return newstr;
	}

	DWORD GetRand_For_DWORD()
	{
		static DWORD dwSeed = ::GetTickCount();
		srand(dwSeed);
		dwSeed = rand() << 15 | rand();
		return dwSeed;
	}
	std::wstring SprintfStr(const wchar_t * buffer, ...)
	{
		va_list pArgList;
		va_start(pArgList, buffer);
		WCHAR temp[MAX_PATH] = { 0 };
		vswprintf_s(temp, buffer, pArgList);
		va_end(pArgList);

		return temp;
	}
}
