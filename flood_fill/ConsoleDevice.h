#pragma once

#include <Windows.h>
#include <assert.h>
#include <iostream>

// Define macro over assert
#define assert_msg_p(cond, fmt, ...) assert(cond || !fprintf_s(stderr, fmt, ##__VA_ARGS__))
#define assert_msg(cond, fmt) assert(cond && fmt)

#ifdef _UNICODE
#define _T(pw) L ##pw
#else
#define _T(pw) pw
#endif

enum ConsoleDeviceEvent
{
	MOUSE_CONSOLE_EVENT,
	KEYBOARD_CONSOLE_EVENT,
	NONE_CONSOLE_EVENT,
};

enum ConsoleMouseState
{
	MOUSE_NONE_STATE = 0x0000,
	MOUSE_MOVE_STATE = 0x0001,
	MOUSE_UP_STATE   = 0x0002,
	MOUSE_DOWN_STATE = 0x0004,
};

enum ConsoleMouseButton
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT
};

struct ConsoleMousePos
{
	int x;
	int y;
};

struct MouseEventInfo
{
	ConsoleMouseButton	m_MouseButton;
	DWORD				m_MouseState = 0;
	ConsoleMousePos		m_MousePos;
};


class ConsoleDevice
{
public:
	MouseEventInfo* GetMouseEvent()
	{
		return &m_MouseInfo;
	}

public:
	ConsoleDevice()
	{
		m_hHwndConsole = GetConsoleWindow();
		m_hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hConsoleInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	}

