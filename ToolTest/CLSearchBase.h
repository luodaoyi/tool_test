#ifndef __CLSEARCHBASE_H__
#define __CLSEARCHBASE_H__

#include <windows.h>
#include <vector>

class CLSearchBase
{
public:
	CLSearchBase();
	~CLSearchBase();

#ifdef _WIN64
#else
	static DWORD	FindAddr(LPCSTR lpszCode, int nOffset, int nOrderNum, LPCWSTR lpszModule);
	static DWORD	FindCALL(LPCSTR lpszCode, int nOffset, DWORD dwModuleAddr, int nMov, int nOrderNum, LPCWSTR lpszModule);
	static DWORD	FindBase(LPCSTR lpszCode, int nOffset, int nMov, int nOrderNum, LPCWSTR lpszModule, DWORD dwAddrLen = 0xFFFFFFFF);
	static DWORD	FindBase_ByCALL(LPCSTR lpszCode, int nOffset, DWORD dwModuleAddr, int nMov, int nOrderNum, LPCWSTR lpszModule, int nBaseOffset, DWORD dwAddrLen = 0xFFFFFFFF);
	static BOOL		SearchBase(LPCSTR szCode, DWORD * pArray, UINT& puLen, LPCWSTR lpszModule);
	static DWORD	GetImageSize(HMODULE hm);
	static BOOL		CL_sunday(DWORD* pKey, UINT uKeyLen, BYTE* pCode, UINT uCodeLen, std::vector<int>& vlst);
	static int		GetWord_By_Char(BYTE dwWord, DWORD* pKey, UINT uKeyLen);
	static BOOL		CompCode(const DWORD * pCode, const BYTE * pMem, UINT uLen);
#endif // _WIN64

	
private:
	WCHAR	m_wchModuleName[32];																				//Search Module Name:L"bsengine_Shipping.dll"
	HANDLE	m_hProcess;																							//return OpenProcess Handle												
	WCHAR	m_wchProcName[32];																					//Search Process Name
	DWORD	m_dwModBaseAddr;																					//Module Start Address
	DWORD	m_dwBaseSize;																						//Module Size
	DWORD	m_dwPid;																							//Search Process ID(PID)
};


#endif // !__CLSEARCHBASE_H__
