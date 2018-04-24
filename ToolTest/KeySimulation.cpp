#include "stdafx.h"
#include "KeySimulation.h"


namespace key_simulation
{
	VOID SendAscii(wchar_t data, BOOL shift)
	{
		INPUT input[2];
		memset(input, 0, 2 * sizeof(INPUT));

		if (shift)
		{
			input[0].type = INPUT_KEYBOARD;
			input[0].ki.wVk = VK_SHIFT;
			SendInput(1, input, sizeof(INPUT));
		}
		input[0].type = INPUT_KEYBOARD;
		input[0].ki.wVk = data;
		input[1].type = INPUT_KEYBOARD;
		input[1].ki.wVk = data;
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(2, input, sizeof(INPUT));
		if (shift)
		{
			input[0].type = INPUT_KEYBOARD;
			input[0].ki.wVk = VK_SHIFT;
			input[0].ki.dwFlags = KEYEVENTF_KEYUP;
			SendInput(1, input, sizeof(INPUT));
		}
	}
}