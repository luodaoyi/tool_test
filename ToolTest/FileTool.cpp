#include "stdafx.h"
#include "FileTool.h"


#include <sstream>
#include <fstream>

#include "StringTool.h"
#include "Shlwapi.h"

#pragma comment(lib,"Shlwapi.lib")
#include <locale>
#include <codecvt>
#include <memory>
#include <mutex>
#include <algorithm>


#include "ResManager.h"

#include <tchar.h>

#include <ShlObj.h>


namespace file_tools
{
	std::vector<std::string> ReadAsciiFileLines(const std::wstring & file_name)
	{
		std::ifstream in_file(file_name);
		std::vector<std::string> ret_lines;
		if (in_file.is_open())
		{
			char line_buffer[10240];
			while (in_file.getline(line_buffer, 10240))
			{
				ret_lines.push_back(line_buffer);
			}
			in_file.close();
		}
		return ret_lines;
	}

	std::vector<std::wstring> ReadUnicodeFileLines(const std::wstring & file_name)
	{
		std::wifstream in_file(file_name);
		std::vector<std::wstring> ret_lines;
		if (in_file.is_open())
		{
			auto loc = std::locale(in_file.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>);
			auto name = loc.name();
			in_file.imbue(loc);
			wchar_t line_buffer[10240] = { 0 };
			while (in_file.getline(line_buffer, 10240))
			{
				std::wstring line = line_buffer;
				if (line.empty())
					continue;
				if (*line.rbegin() == '\r')
					line.erase(line.length() - 1);
				ret_lines.push_back(line);
			}
			in_file.close();
		}
		return ret_lines;
	}


	BOOL CreateUnicodeTextFile(_In_ CONST std::wstring& cwsPath)
	{
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, cwsPath.c_str(), L"a");
		if (pFile == NULL)
			return FALSE;

