#pragma once

#include <Windows.h>

#include <iostream>
#include "ConsoleType.h"


class WinConsoleDeviceAPI : public ConsoleDevice
{

public:
	virtual MouseEventInfo* GetMouseEvent()
	{
		return &m_MouseEventInfo;
	}

	virtual KeyBoardEventInfo* GetKeyboardEvent()
	{
		return &m_KeyBoardEventInfo;
	}

public:
	WinConsoleDeviceAPI()
	{
		m_hHwndConsole = GetConsoleWindow();
		m_hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		m_hConsoleInputHandle = GetStdHandle(STD_INPUT_HANDLE);
	}

	virtual void SetWindowPosition(const int xPos, const int yPos)
	{
		SetWindowPos(m_hHwndConsole, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
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

	virtual ConsoleDeviceEvent GetEvent()
	{
		DWORD events = 0;		// how many events took place
		DWORD input_size = 1;	// how many characters to read

		ConsoleDeviceEvent eEvent = ConsoleDeviceEvent::NONE_CONSOLE_EVENT;

		if (ReadConsoleInput(m_hConsoleInputHandle, &m_ipRecord, input_size, &events))
		{
			if (m_ipRecord.EventType & MOUSE_EVENT)
			{
				eEvent = ConsoleDeviceEvent::MOUSE_CONSOLE_EVENT;
				m_MouseEventInfo.m_MousePos = { m_ipRecord.Event.MouseEvent.dwMousePosition.X, m_ipRecord.Event.MouseEvent.dwMousePosition.Y };

				if (m_ipRecord.Event.MouseEvent.dwEventFlags & MOUSE_MOVED)
				{
					m_MouseEventInfo.m_MouseState = (m_MouseEventInfo.m_MouseState | ConsoleMouseState::MOUSE_MOVE_STATE);
				}
				else if (m_ipRecord.Event.MouseEvent.dwEventFlags == 0 && !(m_MouseEventInfo.m_MouseState & MOUSE_DOWN_STATE))
				{
					if (m_ipRecord.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						m_MouseEventInfo.m_MouseButton = ConsoleMouseButton::MOUSE_BUTTON_LEFT;
					}
					else if (m_ipRecord.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED)
					{
						m_MouseEventInfo.m_MouseButton = ConsoleMouseButton::MOUSE_BUTTON_RIGHT;
					}

					m_MouseEventInfo.m_MouseState = ConsoleMouseState::MOUSE_DOWN_STATE;
				}
				else
				{
					if (m_MouseEventInfo.m_MouseState & MOUSE_DOWN_STATE)
					{
						m_MouseEventInfo.m_MouseState = MOUSE_UP_STATE;
					}
					else
					{
						m_MouseEventInfo.m_MouseState = MOUSE_NONE_STATE;
					}
				}
			}
			else if (m_ipRecord.EventType & KEY_EVENT)
			{
				eEvent = ConsoleDeviceEvent::KEYBOARD_CONSOLE_EVENT;

				int nAsciiChar = static_cast<int>(m_ipRecord.Event.KeyEvent.uChar.AsciiChar);

				if (m_KeyBoardEventInfo.m_nState == ConsoleKeyboardState::KEYBOARD_DOWN_STATE &&
					m_ipRecord.Event.KeyEvent.bKeyDown == false)
				{
					m_KeyBoardEventInfo.m_nState = ConsoleKeyboardState::KEYBOARD_UP_STATE;
				}
				else
				{
					m_KeyBoardEventInfo.m_nState = (m_ipRecord.Event.KeyEvent.bKeyDown) ? ConsoleKeyboardState::KEYBOARD_DOWN_STATE :
						ConsoleKeyboardState::KEYBOARD_NONE_STATE;
				}

				m_KeyBoardEventInfo.m_nKey = nAsciiChar;
			}
		}
		return eEvent;
	}

	virtual void SetWindowCenter()
	{
		int xpos, ypos;
		RECT rectClient, rectWindow;
		GetClientRect(m_hHwndConsole, &rectClient);
		GetWindowRect(m_hHwndConsole, &rectWindow);
		
		xpos = GetSystemMetrics(SM_CXSCREEN) / 2 - (rectWindow.right - rectWindow.left) / 2;
		ypos = GetSystemMetrics(SM_CYSCREEN) / 2 - (rectWindow.bottom - rectWindow.top) / 2;

		SetWindowPosition(xpos, ypos);
	}

	virtual void SetTitle(const wchar_t* strTitle)
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

	virtual void SetFont(const wchar_t* strFontFamily)
	{
		CONSOLE_FONT_INFOEX fontInfo;
		fontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		::GetCurrentConsoleFontEx(m_hConsoleHandle, FALSE, &fontInfo);

		wcscpy_s(fontInfo.FaceName, strFontFamily);

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

	virtual void SetCellSize(const int nWidth, const int nHeight)
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

	virtual bool SetConsoleSize(const int nRow, const int nCol)
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
								if (coord.X < nCol || coord.Y < nRow)
								{
									if (coord.X < nCol)
										coord.X = nCol;

									if (coord.Y < nRow)
										coord.Y = nRow;

									if (!SetConsoleScreenBufferSize(m_hConsoleHandle, coord))
									{
										return FALSE;
									}
								}
								return SetWindowPos(m_hHwndConsole, NULL, rect.left, rect.top, nCol * fi.dwFontSize.X + bw,
									nRow * fi.dwFontSize.Y + bh, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
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

	MouseEventInfo		m_MouseEventInfo;
	KeyBoardEventInfo	m_KeyBoardEventInfo;
};
