﻿// ToolTest.cpp : 定义控制台应用程序的入口点。
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

#include "InlineHook.h"
using namespace std;

#include <string>

#include "ResManager.h"


//测试韩服资源名称转换
#include "Language.h"

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

int _tmain(int argc, _TCHAR* argv[])
{

// 	std::string temp = "zhangdongsheng";
// 
// 	std::vector<char> data;
// 	data.resize(temp.length());
// 	memset(data.data(), 1, data.size());

	//std::copy(szBuff, szBuff + strlen(szBuff), data.begin());
	//std::copy(temp.begin(), temp.end(), data.begin());
	//system("pause");

// 	CInlineHook Hook((DWORD)::MessageBoxA, (DWORD)nakedPrivateChat,0);
// 	Hook.Hook();
// 	::MessageBoxA(NULL, "4124141", NULL, MB_OK);
	
// 	HWND hWnd = FindWindow(NULL, L"LUA_SHOW_DEBUG");
// 	//SetWindowPos(hWnd, HWND_TOP, -100, 0, 500, 800, SWP_NOSIZE);
// 	MoveWindow(hWnd, -100, 0, 500, 800, TRUE);

// 	size_t len = std::wstring(L"이런. 아이 하나 때문에 곤란을").length();
// 
// 	CLanguage & language = CLanguage::GetInstance();
// 	std::wstring s = language.GetOtherText_By_LocalText(L"부서진 칼날");


	
	HANDLE hMutex = CreateMutex(NULL, FALSE, NULL);

	{
		SetResDeleter(hMutex, [](HANDLE & h){MyReleaseMutex(h); });
	}





	system("pause");
	return 0;
}