		fseek(pFile, 0, SEEK_SET);
		WCHAR wszFlag = 0xFEFF;
		fwrite(&wszFlag, sizeof(WCHAR), 1, pFile);
		fclose(pFile);
		return TRUE;
	}
	BOOL CreateNormalFile(const std::wstring & cwsPath)
	{
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, cwsPath.c_str(), L"a");
		if (pFile == NULL)
			return FALSE;
		fclose(pFile);
		return TRUE;
	}

	BOOL WriteUnicodeFile(const std::wstring & wsPath, const std::wstring & wsContent)
	{
		static std::mutex MutexWriteUnicodeFile;
		std::lock_guard<std::mutex> lck(MutexWriteUnicodeFile);
		if (!PathFileExists(wsPath.c_str()))
			CreateUnicodeTextFile(wsPath);

		FILE* pFile = nullptr;
		_wfopen_s(&pFile, wsPath.c_str(), L"wb+");
		if (pFile == nullptr)
			return FALSE;

		std::shared_ptr<WCHAR> pwstrBuffer(new WCHAR[wsContent.length() + 1], [](WCHAR* p){delete[] p; });
		pwstrBuffer.get()[0] = 0xFEFF;
		memcpy(pwstrBuffer.get() + 1, wsContent.c_str(), wsContent.length() * 2);

		fseek(pFile, 0, SEEK_SET);

		fwrite(pwstrBuffer.get(), sizeof(WCHAR), wsContent.length() + 1, pFile);
		fclose(pFile);
		return TRUE;

	}

	std::wstring GetPathByPathFile(const std::wstring & strPathFile)
	{
		std::wstring strLocalFullPath = strPathFile;
		WCHAR szDir[256], szDrive[20], szName[256], szExt[60];
		if (_wsplitpath_s(strLocalFullPath.c_str(),
			szDrive, 20,
			szDir, 256,
			szName, 256,
			szExt, 60
			) == 0)
			return  std::wstring(szDrive) + szDir;//为了得到文件夹
		else
			return L"";
	}

	std::wstring GetFileByPathFile(const std::wstring & strPathFile)
	{
		std::wstring strLocalFullPath = strPathFile;
		WCHAR szDir[256], szDrive[20], szName[256], szExt[60];
		if (_wsplitpath_s(strLocalFullPath.c_str(),
			szDrive, 20,
			szDir, 256,
			szName, 256,
			szExt, 60
		) == 0)
			return  std::wstring(szName) + szExt;//为了得到文件夹
		else
			return L"";
	}
	BOOL IsValidFilePath(const std::wstring & file_path)
	{
		WCHAR szDir[256] = {0};
		WCHAR szDrive[20] = {0};
		WCHAR szName[256] = {0};
		WCHAR szExt[60] = {0};
		if (_wsplitpath_s(file_path.c_str(),
			szDrive, 20,
			szDir, 256,
			szName, 256,
			szExt, 60
			) == 0)
		{
			if(wcslen(szDir) > 0 && wcslen(szDrive) > 0)
				return TRUE;
			else
				return FALSE;
		}
		else
			return FALSE;
	}
	const std::wstring GetCurrentAppPath()
	{
		static std::wstring current_app_path;
		if (current_app_path.empty())
		{
			WCHAR current_exe_file_path[MAX_PATH] = { 0 };
			GetModuleFileNameW(NULL, current_exe_file_path, MAX_PATH);
			current_app_path = GetPathByPathFile(current_exe_file_path);
		}
		return current_app_path;
	}

	std::wstring GetCurrentPath()
	{
		wchar_t buff[MAX_PATH] = { 0 };
		GetCurrentDirectory(MAX_PATH, buff);
		return std::wstring(buff) + L"\\";
	}


	template<size_t nSize>
	inline void ModifyPathSpec(TCHAR(&szDst)[nSize], BOOL  bAddSpec)
	{
		int nLen = lstrlen(szDst);
		if (nLen <= 0) return;
		TCHAR  ch = szDst[nLen - 1];
		if ((ch == L'\\') || (ch == L'/'))
		{
			if (!bAddSpec)
			{
				szDst[nLen - 1] = L'\0';
			}
		}
		else
		{
			if (bAddSpec)
			{
				szDst[nLen] = L'\\';
				szDst[nLen + 1] = L'\0';
			}
		}
	}

	//嵌套创建文件夹
	BOOL  CreateDirectoryNested(const std::wstring &  path)
	{
		if (::PathIsDirectory(path.c_str())) return TRUE;
		TCHAR   szPreDir[MAX_PATH];
		wcscpy_s(szPreDir, path.c_str());
		//确保路径末尾没有反斜杠
		ModifyPathSpec(szPreDir, FALSE);
		//获取上级目录
		BOOL  bGetPreDir = ::PathRemoveFileSpec(szPreDir);
		if (!bGetPreDir) return FALSE;

		//如果上级目录不存在,则递归创建上级目录
		if (!::PathIsDirectory(szPreDir))
		{
			CreateDirectoryNested(szPreDir);
		}

		return ::CreateDirectory(path.c_str(), NULL);
	}

	

	BOOL ReadUnicodeFile(_In_ CONST std::wstring& wsPath, _Out_ std::wstring& wsContent)
	{
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, wsPath.c_str(), L"rb");
		if (pFile == nullptr)
			return FALSE;


		fseek(pFile, 0, SEEK_END);
		LONG lLen = ftell(pFile);
		lLen = (lLen + 2) / 2;

		std::shared_ptr<WCHAR> pwstrBuffer(new WCHAR[lLen], [](WCHAR* p){delete[] p; });
		if (pwstrBuffer == nullptr)
		{
			fclose(pFile);
			return FALSE;
		}

		ZeroMemory(pwstrBuffer.get(), lLen * sizeof(WCHAR));
		fseek(pFile, 0, SEEK_SET);
		fread(pwstrBuffer.get(), sizeof(WCHAR), (size_t)lLen - 1, pFile);
		pwstrBuffer.get()[lLen - 1] = '\0';

		wsContent = pwstrBuffer.get() + ((pwstrBuffer.get()[0] == 0xFEFF) ? 1 : 0);
		fclose(pFile);
		return TRUE;
	}

	BOOL  AppendUnicodeFile(_In_ CONST std::wstring& cwsPath, _In_ CONST std::wstring& cwsContent)
	{
		if (cwsContent.empty())
			return TRUE;
		static std::mutex MutexAppendUnicodeFile;
		std::lock_guard<std::mutex> lck(MutexAppendUnicodeFile);

		if (!PathFileExists(cwsPath.c_str()))
			CreateUnicodeTextFile(cwsPath);

		FILE* pFile = nullptr;
		_wfopen_s(&pFile, cwsPath.c_str(), L"ab+");
		if (pFile == nullptr)
			return FALSE;

		fseek(pFile, 0, SEEK_END);

		std::wstring wsContent = cwsContent;
		if (cwsContent[cwsContent.length() - 1] != '\r\n')
			wsContent.append(L"\r\n");

		fwrite(wsContent.c_str(), sizeof(WCHAR), wsContent.length(), pFile);
		fclose(pFile);
		return TRUE;
	}

	BOOL ReadFile(const std::wstring & file_name, std::vector<char>  &content)
	{

		HANDLE file_handle = ::CreateFile(file_name.c_str(),
			GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file_handle == INVALID_HANDLE_VALUE)
			return FALSE;
		SetResDeleter(file_handle, [](HANDLE & p){::CloseHandle(p); });
		LARGE_INTEGER  file_size = { 0 };
		GetFileSizeEx(file_handle, &file_size);
		content.resize(file_size.LowPart);
		DWORD read_types = 0;
		BOOL bret_ret = ::ReadFile(file_handle, content.data(), file_size.LowPart, &read_types, NULL);
		return (bret_ret && read_types == file_size.LowPart);
	}

	std::string ReadFileToString(const std::wstring & file_name)
	{
		std::vector<char> content;
		ReadFile(file_name, content);
		std::string ret(content.begin(), content.end());
		return ret;
	}
	


	BOOL FileExist(const std::wstring & file_name)
	{
		return PathFileExists(file_name.c_str());
	}
	BOOL WriteFile(const std::wstring & file_namme, const char * buffer, size_t size,DWORD flag)
	{
		HANDLE file_handle = ::CreateFile(file_namme.c_str(), GENERIC_WRITE, 0, NULL, flag, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file_handle == INVALID_HANDLE_VALUE)
			return FALSE;
		SetResDeleter(file_handle, [](HANDLE & h){::CloseHandle(h); });
		DWORD write_size = 0;
		return ::WriteFile(file_handle, buffer, static_cast<DWORD>( size), &write_size, NULL);
	}

	BOOL  ReadAsciiFileLen(_In_ CONST std::wstring& cwsPath, _Out_ ULONG& ulFileLen)
	{
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, cwsPath.c_str(), L"rb");
		if (pFile == nullptr)
			return FALSE;

		fseek(pFile, 0, SEEK_END);
		LONG lLen = ftell(pFile);
		fclose(pFile);

		ulFileLen = lLen;
		return TRUE;
	}


