// ToolTest.cpp : 定义控制台应用程序的入口点。
//

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
void MyMessageBox(const char * szBuff)
{
	if (szBuff)
		std::cout << "MessageBox内容:" <<szBuff<< std::endl;
}

_declspec(naked) void nakedPrivateChat()
{
	__asm
	{
		pushad  //保存寄存器

			mov ebp, esp  //这是固定的
			add ebp, 0x20   //这也是固定的 处理pushad的堆栈

			mov eax, [ebp + 0x8]
			push eax
			call MyMessageBox
			add esp, 4



			popad //保存寄存器取消


			/*
			下面要nop HOOK库会将下面的nop改成相应的代码
			*/
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
			nop
	}
}

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

#include "SimpleLog.h"

#include <boost/log/sources/logger.hpp>



int _tmain(int argc, _TCHAR* argv[])
{
	_setmode(_fileno(stdout), _O_U16TEXT);

	while (true)
	{
		std::wcout << (GetAsyncKeyState(VK_DIVIDE) & 0x0001) << std::endl;
		Sleep(200);
	}
		




	return 0;
}

