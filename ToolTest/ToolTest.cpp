// ToolTest.cpp : 定义控制台应用程序的入口点。
//

#include "SimpleLog.h"
#include "stdafx.h"
#include "StringTool.h"

#include "FileTool.h"
#include "ClassInstance.h"
#include "ProcessTool.h"
#include "ShareStruct.h"
#include <vector>
#include <thread>
#include <iostream>
#include <memory>
#include "HandleMy.h"
#include <assert.h>
#include "BoostLog.h"
#include <fstream>

#include "InlineHook.h"
using namespace std;

#include <string>

#include "ResManager.h"
#include <deque>
#include "PublicTool.h"

//测试韩服资源名称转换
#include "Language.h"
#include <functional>
#include "verification.h"

#include <set>


struct CTestCopy
{
	CTestCopy(){}
	CTestCopy(const CTestCopy & t)
	{
		x = t.x;
	}
	int x ;
	int y ;
};

void MyReleaseMutex(HANDLE h)
{
	::CloseHandle(h);
}

struct Test
{
	int member_count;
	std::wstring fuben_name;
	std::string caree_group;
	bool is_cross_team;

	operator bool() const
	{
		return member_count == 10;
	}
};


#include "TimeTool.h"

#include <io.h>
#include <fcntl.h>



#define THROW_SYSTEM_ERROR(X) { DWORD dwErrVal = GetLastError();\
 std::error_code ec(dwErrVal, std::system_category()); \
 throw std::system_error(ec, X); }



decltype(&::MessageBoxA) fpMessageBoxA = NULL;
int WINAPI MyMessageBoxA(
	_In_opt_ HWND hWnd,
	_In_opt_ LPCSTR lpText,
	_In_opt_ LPCSTR lpCaption,
	_In_ UINT uType)
{
	fpMessageBoxA(NULL, "dddddddd", NULL, MB_OK);
	return 0;
}

_declspec(naked) void nakedFunc()
{
	__asm {
		pushad  //保存寄存器

		mov esi, ebp
		mov ebp, esp  //这是固定的
		add ebp, 0x20   //这也是固定的 处理pushad的堆栈

		popad //保存寄存器取消
	}
	__asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop 
	__asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop 
	__asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop 
	__asm nop __asm nop __asm nop __asm nop __asm nop __asm nop __asm nop
}

#include "NakedHook.h"
#include <ctime>
#include <iomanip>


#include "ThreadPool.h"
int _tmain(int argc, _TCHAR* argv[])
{
	system("pause");
	return 0;
}


