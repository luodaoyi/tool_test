#pragma once


#include "commom_include.h"

#include <vector>

namespace file_tools
{ 
	std::vector<std::wstring> ReadAsciiFileLines(const std::wstring & file_name);
	const std::wstring GetCurrentAppPath();
	std::wstring GetPathByPathFile(const std::wstring & strPathFile);
	BOOL  CreateDirectoryNested(const std::wstring &  path);//Ƕ�״����ļ���
	BOOL IsValidFilePath(const std::wstring & file_path);
}


