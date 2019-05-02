#pragma once
#include <Windows.h>
#include <vector>


namespace key_simulation
{
	typedef UCHAR vk_type;  // Virtual key.
#define COORD_UNSPECIFIED INT_MIN
	VOID SendAscii(wchar_t data, BOOL shift);
	void MouseClickDrag(vk_type aVK, int aX1, int aY1, int aX2, int aY2, int aSpeed, bool aMoveOffset);
	enum KeyEventTypes { KEYDOWN, KEYUP, KEYDOWNANDUP };
	void MouseMove(int &aX, int &aY, DWORD &aEventFlags, int aSpeed, bool aMoveOffset);
	void MouseClick(vk_type aVK // Which button.
		, int aX, int aY, int aRepeatCount, int aSpeed, KeyEventTypes aEventType, bool aMoveOffset = false);
	bool CmpColor(HWND hwnd, int x, int y, const   COLORREF& des_color, const COLORREF & sim);
	bool CaptureImage(RECT rc, std::vector<BYTE> & data);
	void SetForegroundWindowInternal1(HWND hWnd);
	void SetForegroundWindowInternal2(HWND hWnd);
}