// ToolTest.cpp : �������̨Ӧ�ó������ڵ㡣
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


void MyMessageBox(const char * szBuff)
{
	if (szBuff)
		std::cout << "MessageBox����:" <<szBuff<< std::endl;
}

_declspec(naked) void nakedPrivateChat()
{
	__asm
	{
		pushad  //����Ĵ���

			mov ebp, esp  //���ǹ̶���
			add ebp, 0x20   //��Ҳ�ǹ̶��� ����pushad�Ķ�ջ

			mov eax, [ebp + 0x8]
			push eax
			call MyMessageBox
			add esp, 4



			popad //����Ĵ���ȡ��


			/*
			����Ҫnop HOOK��Ὣ�����nop�ĳ���Ӧ�Ĵ���
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

int _tmain(int argc, _TCHAR* argv[])
{
	system("pause");
	std::string temp = "zhangdongsheng";

	std::vector<char> data;
	data.resize(temp.length());
	memset(data.data(), 1, data.size());

	//std::copy(szBuff, szBuff + strlen(szBuff), data.begin());
	//std::copy(temp.begin(), temp.end(), data.begin());
	system("pause");

	CInlineHook Hook((DWORD)::MessageBoxA, (DWORD)nakedPrivateChat,0);

	Hook.Hook();
	::MessageBoxA(NULL, "4124141", NULL, MB_OK);

	system("pause");
	return 0;
}

