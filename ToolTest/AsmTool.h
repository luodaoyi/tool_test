#pragma once
#include <windows.h>

namespace asm_tool
{
	enum ReadMemoryType
	{
		Read_Pointer = 0x1,
		Read_API = 0x2
	};

	DWORD64	ReadDWORD64(DWORD64 dwAddr);																							            //			��ȡ�ڴ�
	UINT_PTR	ReadDWORD(UINT_PTR dwAddr);																							                //			��ȡ�ڴ�
	WORD		ReadWORD(UINT_PTR dwAddr);																								            //			��ȡ�ڴ�
	BYTE		ReadBYTE(UINT_PTR dwAddr);																								            //			��ȡ�ڴ�
	float	ReadFloat(UINT_PTR dwAddr);																							                //			��ȡ�ڴ�
	double	ReadDouble(UINT_PTR dwAddr);
	BOOL		WriteDWORD64(DWORD64 dwAddr, DWORD64 dwValue);																			            //			д���ڴ�
	BOOL		WriteDWORD(UINT_PTR dwAddr, DWORD dwValue);																			                //			д���ڴ�
	BOOL		WriteFloat(UINT_PTR dwAddr, float fValue);																				            //			д���ڴ�
	BOOL		WriteBYTE(UINT_PTR dwAddr, BYTE bValue);
};

#define RD(Addr) asm_tool::ReadDWORD(Addr)
#define RB(Addr) asm_tool::ReadBYTE(Addr)