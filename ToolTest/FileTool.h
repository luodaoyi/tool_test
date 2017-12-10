#pragma once


#include "commom_include.h"

#include <vector>

namespace file_tools
{ 
	std::vector<std::string> ReadAsciiFileLines(const std::wstring & file_name);
	std::vector<std::wstring> ReadUnicodeFileLines(const std::wstring & file_name);
	const std::wstring GetCurrentAppPath();
	std::wstring GetPathByPathFile(const std::wstring & strPathFile);
	BOOL  CreateDirectoryNested(const std::wstring &  path);//嵌套创建文件夹
	BOOL IsValidFilePath(const std::wstring & file_path);
	BOOL ReadUnicodeFile(_In_ CONST std::wstring& wsPath, _Out_ std::wstring& wsContent);
	BOOL WriteUnicodeFile(const std::wstring & file_name, const std::wstring & wsContent);
	BOOL  AppendUnicodeFile(_In_ CONST std::wstring& cwsPath, _In_ CONST std::wstring& cwsContent);
	BOOL ReadFile(const std::wstring & file_name, std::vector<char>  &content);
}


