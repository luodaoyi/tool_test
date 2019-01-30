#include "stdafx.h"
#include "CLSearchBase.h"
#include "AsmTool.h"
#include <vector>

CLSearchBase::CLSearchBase()
{
}

CLSearchBase::~CLSearchBase()
{
}
#ifdef _WIN64
#else
BOOL CLSearchBase::CompCode(const DWORD * pCode, const BYTE * pMem, UINT uLen)
{
	BOOL bComp = TRUE;
	for (UINT i = 0; i < uLen; ++i)
	{
		if (pCode[i] != 0x100 && (BYTE)pCode[i] != (BYTE)pMem[i])
		{
			bComp = FALSE;
			break;
		}
	}

	return bComp;
}

BOOL CLSearchBase::CL_sunday(DWORD* pKey, UINT uKeyLen, BYTE* pCode, UINT uCodeLen, std::vector<int>& vlst)
{
	//807E1000740E
	UINT uNowPos = 0;
	while (uNowPos + uKeyLen < uCodeLen)
	{
		if (CompCode(pKey, pCode + uNowPos, uKeyLen))
		{
			vlst.push_back(uNowPos);
			uNowPos += uKeyLen + 1;
			continue;
		}

		BYTE bWord = pCode[uKeyLen + uNowPos];

		int nWordPos = GetWord_By_Char(bWord, pKey, uKeyLen);
		if (nWordPos == -1)
			uNowPos += uKeyLen + 1;
		else
			uNowPos += (UINT)nWordPos;
	}

	return TRUE;
}

int CLSearchBase::GetWord_By_Char(BYTE dwWord, DWORD* pKey, UINT uKeyLen)
{
	int uLen = uKeyLen - 1;
	for (int i = uLen; i >= 0; --i)
	{
		if (pKey[i] == 0x100 || (BYTE)pKey[i] == dwWord)
		{
			return uKeyLen - i;
		}
	}
	return -1;
}

DWORD CLSearchBase::GetImageSize(HMODULE hm)
{
	DWORD dwSize = 0x0;
	_asm
	{
		PUSHAD
		MOV EBX, DWORD PTR hm
		MOV EAX, DWORD PTR DS : [EBX + 0x3C]
		LEA EAX, DWORD PTR DS : [EBX + EAX + 0x50]
		MOV EAX, DWORD PTR DS : [EAX]
		MOV DWORD PTR DS : [dwSize], EAX
		POPAD
	}	
	return dwSize;
}

BOOL CLSearchBase::SearchBase(LPCSTR szCode, DWORD * pArray, UINT& puLen, LPCWSTR lpszModule)
{
	SYSTEM_INFO		si;
	MEMORY_BASIC_INFORMATION		mbi;

	BOOL	bRetCode = FALSE;

	//将字符串转换成BYTE数组
	//32C08BE55DC3538D45F050E8????????687455BA0250E8????????8B4DF88AD88B45F02BC883C40883F901
	//32 C0 8B E5 5D C3 53 8D 45 F0 50 E8 ?? ?? ?? ?? 68 74 55 BA 02 50 E8 ?? ?? ?? ?? 8B 4D F8 8A D8 8B 45 F0 2B C8 83 C4 08 83 F9 01
	std::vector<DWORD> out_array;
	size_t len = strlen(szCode);
	size_t i = 0;
	while (i < len)
	{
		if (szCode[i] == ' ')
		{
			i += 1;
		}
		else if (szCode[i] == '?')
		{
			out_array.push_back(0x100);
			i += 2;
		}
		else
		{
			std::string schar(szCode + i, 2);
			DWORD nByte = strtol(schar.c_str(), NULL, 16);
			out_array.push_back(nByte);
			i += 2;
		}
	}
	/*
	UINT uCodeLen = static_cast<UINT>(strlen(szCode)) / 2;
	if (strlen(szCode) % 2 != 0)
	{
		MessageBoxW(NULL, L"必须是2的倍数!", L"Error", NULL);
		return FALSE;
	}

	DWORD * pCode = new DWORD[uCodeLen];
	memset(pCode, 0, uCodeLen);

	for (UINT i = 0; i < uCodeLen; ++i)
	{
		if (szCode[i * 2] != '?')
		{
			char szText[] = { szCode[i * 2], szCode[i * 2 + 1], '\0' };
			pCode[i] = (DWORD)strtol(szText, NULL, 16);
		}
		else
		{
			pCode[i] = 0x100;
		}
	}
	*/
	//初始化
	::GetSystemInfo(&si);
	HANDLE hProcess = ::GetCurrentProcess();
	DWORD dwImageBase = static_cast<DWORD>(reinterpret_cast<UINT_PTR>(::GetModuleHandleW(lpszModule)));
	DWORD dwImageSize = GetImageSize((HMODULE)dwImageBase);

	for (DWORD dwAddr = dwImageBase; dwAddr < dwImageBase + dwImageSize; dwAddr += mbi.RegionSize)
		//for (BYTE * pCurPos = (LPBYTE)si.lpMinimumApplicationAddress; pCurPos < (LPBYTE)si.lpMaximumApplicationAddress - uCodeLen; pCurPos = (PBYTE)mbi.BaseAddress + mbi.RegionSize,nCount++ )
	{
		//查询当前内存页的属性
		::VirtualQueryEx(hProcess, (LPCVOID)dwAddr, &mbi, sizeof(mbi));
		if (mbi.Protect == PAGE_READONLY)//扫描只读内存
		{
			DWORD dwOldProtect;
			::VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READ, &dwOldProtect);
		}
		if (mbi.State == MEM_COMMIT && (mbi.Protect == PAGE_EXECUTE_READ || mbi.Protect == PAGE_EXECUTE_READWRITE))
		{
			std::vector<int> vlst;
			CL_sunday(out_array.data(), out_array.size(), (PBYTE)mbi.BaseAddress, mbi.RegionSize, vlst);

			for (UINT i = 0; i < vlst.size() && puLen < 10; ++i)
			{
				pArray[puLen] = (DWORD)mbi.BaseAddress + vlst.at(i);
				++puLen;
			}

		}
	}

	if (puLen >= 10)
	{
		bRetCode = TRUE;
	}
	else if (puLen < 10 && puLen > 0)
	{
		bRetCode = TRUE;
	}
	return bRetCode;
}

