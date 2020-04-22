
#include "SystemTool.h"
#include "FileTool.h"
#include <ShlObj.h>
namespace system_tool
{
	BOOL IsValidHandle(const HANDLE & h)
	{
		if (h == NULL || h == INVALID_HANDLE_VALUE)
			return FALSE;
		else
			return TRUE;
	}

	BOOL LockMutex(HANDLE hHandle, DWORD dwMilliseconds)
	{
		DWORD dwRet = ::WaitForSingleObject(hHandle, dwMilliseconds);
		if (dwRet == WAIT_ABANDONED || dwRet == WAIT_OBJECT_0)
			return TRUE;
		else
			return FALSE;
	}
	VOID UnLockMutex(HANDLE hHandle)
	{
		::ReleaseMutex(hHandle);
	}

	BOOL DoActionTimeOut(DWORD dwMilliseconds, std::function<ACTION_RET(void)> fn)
	{
		DWORD begin_time = GetTickCount();
		while (true)
		{
			Sleep(500);
			if (GetTickCount() - begin_time > dwMilliseconds)
				return FALSE;	
			auto ret = fn();
			if (ret == RET_OK)
				return TRUE;
			else if (ret == RET_WAIT)
				continue;
			else if (ret == RET_BREAK)
				return FALSE;
		}
		return FALSE;
	}

	std::string GetSystemUuid()
	{
		static std::string ret_string;
		if (!ret_string.empty())
			return ret_string;

		wchar_t temp_path[MAX_PATH] = { 0 };
		SHGetSpecialFolderPath(NULL, temp_path, CSIDL_APPDATA, TRUE);
		std::wstring file_name = std::wstring (temp_path) + L"\\ZdsSoft\\uuid.dat";
		std::vector<char> file_content;
		file_tools::ReadFile(file_name.c_str(), file_content);
		if (file_content.size() == 0) {
			GUID guid;
			char szuuid[128] = { 0 };
			CoCreateGuid(&guid);
			sprintf_s(
				szuuid,
				128,
				"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
				guid.Data1, guid.Data2, guid.Data3,
				guid.Data4[0], guid.Data4[1],
				guid.Data4[2], guid.Data4[3],
				guid.Data4[4], guid.Data4[5],
				guid.Data4[6], guid.Data4[7]);

			file_tools::CreateDirectoryNested(std::wstring(temp_path) + L"\\ZdsSoft\\");

			file_tools::WriteFile(file_name, szuuid, strlen(szuuid));
			ret_string = szuuid;
			return szuuid;
		}
		else
		{


			std::string ret(file_content.data(), file_content.size());
			ret_string = ret;
			return ret;
		}
		
	}
}