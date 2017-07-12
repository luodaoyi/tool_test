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
using namespace std;


int GetX()
{
	return 100;
}

int _tmain(int argc, _TCHAR* argv[])
{

	struct TTest
	{
		int x;
		int y;
	};

	TTest tt = { GetX(), 244 };

	return 0;
}