DWORD CLSearchBase::FindBase(LPCSTR lpszCode, int nOffset, int nMov, int nOrderNum, LPCWSTR lpszModule, DWORD dwAddrLen)
{
	DWORD	dwArray[10] = { 0 };
	UINT	uArrayLen = 0x0;
	DWORD	dwBase = 0x0;
	DWORD	dwAddr = 0x0;

	//开始搜索基址
	if (SearchBase(lpszCode, dwArray, uArrayLen, lpszModule))
	{
		if (uArrayLen == 1)//判断只有一个的时候
			dwAddr = dwArray[0];
		else
			dwAddr = dwArray[nOrderNum];
	}

	if (dwAddr != 0x0)
	{
		if (nOffset >= 0)
			dwAddr -= abs(nOffset);
		else
			dwAddr += abs(nOffset);

		dwAddr += nMov;
		dwBase = asm_tool::ReadDWORD(dwAddr)&dwAddrLen;
	}

	return dwBase;
}

DWORD CLSearchBase::FindCALL(LPCSTR lpszCode, int nOffset, DWORD dwModuleAddr, int nMov, int nOrderNum, LPCWSTR lpszModule)
{
	DWORD	dwArray[10] = { 0 };
	UINT	uArrayLen = 0x0;
	DWORD	dwCALL = 0x0;
	DWORD	dwAddr = 0x0;

	//开始搜索基址
	if (SearchBase(lpszCode, dwArray, uArrayLen, lpszModule))
	{
		if (uArrayLen == 1)//判断只有一个的时候
			dwAddr = dwArray[0];
		else
			dwAddr = dwArray[nOrderNum];
	}

	if (dwAddr != 0x0)
	{
		if (nOffset >= 0)
			dwAddr -= abs(nOffset);
		else
			dwAddr += abs(nOffset);

		// 不是CALL!
		if (asm_tool::ReadBYTE(dwAddr) != 0xE8)
			return 0x0;

		//首先计算相对地址
		DWORD dwRelativeAddr = dwAddr - (dwModuleAddr + 0x1000) + 0x1000 + nMov;
		dwRelativeAddr += dwModuleAddr;
		DWORD dwReadAddr = asm_tool::ReadDWORD(dwRelativeAddr);//dwRelativeAddr = [0x00de6ea3+1]=FFC98798
		dwReadAddr += 4;//FFC98798+4 = FFC9879C
		dwReadAddr += dwRelativeAddr;//FFC9879C + (0x00de6ea3+1) =  100A7F640
		dwCALL = dwReadAddr & 0xFFFFFFFF;
	}

	return dwCALL;
}

DWORD CLSearchBase::FindAddr(LPCSTR lpszCode, int nOffset, int nOrderNum, LPCWSTR lpszModule)
{
	DWORD	dwArray[10] = { 0 };
	UINT	uArrayLen = 0x0;
	DWORD	dwAddr = 0x0;

	//开始搜索基址
	if (SearchBase(lpszCode, dwArray, uArrayLen, lpszModule))
	{
		if (uArrayLen == 1)//判断只有一个的时候
			dwAddr = dwArray[0];
		else
			dwAddr = dwArray[nOrderNum];
	}

	if (dwAddr != 0x0){
		if (nOffset >= 0)
			dwAddr -= abs(nOffset);
		else
			dwAddr += abs(nOffset);
	}


	return dwAddr;
}

DWORD CLSearchBase::FindBase_ByCALL(LPCSTR lpszCode, int nOffset, DWORD dwModuleAddr, int nMov, int nOrderNum, LPCWSTR lpszModule, int nBaseOffset, DWORD dwAddrLen /*= 0xFFFFFFFF*/)
{
	DWORD dwCALL = FindCALL(lpszCode, nOffset, dwModuleAddr, nMov, nOrderNum, lpszModule);
	if (dwCALL == NULL)
		return NULL;

	dwCALL += nBaseOffset;
	return asm_tool::ReadDWORD(dwCALL) & dwAddrLen;
}

DWORD CLSearchBase::GetCall(DWORD dwAddr1, DWORD dwAddr2, LPCWSTR pwszModuleName)
{
	DWORD dwAddr = dwAddr1 - dwAddr2 + 0x1/*0xE8*/;
	dwAddr += (DWORD)GetModuleHandleW(pwszModuleName);
	DWORD dwReaderAddr = asm_tool::ReadDWORD(dwAddr);
	dwReaderAddr += 4;
	dwReaderAddr += dwAddr;
	return dwReaderAddr & 0xFFFFFFFF;
}
#endif