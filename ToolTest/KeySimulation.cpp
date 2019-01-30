#include "stdafx.h"
#include "KeySimulation.h"
#include <cstdlib>
#include "ResManager.h"

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



	bool CmpColor(HWND hwnd, int x, int y, const   COLORREF& des_color, const COLORREF & sim)
	{
		RECT rc;
		::GetWindowRect(hwnd, &rc);

		x = rc.left + 340;
		y = rc.top + 350;


		HDC dc = ::GetDC(NULL);
		const COLORREF real_color = GetPixel(dc, x, y);
		::ReleaseDC(NULL, dc);
		BYTE real_r_color = GetRValue(real_color);
		BYTE real_g_color = GetGValue(real_color);
		BYTE real_b_color = GetBValue(real_color);

		BYTE r_color_cmp = GetRValue(des_color);
		BYTE g_color_cmp = GetGValue(des_color);
		BYTE b_color_cmp = GetBValue(des_color);

		BYTE r_color_sim = GetRValue(sim);
		BYTE g_color_sim = GetGValue(sim);
		BYTE b_color_sim = GetBValue(sim);

		int r_diff = abs((int)real_r_color - (int)r_color_cmp);
		int g_diff = abs((int)real_g_color - (int)g_color_cmp);
		int b_diff = abs((int)real_b_color - (int)b_color_cmp);
		return (r_diff < r_color_sim && g_diff < g_color_sim && b_diff < b_color_sim);
	}


#define INT_MIN     (-2147483647 - 1) // minimum (signed) int value
#define INT_MAX       2147483647    // maximum (signed) int value
#define COORD_UNSPECIFIED INT_MIN

#define VK_NEW_MOUSE_FIRST 0x9A
#define VK_LBUTTON_LOGICAL 0x9A // v1.0.43: Added to support swapping of left/right mouse buttons in Control Panel.
#define VK_RBUTTON_LOGICAL 0x9B //
#define VK_WHEEL_LEFT      0x9C // v1.0.48: Lexikos: Fake virtual keys for support for horizontal scrolling in
#define VK_WHEEL_RIGHT     0x9D // Windows Vista and later.
#define VK_WHEEL_DOWN      0x9E
#define VK_WHEEL_UP        0x9F
#define IS_WHEEL_VK(aVK) ((aVK) >= VK_WHEEL_LEFT && (aVK) <= VK_WHEEL_UP)
#define VK_NEW_MOUSE_LAST  0x9F

#define MAX_MOUSE_SPEED 100

	typedef UCHAR vk_type;  // Virtual key.
	typedef UCHAR SendLevelType;
	typedef USHORT CoordModeType;
	enum SendModes { SM_EVENT, SM_INPUT, SM_PLAY, SM_INPUT_FALLBACK_TO_PLAY, SM_INVALID }; // SM_EVENT must be zero.

	//全局变量
	static SendModes sSendMode = SM_EVENT;
	static int MouseDelay = 10;
	static SendLevelType SendLevel = 0;
	static CoordModeType CoordMode = 0;
#define KEY_IGNORE 0xFFC3D44F
#define KEY_PHYS_IGNORE (KEY_IGNORE - 1)  // Same as above but marked as physical for other instances of the hook.
#define KEY_IGNORE_ALL_EXCEPT_MODIFIER (KEY_IGNORE - 2)  // Non-physical and ignored only if it's not a modifier.

