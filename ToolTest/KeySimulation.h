#pragma once
#include <Windows.h>
#include <vector>


namespace key_simulation
{
	typedef UCHAR vk_type;  // Virtual key.
	VOID SendAscii(wchar_t data, BOOL shift);
	void MouseClickDrag(vk_type aVK, int aX1, int aY1, int aX2, int aY2, int aSpeed, bool aMoveOffset);
	bool CmpColor(HWND hwnd, int x, int y, const   COLORREF& des_color, const COLORREF & sim);
	bool CaptureImage(RECT rc, std::vector<BYTE> & data);
}