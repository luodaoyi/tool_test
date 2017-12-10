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
#include "DebugOutput.h"
#include <mutex>


namespace file_tools
{
	std::vector<string> ReadAsciiFileLines(const std::wstring & file_name)
	{
		std::ifstream in_file(file_name);
		std::vector<std::string> ret_lines;
		if (in_file.is_open())
		{
			char line_buffer[1024];
			while (in_file.getline(line_buffer, 1024))
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
		{
			OutputDebugStr( L"创建文件:%s 失败!", cwsPath.c_str());
			return FALSE;
		}

		fseek(pFile, 0, SEEK_SET);
		WCHAR wszFlag = 0xFEFF;
		fwrite(&wszFlag, sizeof(WCHAR), 1, pFile);
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
		{
			OutputDebugStr( L"ReadScriptFile Fiald! Path:%s", wsPath.c_str());
			return FALSE;
		}

		std::shared_ptr<WCHAR> pwstrBuffer(new WCHAR[wsContent.length() + 1], [](WCHAR* p){delete[] p; });
		pwstrBuffer.get()[0] = 0xFEFF;
		memcpy(pwstrBuffer.get() + 1, wsContent.c_str(), wsContent.length() * 2);

		fseek(pFile, 0, SEEK_SET);

		fwrite(pwstrBuffer.get(), sizeof(WCHAR), wsContent.length() + 1, pFile);
		fclose(pFile);
		return TRUE;

	}

	wstring GetPathByPathFile(const std::wstring & strPathFile)
	{
		wstring strLocalFullPath = strPathFile;
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
		{
			//LOG_CF(CLog::em_Log_Type::em_Log_Type_Exception, L"ReadScriptFile Fiald! Path:%s", wsPath.c_str());
			OutputDebugStr(L"ReadScriptFile Fiald! Path:%s", wsPath.c_str());
			return FALSE;
		}

		fseek(pFile, 0, SEEK_END);
		LONG lLen = ftell(pFile);
		lLen = (lLen + 2) / 2;

		std::shared_ptr<WCHAR> pwstrBuffer(new WCHAR[lLen], [](WCHAR* p){delete[] p; });
		if (pwstrBuffer == nullptr)
		{
			fclose(pFile);
			OutputDebugStr(L"Alloc Memory Fiald!");
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
		static std::mutex MutexAppendUnicodeFile;
		std::lock_guard<std::mutex> lck(MutexAppendUnicodeFile);

		if (!PathFileExists(cwsPath.c_str()))
			CreateUnicodeTextFile(cwsPath);

		FILE* pFile = nullptr;
		_wfopen_s(&pFile, cwsPath.c_str(), L"ab+");
		if (pFile == nullptr)
		{
			OutputDebugStr(L"AppendUnicodeFile Fiald! Path:%s", cwsPath.c_str());
			return FALSE;
		}

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

		FILE* pFile = nullptr;
		_wfopen_s(&pFile, file_name.c_str(), L"rb");
		if (pFile == nullptr)
		{
			OutputDebugStr(L"AppendUnicodeFile Fiald! Path:%s", file_name.c_str());
			return FALSE;
		}

		fseek(pFile, 0, SEEK_END);
		auto size = ftell(pFile);
		content.resize(size);
		fread(content.data(), 1, size, pFile);
		fclose(pFile);
		return TRUE;
	}
	


}

