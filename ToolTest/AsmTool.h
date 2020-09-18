#pragma once
#include <windows.h>

namespace asm_tool
{
	enum ReadMemoryType
	{
		Read_Pointer = 0x1,
		Read_API = 0x2
	};

	DWORD64	ReadDWORD64(DWORD64 dwAddr);																							            //			读取内存
	UINT_PTR	ReadDWORD(UINT_PTR dwAddr);																							                //			读取内存
	WORD		ReadWORD(UINT_PTR dwAddr);																								            //			读取内存
	BYTE		ReadBYTE(UINT_PTR dwAddr);																								            //			读取内存
	float	ReadFloat(UINT_PTR dwAddr);																							                //			读取内存
	double	ReadDouble(UINT_PTR dwAddr);
	BOOL		WriteDWORD64(DWORD64 dwAddr, DWORD64 dwValue);																			            //			写入内存
	BOOL		WriteDWORD(UINT_PTR dwAddr, DWORD dwValue);																			                //			写入内存
	BOOL		WriteFloat(UINT_PTR dwAddr, float fValue);																				            //			写入内存
	BOOL		WriteBYTE(UINT_PTR dwAddr, BYTE bValue);
};

#define RD(Addr) asm_tool::ReadDWORD(Addr)
#define RB(Addr) asm_tool::ReadBYTE(Addr)