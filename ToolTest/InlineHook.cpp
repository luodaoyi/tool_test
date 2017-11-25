#include "stdafx.h"
#include "InlineHook.h"


CInlineHook::CInlineHook(DWORD dwHookAddr, DWORD dwMyAddr, DWORD nopCount) : m_HookAddr(dwHookAddr), m_MyFuncAddr(dwMyAddr), m_dwNopCount(nopCount)
{

}


CInlineHook::~CInlineHook()
{
}
#pragma pack(push, 1)
typedef struct _JMPCODE_
{
	BYTE jmp;
	DWORD addr;
}JMPCODE_, *PJMPCODE_;
#pragma pack(pop)

bool CInlineHook::Hook()
{
	if (m_has_hook)
		return true;
	if (!m_HookAddr || !m_HookAddr)
		return false;

	if (*(BYTE*)m_HookAddr == 0xe9)
	{
		//特殊情况 说明这个要hook的地方就是一个 jump，例如：JMP 5A517301
		DWORD SrcJumpAddr = m_HookAddr + 5 + *(DWORD*)(m_HookAddr + 1);
		if (SrcJumpAddr == SrcJumpAddr)
			return true;//由可以是已经被HOOK过了
		else
			m_RealHookAddr = SrcJumpAddr;
	}
	else
		m_RealHookAddr = m_HookAddr;

	if (*(BYTE*)m_MyFuncAddr == 0xe9)
	{
		//保存以前的样子
		DWORD dwE9_Hook_WillGo = m_MyFuncAddr + 5 + *(DWORD*)(m_MyFuncAddr + 1);
		m_RealMyFuncAddr = dwE9_Hook_WillGo;
	}
	else
		m_RealMyFuncAddr = m_MyFuncAddr;


	m_SaveSrcCodeSize = GetJumpCostByte();//做一个JUMP实际费了多少字节(>=5)
	const DWORD dwJumpBackAddr = m_RealHookAddr + m_SaveSrcCodeSize;//自己函数跳回的地址
	if (m_SaveSrcCodeSize == 0)
		return false;

	memcpy_s(m_SavedSrcCode, max_save_size, (const void*)m_RealHookAddr, m_SaveSrcCodeSize);

	//修改自己给定的那个函数
	if (!m_NakeFuncChangeed)
	{
		const DWORD dwNopPos = GetMyNakedFunctNopPos();
		if (dwNopPos == 0)
			return false;
		WriteProcessMemory(::GetCurrentProcess(), (LPVOID)dwNopPos, (LPCVOID)m_SavedSrcCode, m_SaveSrcCodeSize, NULL);

		//下面开始跳转到原函数的代码
		const DWORD dwNextCodeAddr = dwNopPos + m_SaveSrcCodeSize;
		JMPCODE_ jmpCode;
		jmpCode.jmp = 0xe9;
		jmpCode.addr = dwJumpBackAddr - dwNextCodeAddr - 5;
		::WriteProcessMemory(GetCurrentProcess(), (LPVOID)dwNextCodeAddr, &jmpCode, sizeof(JMPCODE_), NULL);

		m_NakeFuncChangeed = true;
	}
	//下面开始正式HOOK
	WriteHook();

	m_has_hook = true;
	return true;
}


void CInlineHook::UnHook()
{
	if (m_SavedSrcCode == 0)
		return;

	WriteProcessMemory(GetCurrentProcess(),
		(LPVOID)m_RealHookAddr,
		m_SavedSrcCode,
		m_SaveSrcCodeSize,
		NULL);
	m_has_hook = false;
}

void CInlineHook::WriteHook()
{
	///////////////下面正式进行HOOK
	JMPCODE_ jmpCode2;
	jmpCode2.jmp = 0xe9;
	//JMP的地址(88881234) – 代码地址(010073bb) – 5（字节） = 机器码跳转地址(E9 87879e74)
	jmpCode2.addr = (DWORD)m_RealMyFuncAddr - (DWORD)m_RealHookAddr - 5;
	::WriteProcessMemory(GetCurrentProcess(), (LPVOID)m_RealHookAddr, &jmpCode2, sizeof(JMPCODE_), NULL);

	BYTE nopFlag = 0x90;
	for (DWORD i = 0; i < m_dwNopCount; i++)
	{
		::WriteProcessMemory(GetCurrentProcess(), (LPVOID)(m_RealHookAddr + 5 + i), &nopFlag, 1, NULL);
	}

}

DWORD CInlineHook::GetMyNakedFunctNopPos()
{
	BYTE * pCode = NULL;
	int i = 0;
	for (i = 0, pCode = (BYTE*)m_RealMyFuncAddr; i < 100; i++,pCode++)
	{
		if (*pCode == 0x90)
			return (DWORD)pCode;
	}
	return 0;
}

DWORD CInlineHook::GetJumpCostByte()
{
	//前面没有进行修正E9？
	if (*(BYTE*)m_RealMyFuncAddr == 0xe9)
		return 0;

	DWORD dwCostLen = 5 + m_dwNopCount;
	return dwCostLen;//一条Jump语句所耗费的字节数
}



DWORD CInlineHook::GetHookAddr() const
{
	return m_HookAddr;
}