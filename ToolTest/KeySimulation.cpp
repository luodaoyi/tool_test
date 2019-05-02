
#include "KeySimulation.h"
#include <cstdlib>
#include <ProcessLib/Common/ResHandleManager.h>
#pragma comment(lib,"Gdi32.lib")

#pragma warning(disable : 4706)

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
	static int MouseDelay = 3;
	static SendLevelType SendLevel = 0;
	static CoordModeType CoordMode = 0;
	static UINT sEventCount = 0; 
	static UINT sMaxEvents = 10; // Number of items in the above arrays and the current array capacity.
#define KEY_IGNORE 0xFFC3D44F
#define KEY_PHYS_IGNORE (KEY_IGNORE - 1)  // Same as above but marked as physical for other instances of the hook.
#define KEY_IGNORE_ALL_EXCEPT_MODIFIER (KEY_IGNORE - 2)  // Non-physical and ignored only if it's not a modifier.

#define KEY_IGNORE_LEVEL(LEVEL) (KEY_IGNORE_ALL_EXCEPT_MODIFIER - LEVEL)
#define KEY_IGNORE_MIN KEY_IGNORE_LEVEL(SendLevelMax)
#define KEY_IGNORE_MAX KEY_IGNORE // There are two extra values above KEY_IGNORE_LEVEL(0)
	// This is used to generate an Alt key-up event for the purpose of changing system state, but having the hook


	HWND GetNonChildParent(HWND aWnd)
		// Returns the first ancestor of aWnd that isn't itself a child.  aWnd itself is returned if
		// it is not a child.  Returns NULL only if aWnd is NULL.  Also, it should always succeed
		// based on the axiom that any window with the WS_CHILD style (aka WS_CHILDWINDOW) must have
		// a non-child ancestor somewhere up the line.
		// This function doesn't do anything special with owned vs. unowned windows.  Despite what MSDN
		// says, GetParent() does not return the owner window, at least in some cases on Windows XP
		// (e.g. BulletProof FTP Server). It returns NULL instead. In any case, it seems best not to
		// worry about owner windows for this function's caller (MouseGetPos()), since it might be
		// desirable for that command to return the owner window even though it can't actually be
		// activated.  This is because attempts to activate an owner window should automatically cause
		// the OS to activate the topmost owned window instead.  In addition, the owner window may
		// contain the actual title or text that the user is interested in.  UPDATE: Due to the fact
		// that this function retrieves the first parent that's not a child window, it's likely that
		// that window isn't its owner anyway (since the owner problem usually applies to a parent
		// window being owned by some controlling window behind it).
	{
		if (!aWnd) return aWnd;
		HWND parent, parent_prev;
		for (parent_prev = aWnd; ; parent_prev = parent)
		{
			if (!(GetWindowLong(parent_prev, GWL_STYLE) & WS_CHILD))  // Found the first non-child parent, so return it.
				return parent_prev;
			// Because Windows 95 doesn't support GetAncestor(), we'll use GetParent() instead:
			if (!(parent = GetParent(parent_prev)))
				return parent_prev;  // This will return aWnd if aWnd has no parents.
		}
	}



	
	void MouseEvent(DWORD aEventFlags, DWORD aData, DWORD aX, DWORD aY)
	{
		mouse_event(aEventFlags
			, aX == COORD_UNSPECIFIED ? 0 : aX // v1.0.43.01: Must be zero if no change in position is desired
			, aY == COORD_UNSPECIFIED ? 0 : aY // (fixes compatibility with certain apps/games).
			, aData, KEY_IGNORE_LEVEL(SendLevel));
		//LOGW(notice) << std::hex << aEventFlags << L"|" << aX << L"|" << aY << L"|" << aData << L"|" << KEY_IGNORE_LEVEL(SendLevel);
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




	void MouseClick(vk_type aVK, int aX, int aY, int aRepeatCount, int aSpeed, KeyEventTypes aEventType
		, bool aMoveOffset)
	{
		// Check if one of the coordinates is missing, which can happen in cases where this was called from
		// a source that didn't already validate it (such as MouseClick, %x%, %BlankVar%).
		// Allow aRepeatCount<1 to simply "do nothing", because it increases flexibility in the case where
		// the number of clicks is a dereferenced script variable that may sometimes (by intent) resolve to
		// zero or negative.  For backward compatibility, a RepeatCount <1 does not move the mouse (unlike
		// the Click command and Send {Click}).
		if ((aX == COORD_UNSPECIFIED && aY != COORD_UNSPECIFIED) || (aX != COORD_UNSPECIFIED && aY == COORD_UNSPECIFIED)
			|| (aRepeatCount < 1))
			return;

		DWORD event_flags = 0; // Set default.

		if (!(aX == COORD_UNSPECIFIED || aY == COORD_UNSPECIFIED)) // Both coordinates were specified.
		{
			// The movement must be a separate event from the click, otherwise it's completely unreliable with
			// SendInput() and probably keybd_event() too.  SendPlay is unknown, but it seems best for
			// compatibility and peace-of-mind to do it for that too.  For example, some apps may be designed
			// to expect mouse movement prior to a click at a *new* position, which is not unreasonable given
			// that this would be the case 99.999% of the time if the user were moving the mouse physically.
			MouseMove(aX, aY, event_flags, aSpeed, aMoveOffset); // It calls DoMouseDelay() and also converts aX and aY to MOUSEEVENTF_ABSOLUTE coordinates.
			// v1.0.43: event_flags was added to improve reliability.  Explanation: Since the mouse was just moved to an
			// explicitly specified set of coordinates, use those coordinates with subsequent clicks.  This has been
			// shown to significantly improve reliability in cases where the user is moving the mouse during the
			// MouseClick/Drag commands.
		}
		// Above must be done prior to below because initial mouse-move is supported even for wheel turning.

		// For wheel turning, if the user activated this command via a hotkey, and that hotkey
		// has a modifier such as CTRL, the user is probably still holding down the CTRL key
		// at this point.  Therefore, there's some merit to the fact that we should release
		// those modifier keys prior to turning the mouse wheel (since some apps disable the
		// wheel or give it different behavior when the CTRL key is down -- for example, MSIE
		// changes the font size when you use the wheel while CTRL is down).  However, if that
		// were to be done, there would be no way to ever hold down the CTRL key explicitly
		// (via Send, {CtrlDown}) unless the hook were installed.  The same argument could probably
		// be made for mouse button clicks: modifier keys can often affect their behavior.  But
		// changing this function to adjust modifiers for all types of events would probably break
		// some existing scripts.  Maybe it can be a script option in the future.  In the meantime,
		// it seems best not to adjust the modifiers for any mouse events and just document that
		// behavior in the MouseClick command.
		switch (aVK)
		{
		case VK_WHEEL_UP:
			MouseEvent(event_flags | MOUSEEVENTF_WHEEL, aRepeatCount * WHEEL_DELTA, aX, aY);  // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
			return;
		case VK_WHEEL_DOWN:
			MouseEvent(event_flags | MOUSEEVENTF_WHEEL, -(aRepeatCount * WHEEL_DELTA), aX, aY);
			return;
			// v1.0.48: Lexikos: Support horizontal scrolling in Windows Vista and later.
		case VK_WHEEL_LEFT:
			MouseEvent(event_flags | MOUSEEVENTF_HWHEEL, -(aRepeatCount * WHEEL_DELTA), aX, aY);
			return;
		case VK_WHEEL_RIGHT:
			MouseEvent(event_flags | MOUSEEVENTF_HWHEEL, aRepeatCount * WHEEL_DELTA, aX, aY);
			return;
		}
		// Since above didn't return:

		// Although not thread-safe, the following static vars seem okay because:
		// 1) This function is currently only called by the main thread.
		// 2) Even if that isn't true, the serialized nature of simulated mouse clicks makes it likely that
		//    the statics will produce the correct behavior anyway.
		// 3) Even if that isn't true, the consequences of incorrect behavior seem minimal in this case.
		static vk_type sWorkaroundVK = 0;
		static LRESULT sWorkaroundHitTest; // Not initialized because the above will be the sole signal of whether the workaround is in progress.
		DWORD event_down = 0, event_up = 0, event_data = 0; // Set default.
		// MSDN: If [event_flags] is not MOUSEEVENTF_WHEEL, MOUSEEVENTF_XDOWN, or MOUSEEVENTF_XUP, then [event_data]
		// should be zero. 

		// v1.0.43: Translate logical buttons into physical ones.  Which physical button it becomes depends
		// on whether the mouse buttons are swapped via the Control Panel.
		if (aVK == VK_LBUTTON_LOGICAL)
			aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON;
		else if (aVK == VK_RBUTTON_LOGICAL)
			aVK = sSendMode != SM_PLAY && GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON;

		switch (aVK)
		{
		case VK_LBUTTON:
		case VK_RBUTTON:
			// v1.0.43 The first line below means: We're not in SendInput/Play mode or we are but this
			// will be the first event inside the array.  The latter case also implies that no initial
			// mouse-move was done above (otherwise there would already be a MouseMove event in the array,
			// and thus the click here wouldn't be the first item).  It doesn't seem necessary to support
			// the MouseMove case above because the workaround generally isn't needed in such situations
			// (see detailed comments below).  Furthermore, if the MouseMove were supported in array-mode,
			// it would require that GetCursorPos() below be conditionally replaced with something like
			// the following (since when in array-mode, the cursor hasn't actually moved *yet*):
			//		CoordToScreen(aX_orig, aY_orig, COORD_MODE_MOUSE);  // Moving mouse relative to the active window.
			// Known limitation: the work-around described below isn't as complete for SendPlay as it is
			// for the other modes: because dragging the title bar of one of this thread's windows with a
			// remap such as F1::LButton doesn't work if that remap uses SendPlay internally (the window
			// gets stuck to the mouse cursor).
			if ((!sSendMode || !sEventCount) // See above.
				&& (aEventType == KEYDOWN || (aEventType == KEYUP && sWorkaroundVK))) // i.e. this is a down-only event or up-only event.
			{
				// v1.0.40.01: The following section corrects misbehavior caused by a thread sending
				// simulated mouse clicks to one of its own windows.  A script consisting only of the
				// following two lines can reproduce this issue:
				// F1::LButton
				// F2::RButton
				// The problems came about from the following sequence of events:
				// 1) Script simulates a left-click-down in the title bar's close, minimize, or maximize button.
				// 2) WM_NCLBUTTONDOWN is sent to the window's window proc, which then passes it on to
				//    DefWindowProc or DefDlgProc, which then apparently enters a loop in which no messages
				//    (or a very limited subset) are pumped.
				// 3) Thus, if the user presses a hotkey while the thread is in this state, that hotkey is
				//    queued/buffered until DefWindowProc/DefDlgProc exits its loop.
				// 4) But the buffered hotkey is the very thing that's supposed to exit the loop via sending a
				//    simulated left-click-up event.
				// 5) Thus, a deadlock occurs.
				// 6) A similar situation arises when a right-click-down is sent to the title bar or sys-menu-icon.
				//
				// The following workaround operates by suppressing qualified click-down events until the
				// corresponding click-up occurs, at which time the click-up is transformed into a down+up if the
				// click-up is still in the same cursor position as the down. It seems preferable to fix this here
				// rather than changing each window proc. to always respond to click-down rather vs. click-up
				// because that would make all of the script's windows behave in a non-standard way, possibly
				// producing side-effects and defeating other programs' attempts to interact with them.
				// (Thanks to Shimanov for this solution.)
				//
				// Remaining known limitations:
				// 1) Title bar buttons are not visibly in a pressed down state when a simulated click-down is sent
				//    to them.
				// 2) A window that should not be activated, such as AlwaysOnTop+Disabled, is activated anyway
				//    by SetForegroundWindowEx().  Not yet fixed due to its rarity and minimal consequences.
				// 3) A related problem for which no solution has been discovered (and perhaps it's too obscure
				//    an issue to justify any added code size): If a remapping such as "F1::LButton" is in effect,
				//    pressing and releasing F1 while the cursor is over a script window's title bar will cause the
				//    window to move slightly the next time the mouse is moved.
				// 4) Clicking one of the script's window's title bar with a key/button that has been remapped to
				//    become the left mouse button sometimes causes the button to get stuck down from the window's
				//    point of view.  The reasons are related to those in #1 above.  In both #1 and #2, the workaround
				//    is not at fault because it's not in effect then.  Instead, the issue is that DefWindowProc enters
				//    a non-msg-pumping loop while it waits for the user to drag-move the window.  If instead the user
				//    releases the button without dragging, the loop exits on its own after a 500ms delay or so.
				// 5) Obscure behavior caused by keyboard's auto-repeat feature: Use a key that's been remapped to
				//    become the left mouse button to click and hold the minimize button of one of the script's windows.
				//    Drag to the left.  The window starts moving.  This is caused by the fact that the down-click is
				//    suppressed, thus the remap's hotkey subroutine thinks the mouse button is down, thus its
				//    auto-repeat suppression doesn't work and it sends another click.
				POINT point;
				GetCursorPos(&point); // Assuming success seems harmless.
				// Despite what MSDN says, WindowFromPoint() appears to fetch a non-NULL value even when the
				// mouse is hovering over a disabled control (at least on XP).
				HWND child_under_cursor, parent_under_cursor;
				if ((child_under_cursor = WindowFromPoint(point))
					&& (parent_under_cursor = GetNonChildParent(child_under_cursor)) // WM_NCHITTEST below probably requires parent vs. child.
					&& GetWindowThreadProcessId(parent_under_cursor, NULL) == GetCurrentThreadId()) // It's one of our thread's windows.
				{
					LRESULT hit_test = SendMessage(parent_under_cursor, WM_NCHITTEST, 0, MAKELPARAM(point.x, point.y));
					if (aVK == VK_LBUTTON && (hit_test == HTCLOSE || hit_test == HTMAXBUTTON // Title bar buttons: Close, Maximize.
						|| hit_test == HTMINBUTTON || hit_test == HTHELP) // Title bar buttons: Minimize, Help.
						|| aVK == VK_RBUTTON && (hit_test == HTCAPTION || hit_test == HTSYSMENU))
					{
						if (aEventType == KEYDOWN)
						{
							// Ignore this event and substitute for it: Activate the window when one
							// of its title bar buttons is down-clicked.
							sWorkaroundVK = aVK;
							sWorkaroundHitTest = hit_test;
							SetForegroundWindow(parent_under_cursor); // Try to reproduce customary behavior.
							// For simplicity, aRepeatCount>1 is ignored and DoMouseDelay() is not done.
							return;
						}
						else // KEYUP
						{
							if (sWorkaroundHitTest == hit_test) // To weed out cases where user clicked down on a button then released somewhere other than the button.
								aEventType = KEYDOWNANDUP; // Translate this click-up into down+up to make up for the fact that the down was previously suppressed.
							//else let the click-up occur in case it does something or user wants it.
						}
					}
				} // Work-around for sending mouse clicks to one of our thread's own windows.
			}
			// sWorkaroundVK is reset later below.

			// Since above didn't return, the work-around isn't in effect and normal click(s) will be sent:
			if (aVK == VK_LBUTTON)
			{
				event_down = MOUSEEVENTF_LEFTDOWN;
				event_up = MOUSEEVENTF_LEFTUP;
			}
			else // aVK == VK_RBUTTON
			{
				event_down = MOUSEEVENTF_RIGHTDOWN;
				event_up = MOUSEEVENTF_RIGHTUP;
			}
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
		} // switch()

		// For simplicity and possibly backward compatibility, LONG_OPERATION_INIT/UPDATE isn't done.
		// In addition, some callers might do it for themselves, at least when aRepeatCount==1.
		for (int i = 0; i < aRepeatCount; ++i)
		{
			if (aEventType != KEYUP) // It's either KEYDOWN or KEYDOWNANDUP.
			{
				// v1.0.43: Reliability is significantly improved by specifying the coordinates with the event (if
				// caller gave us coordinates).  This is mostly because of SetMouseDelay: In previously versions,
				// the delay between a MouseClick's move and its click allowed time for the user to move the mouse
				// away from the target position before the click was sent.
				MouseEvent(event_flags | event_down, event_data, aX, aY); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
				// It seems best to always Sleep a certain minimum time between events
				// because the click-down event may cause the target app to do something which
				// changes the context or nature of the click-up event.  AutoIt3 has also been
				// revised to do this. v1.0.40.02: Avoid doing the Sleep between the down and up
				// events when the workaround is in effect because any MouseDelay greater than 10
				// would cause DoMouseDelay() to pump messages, which would defeat the workaround:
				if (!sWorkaroundVK)
					DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
			}
			if (aEventType != KEYDOWN) // It's either KEYUP or KEYDOWNANDUP.
			{
				MouseEvent(event_flags | event_up, event_data, aX, aY); // It ignores aX and aY when MOUSEEVENTF_MOVE is absent.
				// It seems best to always do this one too in case the script line that caused
				// us to be called here is followed immediately by another script line which
				// is either another mouse click or something that relies upon the mouse click
				// having been completed:
				DoMouseDelay(); // Inserts delay for all modes except SendInput, for which it does nothing.
			}
		} // for()

		sWorkaroundVK = 0; // Reset this indicator in all cases except those for which above already returned.
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

		//DWORD dwBytesWritten = 0;
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

	void SetForegroundWindowInternal1(HWND hWnd)
	{
		if (!::IsWindow(hWnd)) return;

		BYTE keyState[256] = { 0 };
		//to unlock SetForegroundWindow we need to imitate Alt pressing
		if (::GetKeyboardState((LPBYTE)&keyState))
		{
			if (!(keyState[VK_MENU] & 0x80))
			{
				::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | 0, 0);
			}
		}

		::SetForegroundWindow(hWnd);

		if (::GetKeyboardState((LPBYTE)&keyState))
		{
			if (!(keyState[VK_MENU] & 0x80))
			{
				::keybd_event(VK_MENU, 0, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
			}
		}
	}


	void SetForegroundWindowInternal2(HWND hWnd)
	{
		if (!::IsWindow(hWnd)) return;

		//relation time of SetForegroundWindow lock
		DWORD lockTimeOut = 0;
		HWND  hCurrWnd = ::GetForegroundWindow();
		DWORD dwThisTID = ::GetCurrentThreadId(),
			dwCurrTID = ::GetWindowThreadProcessId(hCurrWnd, 0);

		//we need to bypass some limitations from Microsoft :)
		if (dwThisTID != dwCurrTID)
		{
			::AttachThreadInput(dwThisTID, dwCurrTID, TRUE);

			::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut, 0);
			::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);

			::AllowSetForegroundWindow(ASFW_ANY);
		}

		::SetForegroundWindow(hWnd);

		if (dwThisTID != dwCurrTID)
		{
			::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)lockTimeOut, SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
			::AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
		}
	}
}