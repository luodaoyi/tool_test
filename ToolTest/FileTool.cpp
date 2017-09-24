#include "stdafx.h"
#include "FileTool.h"


#include <sstream>
#include <fstream>

#include "StringTool.h"
#include "Shlwapi.h"
#include <locale>
#pragma comment(lib,"Shlwapi.lib")

#include <codecvt>
#include "BoostLog.h"


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
			wchar_t line_buffer[10240] = { 0 };
			while (in_file.getline(line_buffer, 10240))
			{
				ret_lines.push_back(line_buffer);
			}
			in_file.close();
		}
		return ret_lines;
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
			LOGW(error) << L"ReadScriptFile Fiald! Path:" << wsPath;
			return FALSE;
		}

		fseek(pFile, 0, SEEK_END);
		LONG lLen = ftell(pFile);
		lLen = (lLen + 2) / 2;

		std::shared_ptr<WCHAR> pwstrBuffer(new WCHAR[lLen], [](WCHAR* p){delete[] p; });
		if (pwstrBuffer == nullptr)
		{
			fclose(pFile);
			LOGW(error) << L"Alloc Memory Fiald!";
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
	
}