#define KEY_IGNORE_LEVEL(LEVEL) (KEY_IGNORE_ALL_EXCEPT_MODIFIER - LEVEL)
#define KEY_IGNORE_MIN KEY_IGNORE_LEVEL(SendLevelMax)
#define KEY_IGNORE_MAX KEY_IGNORE // There are two extra values above KEY_IGNORE_LEVEL(0)
	// This is used to generate an Alt key-up event for the purpose of changing system state, but having the hook
	void MouseEvent(DWORD aEventFlags, DWORD aData, DWORD aX, DWORD aY)
	{
		mouse_event(aEventFlags
			, aX == COORD_UNSPECIFIED ? 0 : aX // v1.0.43.01: Must be zero if no change in position is desired
			, aY == COORD_UNSPECIFIED ? 0 : aY // (fixes compatibility with certain apps/games).
			, aData, KEY_IGNORE_LEVEL(SendLevel));
	}

	void DoMouseDelay() // Helper function for the mouse functions below.
	{
		int mouse_delay = MouseDelay;
		if (mouse_delay < 0) // To support user-specified KeyDelay of -1 (fastest send rate).
			return;
		Sleep(mouse_delay);
	}

	void DoIncrementalMouseMove(int aX1, int aY1, int aX2, int aY2, int aSpeed)
		// aX1 and aY1 are the starting coordinates, and "2" are the destination coordinates.
		// Caller has ensured that aSpeed is in the range 0 to 100, inclusive.
	{
		// AutoIt3: So, it's a more gradual speed that is needed :)
		int delta;
#define INCR_MOUSE_MIN_SPEED 32

		while (aX1 != aX2 || aY1 != aY2)
		{
			if (aX1 < aX2)
			{
				delta = (aX2 - aX1) / aSpeed;
				if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
					delta = INCR_MOUSE_MIN_SPEED;
				if ((aX1 + delta) > aX2)
					aX1 = aX2;
				else
					aX1 += delta;
			}
			else
				if (aX1 > aX2)
				{
					delta = (aX1 - aX2) / aSpeed;
					if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
						delta = INCR_MOUSE_MIN_SPEED;
					if ((aX1 - delta) < aX2)
						aX1 = aX2;
					else
						aX1 -= delta;
				}

			if (aY1 < aY2)
			{
				delta = (aY2 - aY1) / aSpeed;
				if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
					delta = INCR_MOUSE_MIN_SPEED;
				if ((aY1 + delta) > aY2)
					aY1 = aY2;
				else
					aY1 += delta;
			}
			else
				if (aY1 > aY2)
				{
					delta = (aY1 - aY2) / aSpeed;
					if (delta == 0 || delta < INCR_MOUSE_MIN_SPEED)
						delta = INCR_MOUSE_MIN_SPEED;
					if ((aY1 - delta) < aY2)
						aY1 = aY2;
					else
						aY1 -= delta;
				}

			MouseEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, aX1, aY1);
			DoMouseDelay();
			// Above: A delay is required for backward compatibility and because it's just how the incremental-move
			// feature was originally designed in AutoIt v3.  It may in fact improve reliability in some cases,
			// especially with the mouse_event() method vs. SendInput/Play.
		} // while()
	}



	// Bit-field offsets:
#define COORD_MODE_PIXEL   0
#define COORD_MODE_MOUSE   2
#define COORD_MODE_TOOLTIP 4
#define COORD_MODE_CARET   6
#define COORD_MODE_MENU    8

#define COORD_MODE_WINDOW  0
#define COORD_MODE_CLIENT  1
#define COORD_MODE_SCREEN  2
#define COORD_MODE_MASK    3