	void SetWindowPosition(int xpos, int ypos)
	{
		SetWindowPos(m_hHwndConsole, NULL, xpos, ypos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

	bool PoolEvent()
	{
		DWORD numEvents = 0;

		if (GetNumberOfConsoleInputEvents(m_hConsoleInputHandle, &numEvents))
		{
			if(numEvents > 0)
				return true;
		}

		return false;
	}

	ConsoleDeviceEvent GetEvent()
	{
		DWORD events = 0;		// how many events took place
		DWORD input_size = 1;	// how many characters to read

		ConsoleDeviceEvent eEvent = ConsoleDeviceEvent::NONE_CONSOLE_EVENT;

		if (ReadConsoleInput(m_hConsoleInputHandle, &m_ipRecord, input_size, &events))
		{
			if (m_ipRecord.EventType & MOUSE_EVENT)
			{
				eEvent = ConsoleDeviceEvent::MOUSE_CONSOLE_EVENT;
				m_MouseInfo.m_MousePos = { m_ipRecord.Event.MouseEvent.dwMousePosition.X, m_ipRecord.Event.MouseEvent.dwMousePosition.Y };

				if (m_ipRecord.Event.MouseEvent.dwEventFlags & MOUSE_MOVED)
				{
					m_MouseInfo.m_MouseState = (m_MouseInfo.m_MouseState | ConsoleMouseState::MOUSE_MOVE_STATE);
				}
				else if (m_ipRecord.Event.MouseEvent.dwEventFlags == 0 && !(m_MouseInfo.m_MouseState & MOUSE_DOWN_STATE))
				{
					if (m_ipRecord.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						m_MouseInfo.m_MouseButton = ConsoleMouseButton::MOUSE_BUTTON_LEFT;
					}
					else if (m_ipRecord.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
					{
						m_MouseInfo.m_MouseButton = ConsoleMouseButton::MOUSE_BUTTON_RIGHT;
					}

					m_MouseInfo.m_MouseState = ConsoleMouseState::MOUSE_DOWN_STATE;
				}
				else
				{
					if (m_MouseInfo.m_MouseState & MOUSE_DOWN_STATE)
					{
						m_MouseInfo.m_MouseState = MOUSE_UP_STATE;
					}
					else
					{
						m_MouseInfo.m_MouseState = MOUSE_NONE_STATE;
					}
				}
			}
			else if (m_ipRecord.EventType & KEY_EVENT)
			{
				eEvent = ConsoleDeviceEvent::KEYBOARD_CONSOLE_EVENT;
			}
		}
		return eEvent;
	}

	void SetWindowCenter()
	{
		int xpos, ypos;
		RECT rectClient, rectWindow;
		GetClientRect(m_hHwndConsole, &rectClient);
		GetWindowRect(m_hHwndConsole, &rectWindow);
		
		xpos = GetSystemMetrics(SM_CXSCREEN) / 2 - (rectWindow.right - rectWindow.left) / 2;
		ypos = GetSystemMetrics(SM_CYSCREEN) / 2 - (rectWindow.bottom - rectWindow.top) / 2;

		SetWindowPosition(xpos, ypos);
	}

	void SetTitle(const wchar_t* strTitle)
	{
		SetConsoleTitle(strTitle);
	}

	void HideScollBar()
	{
		// retrieve screen buffer info
		CONSOLE_SCREEN_BUFFER_INFO scrBufferInfo;
		GetConsoleScreenBufferInfo(m_hConsoleHandle, &scrBufferInfo);

		// current window size
		short winWidth = scrBufferInfo.srWindow.Right - scrBufferInfo.srWindow.Left + 1;
		short winHeight = scrBufferInfo.srWindow.Bottom - scrBufferInfo.srWindow.Top + 1;

		// current screen buffer size
		short scrBufferWidth = scrBufferInfo.dwSize.X;
		short scrBufferHeight = scrBufferInfo.dwSize.Y;

		// to remove the scrollbar, make sure the window height matches the screen buffer height
		COORD newSize;
		newSize.X = scrBufferWidth;
		newSize.Y = winHeight;

		// set the new screen buffer dimensions
		if (!SetConsoleScreenBufferSize(m_hConsoleHandle, newSize))
		{
			assert_msg_p(0, "SetConsoleScreenBufferSize() failed!Reason : %d", GetLastError());
		}

		RECT rectClient, rectWindow;
		GetWindowRect(m_hHwndConsole, &rectWindow);

		SetWindowPos(m_hHwndConsole, NULL, 0, 0, rectWindow.right - rectWindow.left + 1, rectWindow.bottom - rectWindow.top, SWP_NOMOVE | SWP_NOZORDER);
	}

	void SetFont(const wchar_t* strFont)
	{
		CONSOLE_FONT_INFOEX fontInfo;
		fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		::GetCurrentConsoleFontEx(m_hConsoleHandle, FALSE, &fontInfo);

		wcscpy_s(fontInfo.FaceName, L"Arial");

		if (!SetCurrentConsoleFontEx(m_hConsoleHandle, FALSE, &fontInfo))
		{
			assert_msg_p(0, "Failed to change the font LastError=%d", GetLastError());
		}
	}

	void ShowHideCursor(DWORD bShow)
	{
		CONSOLE_CURSOR_INFO cursorInfo;

		GetConsoleCursorInfo(m_hConsoleHandle, &cursorInfo);
		cursorInfo.bVisible = bShow;
		SetConsoleCursorInfo(m_hConsoleHandle, &cursorInfo);

		DWORD dw;
		auto pipe = !GetConsoleMode(m_hConsoleInputHandle, &dw);
		if (!pipe) {
			SetConsoleMode(m_hConsoleInputHandle, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS);
		}

		FlushConsoleInputBuffer(m_hConsoleInputHandle);
	}

	void SetSingleSelection()
	{
		CONSOLE_SELECTION_INFO selectionInf;
		GetConsoleSelectionInfo(&selectionInf);
	}

	void SetCellSize(const int nWidth, const int nHeight)
	{
		CONSOLE_FONT_INFOEX fontInfo;
		fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		fontInfo.dwFontSize.X = nWidth;
		fontInfo.dwFontSize.Y = nHeight;
		fontInfo.FontFamily = TMPF_FIXED_PITCH;
		wcscpy_s(fontInfo.FaceName, L"Arial");

		if (!SetCurrentConsoleFontEx(m_hConsoleHandle, FALSE, &fontInfo))
		{
			assert_msg(0, "Failed to change the font");
		}
	}

	BOOL SetConsoleSize(int nCols, int nRows)
	{
		CONSOLE_FONT_INFO fi;
		CONSOLE_SCREEN_BUFFER_INFO bi;
		int w, h, bw, bh;
		RECT rect = { 0, 0, 0, 0 };
		COORD coord = { 0, 0 };
		if (m_hHwndConsole)
		{
			if (m_hConsoleHandle && m_hConsoleHandle != (HANDLE)-1)
			{
				if (GetCurrentConsoleFont(m_hConsoleHandle, FALSE, &fi))
				{
					if (GetClientRect(m_hHwndConsole, &rect))
					{
						w = rect.right - rect.left;
						h = rect.bottom - rect.top;
						if (GetWindowRect(m_hHwndConsole, &rect))
						{
							bw = rect.right - rect.left - w;
							bh = rect.bottom - rect.top - h;
							if (GetConsoleScreenBufferInfo(m_hConsoleHandle, &bi))
							{
								coord.X = bi.dwSize.X;
								coord.Y = bi.dwSize.Y;
								if (coord.X < nCols || coord.Y < nRows)
								{
									if (coord.X < nCols)
										coord.X = nCols;

									if (coord.Y < nRows)
										coord.Y = nRows;

									if (!SetConsoleScreenBufferSize(m_hConsoleHandle, coord))
									{
										return FALSE;
									}
								}
								return SetWindowPos(m_hHwndConsole, NULL, rect.left, rect.top, nCols * fi.dwFontSize.X + bw,
									nRows * fi.dwFontSize.Y + bh, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
							}
						}
					}
				}
			}
		}
		return FALSE;
	}

protected:
	HWND			m_hHwndConsole;
	HANDLE			m_hConsoleHandle;
	HANDLE			m_hConsoleInputHandle;
	INPUT_RECORD	m_ipRecord;

	MouseEventInfo	m_MouseInfo;
};