#include <Wincrypt.h>

#define BUFSIZE 1024
#define MD5LEN  16
	std::wstring CalcFileMd5(LPCTSTR szFileName)
	{
		std::wstring ret_md5;

		HANDLE hFile = CreateFile(szFileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_SEQUENTIAL_SCAN,
			NULL);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			DWORD dwStatus = GetLastError();
			return ret_md5;
		}

		SetResDeleter(hFile, [](HANDLE & h){::CloseHandle(h); });

		// Get handle to the crypto provider
		HCRYPTPROV hProv = NULL;
		if (!CryptAcquireContext(&hProv,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT))
		{
			DWORD dwStatus = GetLastError();
			CloseHandle(hFile);
			return ret_md5;
		}
		SetResDeleter(hProv, [](HCRYPTPROV  & h){CryptReleaseContext(h, 0); });

		HCRYPTHASH hHash = NULL;
		if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
		{
			DWORD dwStatus = GetLastError();
			return ret_md5;
		}
		SetResDeleter(hHash, [](HCRYPTHASH &h){CryptDestroyHash(h); });
		DWORD cbRead = 0;
		BYTE rgbFile[BUFSIZE];
		BOOL bResult = FALSE;
		while (bResult = ::ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
		{
			if (0 == cbRead)
				break;
			if (!CryptHashData(hHash, rgbFile, cbRead, 0))
			{
				DWORD dwStatus = GetLastError();
			}
		}

		if (!bResult)
		{
			DWORD dwStatus = GetLastError();
			return ret_md5;
		}

		DWORD cbHash = MD5LEN;
		BYTE rgbHash[MD5LEN] = { 0 };
		CHAR rgbDigits[] = "0123456789abcdef";
		if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
		{
			TCHAR szMd5[256] = { 0 };
			for (DWORD i = 0; i < cbHash; i++)
			{
				TCHAR szBuf[10] = { 0 };
				_stprintf_s(szBuf, _T("%c%c"), rgbDigits[rgbHash[i] >> 4], rgbDigits[rgbHash[i] & 0xf]);
				_tcscat_s(szMd5, szBuf);
			}
			ret_md5 = szMd5;
		}
		else
		{
			DWORD dwStatus = GetLastError();
			return ret_md5;
		}
		return ret_md5;
	}

	BOOL GetFileNameList(std::vector<std::wstring> & retFileNameList, const std::wstring & strFolder, const std::wstring & suffix)
	{
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WCHAR szFindDir[MAX_PATH] = {0};
		wcscpy_s(szFindDir,strFolder.c_str());
		wcscat_s(szFindDir,suffix.c_str());
		hFind = FindFirstFile(szFindDir,&ffd);

		if(INVALID_HANDLE_VALUE == hFind)
		{
			//::MessageBoxA(NULL,"hFind is INVALID_HANDLE_VALUE",NULL,MB_OK);
			return FALSE;;
		}

		BOOL bRet = FALSE;
		do
		{
			if(wcscmp(ffd.cFileName,L".") == 0 || wcscmp(ffd.cFileName,L"..") == 0)
				continue;
			if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				GetFileNameList(retFileNameList,strFolder + ffd.cFileName + L"\\",suffix);
			}
			else
			{
				std::wstring strFileName = ffd.cFileName;
				retFileNameList.push_back((strFolder + ffd.cFileName));
			}
		}while(FindNextFile(hFind,&ffd) != 0);
		FindClose(hFind);
		return TRUE;
	}


	std::wstring ReadUtf8FileStr(const std::wstring & file_name)
	{
		std::vector<char> data;
		ReadFile(file_name, data);
		if (data.size() == 0)
			return L"";
		std::string str(data.data(), data.size());
		//if (str[0] == 0xef && str[1] == 0xbb && str[2] == 0xbf)
		//	str = str.substr(3);
		return string_tool::utf8_to_wstring(str);
	}
	void WriteUtf8FileStr(const std::wstring & file_name, const std::wstring & file_context,bool bom)
	{
		std::string str_utf8 = string_tool::wstring_to_utf8(file_context);
		WriteFile(file_name, str_utf8.c_str(), str_utf8.length());
	}
	std::wstring GetFileType(const std::wstring & file_name)
	{
		auto pos = file_name.rfind(L'.');
		if (pos != std::wstring::npos)
		{
			std::wstring type_str = file_name.substr(pos + 1);
			std::transform(type_str.begin(), type_str.end(), type_str.begin(), towlower);
			return type_str;
		}
		return std::wstring();
	}
	BOOL GetFileNameListNoPath(std::vector<std::wstring> & retFileNameList, const std::wstring & strFolder, const std::wstring & suffix)
	{
		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WCHAR szFindDir[MAX_PATH] = { 0 };
		wcscpy_s(szFindDir, strFolder.c_str());
		wcscat_s(szFindDir, suffix.c_str());
		hFind = FindFirstFile(szFindDir, &ffd);

		if (INVALID_HANDLE_VALUE == hFind)
		{
			//::MessageBoxA(NULL,"hFind is INVALID_HANDLE_VALUE",NULL,MB_OK);
			return FALSE;;
		}

		BOOL bRet = FALSE;
		do
		{
			if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
				continue;
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				GetFileNameList(retFileNameList, strFolder + ffd.cFileName + L"\\", suffix);
			}
			else
			{
				std::wstring strFileName = ffd.cFileName;
				retFileNameList.push_back(( ffd.cFileName));
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
		return TRUE;
	}

	DWORD64 GetFolderSize(const std::wstring & path)
	{
		DWORD64 ret_size = 0;

		WIN32_FIND_DATAW ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WCHAR szFindDir[MAX_PATH] = { 0 };
		wcscpy_s(szFindDir, path.c_str());
		wcscat_s(szFindDir, L"*.*");
		hFind = FindFirstFile(szFindDir, &ffd);

		if (INVALID_HANDLE_VALUE == hFind)
		{
			//::MessageBoxA(NULL,"hFind is INVALID_HANDLE_VALUE",NULL,MB_OK);
			return FALSE;;
		}

		BOOL bRet = FALSE;
		do
		{
			if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0)
				continue;
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//GetFileNameList(retFileNameList, strFolder + ffd.cFileName + L"\\", suffix);
			}
			else
			{
				DWORD64 file_size = ffd.nFileSizeHigh;
				file_size <<= 32;
				file_size += ffd.nFileSizeLow;
				ret_size += file_size;
				
			}
		} while (FindNextFile(hFind, &ffd) != 0);
		FindClose(hFind);
		return ret_size;
	}

	std::wstring GetTempFolder()
	{
		wchar_t path[MAX_PATH] = { 0 };
		GetTempPath(MAX_PATH, path);
		std::wstring ret_path = path;
		if (ret_path.back() != L'\\')
			ret_path += L'\\';
		return ret_path;
	}

	std::wstring GetTmpFileName()
	{
		wchar_t tmp_path[MAX_PATH] = { 0 };
		GetTempPath(MAX_PATH, tmp_path);

		wchar_t buffer[MAX_PATH] = { 0 };
		if (GetTempFileName(tmp_path, // directory for tmp files
			TEXT("Ck"),     // temp file name prefix 
			0,                // create unique name 
			buffer) == 0)  // buffer for name 
			return L"";
		return buffer;
	}

}