#define COORD_CENTERED (INT_MIN + 1)
#define COORD_UNSPECIFIED INT_MIN
#define COORD_UNSPECIFIED_SHORT SHRT_MIN  // This essentially makes coord -32768 "reserved", but it seems acceptable given usefulness and the rarity of a real coord like that.



	void CoordToScreen(int &aX, int &aY, int aWhichMode)
		// aX and aY are interpreted according to the current coord mode.  If necessary, they are converted to
		// screen coordinates based on the position of the active window's upper-left corner (or its client area).
	{
		int coord_mode = ((CoordMode >> aWhichMode) & COORD_MODE_MASK);

		if (coord_mode == COORD_MODE_SCREEN)
			return;

		HWND active_window = GetForegroundWindow();
		if (active_window && !IsIconic(active_window))
		{
			if (coord_mode == COORD_MODE_WINDOW)
			{
				RECT rect;
				if (GetWindowRect(active_window, &rect))
				{
					aX += rect.left;
					aY += rect.top;
				}
			}
			else // (coord_mode == COORD_MODE_CLIENT)
			{
				POINT pt = { 0 };
				if (ClientToScreen(active_window, &pt))
				{
					aX += pt.x;
					aY += pt.y;
				}
			}
		}
	}

	void MouseMove(int &aX, int &aY, DWORD &aEventFlags, int aSpeed, bool aMoveOffset)
	{
		if (aX == COORD_UNSPECIFIED || aY == COORD_UNSPECIFIED)
			return;
		aEventFlags |= MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE; // Done here for caller, for easier maintenance.
		POINT cursor_pos;
		if (aMoveOffset)  // We're moving the mouse cursor relative to its current position.
		{
			GetCursorPos(&cursor_pos); // None of this is done for playback mode since that mode already returned higher above.
			aX += cursor_pos.x;
			aY += cursor_pos.y;
		}
		else
			CoordToScreen(aX, aY, COORD_MODE_MOUSE);

		int screen_width = GetSystemMetrics(SM_CXSCREEN);
		int screen_height = GetSystemMetrics(SM_CYSCREEN);
#define MOUSE_COORD_TO_ABS(coord, width_or_height) (((65536 * coord) / width_or_height) + (coord < 0 ? -1 : 1))
		aX = MOUSE_COORD_TO_ABS(aX, screen_width);
		aY = MOUSE_COORD_TO_ABS(aY, screen_height);

		if (aSpeed < 0)  // This can happen during script's runtime due to something like: MouseMove, X, Y, %VarContainingNegative%
			aSpeed = 0;  // 0 is the fastest.
		else
			if (aSpeed > MAX_MOUSE_SPEED)
				aSpeed = MAX_MOUSE_SPEED;

		if (aSpeed == 0) // Instantaneous move to destination coordinates with no incremental positions in between.
		{
			// See the comments in the playback-mode section at the top of this function for why SM_INPUT ignores aSpeed.
			MouseEvent(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, aX, aY);
			DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
			return;
		}

		GetCursorPos(&cursor_pos);
		DoIncrementalMouseMove(
			MOUSE_COORD_TO_ABS(cursor_pos.x, screen_width)  // Source/starting coords.
			, MOUSE_COORD_TO_ABS(cursor_pos.y, screen_height) //
			, aX, aY, aSpeed);
	}

	void MouseClickDrag(vk_type aVK, int aX1, int aY1, int aX2, int aY2, int aSpeed, bool aMoveOffset)
	{
		if ((aX1 == COORD_UNSPECIFIED && aY1 != COORD_UNSPECIFIED) || (aX1 != COORD_UNSPECIFIED && aY1 == COORD_UNSPECIFIED)
			|| (aX2 == COORD_UNSPECIFIED && aY2 != COORD_UNSPECIFIED) || (aX2 != COORD_UNSPECIFIED && aY2 == COORD_UNSPECIFIED))
			return;

		if (aVK == VK_LBUTTON_LOGICAL)
			aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
		else if (aVK == VK_RBUTTON_LOGICAL)
			aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON;

		DWORD event_down, event_up, event_flags = 0, event_data = 0; // Set defaults for some.
		switch (aVK)
		{
		case VK_LBUTTON:
			event_down = MOUSEEVENTF_LEFTDOWN;
			event_up = MOUSEEVENTF_LEFTUP;
			break;
		case VK_RBUTTON:
			event_down = MOUSEEVENTF_RIGHTDOWN;
			event_up = MOUSEEVENTF_RIGHTUP;
			break;
		case VK_MBUTTON:
			event_down = MOUSEEVENTF_MIDDLEDOWN;
			event_up = MOUSEEVENTF_MIDDLEUP;
			break;
		case VK_XBUTTON1:
		case VK_XBUTTON2:
			event_down = MOUSEEVENTF_XDOWN;
			event_up = MOUSEEVENTF_XUP;
			event_data = (aVK == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
			break;
		}

		if (aX1 != COORD_UNSPECIFIED && aY1 != COORD_UNSPECIFIED)
		{
			MouseMove(aX1, aY1, event_flags, aSpeed, aMoveOffset); // It calls DoMouseDelay() and also converts aX1 and aY1 to MOUSEEVENTF_ABSOLUTE coordinates.
		}

		MouseEvent(event_flags | event_down, event_data, aX1, aY1); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
		DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
		// Now that the mouse button has been pushed down, move the mouse to perform the drag:
		MouseMove(aX2, aY2, event_flags, aSpeed, aMoveOffset); // It calls DoMouseDelay() and also converts aX2 and aY2 to MOUSEEVENTF_ABSOLUTE coordinates.
		DoMouseDelay(); // Duplicate, see below.

		MouseEvent(event_flags | event_up, event_data, aX2, aY2); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
		DoMouseDelay();
	}



	bool CaptureImage(RECT rc, std::vector<BYTE> & data)
	{
		HDC hdcScreen = GetDC(NULL);
		if (!hdcScreen)
			return false;
		SetResDeleter(hdcScreen, [](HDC & hdc) {::ReleaseDC(NULL, hdc); });
		HDC hdcMemDC = CreateCompatibleDC(hdcScreen);
		if (!hdcMemDC)
			return false;
		SetResDeleter(hdcMemDC, [](HDC & hdc) {::DeleteDC(hdc); });

		HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, rc.right - rc.left, rc.bottom - rc.top);
		if (!hBitmap)
			return false;
		SetResDeleter(hBitmap, [](HBITMAP & h) {::DeleteObject(h); });
		SelectObject(hdcMemDC, hBitmap);//把hBitmap放进hdcMemDC
		//把屏幕复制到内存DC
		if (!BitBlt(hdcMemDC, 0, 0, rc.right - rc.left, rc.bottom - rc.top, hdcScreen, rc.left, rc.top, SRCCOPY))
			return false;

		// Get the BITMAP from the HBITMAP
		BITMAP bmpScreen;
		GetObject(hBitmap, sizeof(BITMAP), &bmpScreen);

		BITMAPFILEHEADER   bmfHeader;
		BITMAPINFOHEADER   bi;

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = bmpScreen.bmWidth;
		bi.biHeight = bmpScreen.bmHeight;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

		// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
		// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
		// have greater overhead than HeapAlloc.
		HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
		if (!hDIB)
			return false;
		SetResDeleter(hDIB, [](HANDLE & h) {::GlobalFree(h); });
		char *lpbitmap = (char *)GlobalLock(hDIB);

		// Gets the "bits" from the bitmap and copies them into a buffer 
		// which is pointed to by lpbitmap.
		GetDIBits(hdcMemDC, hBitmap, 0,
			(UINT)bmpScreen.bmHeight,
			lpbitmap,
			(BITMAPINFO *)&bi, DIB_RGB_COLORS);

		// A file is created, this is where we will save the screen capture.

		// Add the size of the headers to the size of the bitmap to get the total file size
		DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//Offset to where the actual bitmap bits start.
		bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

		//Size of the file
		bmfHeader.bfSize = dwSizeofDIB;

		//bfType must always be BM for Bitmaps
		bmfHeader.bfType = 0x4D42; //BM   

		DWORD dwBytesWritten = 0;
		//WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
		//WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
		//WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);


		data.resize(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize);
		BYTE * pWrite = data.data();
		memcpy(pWrite, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER)); pWrite += sizeof(BITMAPFILEHEADER);
		memcpy(pWrite, (LPSTR)&bi, sizeof(BITMAPINFOHEADER)); pWrite += sizeof(BITMAPINFOHEADER);
		memcpy(pWrite, (LPSTR)lpbitmap, dwBmpSize);

		GlobalUnlock(hDIB);
		return true;
	}


}