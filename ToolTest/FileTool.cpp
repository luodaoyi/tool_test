#include "stdafx.h"
#include "FileTool.h"


#include <sstream>
#include <fstream>

#include "StringTool.h"
#include "Shlwapi.h"
#pragma comment(lib,"Shlwapi.lib")

namespace file_tools
{
	std::vector<wstring> ReadAsciiFileLines(const std::wstring & file_name)
	{
		std::ifstream in_file(file_name);
		std::vector<std::wstring> ret_lines;
		if (in_file.is_open())
		{
			char line_buffer[1024];
			while (in_file.getline(line_buffer, 1024))
			{
				ret_lines.push_back(string_tool::CharToWide(line_buffer));
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
			return  std::wstring(szDrive) + szDir;//Ϊ�˵õ��ļ���
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

	//Ƕ�״����ļ���
	BOOL  CreateDirectoryNested(const std::wstring &  path)
	{
		if (::PathIsDirectory(path.c_str())) return TRUE;
		TCHAR   szPreDir[MAX_PATH];
		wcscpy_s(szPreDir, path.c_str());
		//ȷ��·��ĩβû�з�б��
		ModifyPathSpec(szPreDir, FALSE);
		//��ȡ�ϼ�Ŀ¼
		BOOL  bGetPreDir = ::PathRemoveFileSpec(szPreDir);
		if (!bGetPreDir) return FALSE;

		//����ϼ�Ŀ¼������,��ݹ鴴���ϼ�Ŀ¼
		if (!::PathIsDirectory(szPreDir))
		{
			CreateDirectoryNested(szPreDir);
		}

		return ::CreateDirectory(path.c_str(), NULL);
	}

	
}

