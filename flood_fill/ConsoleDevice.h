#pragma once

#include <Windows.h>
#include <map>

#include <iostream>
#include "ConsoleType.h"


class WinConsoleDeviceAPI : public ConsoleDevice, public ConsoleDeviecDrawable
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

protected:
	void UpdateScreenInfo()
	{
		m_ScreenInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		GetConsoleScreenBufferInfoEx(m_hConsoleHandle, &m_ScreenInfo);
	}

	unsigned char GetColorCode(unsigned char colorBackground, unsigned char colorForeground)
	{
		//return most signifigant bit of colorBackground and
		//least signifigant bit of colorForground as one byte
		return (colorBackground << 4) + colorForeground;
	}

	void UpdateColor(unsigned char nColCode)
	{
		//set front and back colors
		if (!SetConsoleTextAttribute(m_hConsoleHandle, nColCode))
		{
			assert_msg_p(0, "Failed to change the font LastError=%d", GetLastError());
		}
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

				if (m_MouseEventInfo.m_MouseState & ConsoleMouseState::MOUSE_MOVE_STATE)
				{
					if (m_MouseEventInfo.m_MousePos.x == m_ipRecord.Event.MouseEvent.dwMousePosition.X &&
						m_MouseEventInfo.m_MousePos.y == m_ipRecord.Event.MouseEvent.dwMousePosition.Y)
					{
						m_MouseEventInfo.m_MouseState &= ~ConsoleMouseState::MOUSE_MOVE_STATE;

						m_MouseEventInfo.m_MouseState |= ConsoleMouseState::MOUSE_MOVE_NO_OVER_STATE;
					}
				}

				m_MouseEventInfo.m_MousePos = { m_ipRecord.Event.MouseEvent.dwMousePosition.X, m_ipRecord.Event.MouseEvent.dwMousePosition.Y };
			}
			else if (m_ipRecord.EventType & KEY_EVENT)
			{
				eEvent = ConsoleDeviceEvent::KEYBOARD_CONSOLE_EVENT;

				int nAsciiChar = static_cast<int>(m_ipRecord.Event.KeyEvent.uChar.AsciiChar);

				if (m_KeyBoardTracker[nAsciiChar] == ConsoleKeyboardState::KEYBOARD_DOWN_STATE &&
					m_ipRecord.Event.KeyEvent.bKeyDown == false)
				{
					m_KeyBoardTracker[nAsciiChar] = ConsoleKeyboardState::KEYBOARD_UP_STATE;
					m_KeyBoardEventInfo.m_nState = ConsoleKeyboardState::KEYBOARD_UP_STATE;
				}
				else
				{
					m_KeyBoardTracker[nAsciiChar] = (m_ipRecord.Event.KeyEvent.bKeyDown) ? ConsoleKeyboardState::KEYBOARD_DOWN_STATE :
						ConsoleKeyboardState::KEYBOARD_NONE_STATE;

					m_KeyBoardEventInfo.m_nState = m_KeyBoardTracker[nAsciiChar];
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

		UpdateScreenInfo();
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

	virtual bool SetConsoleSize(const int nCol, const int nRow)
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

									UpdateScreenInfo();
								}

								int cx = nCol * fi.dwFontSize.X + bw * 2;
								int cy = nRow * fi.dwFontSize.Y + bh;

								return SetWindowPos(m_hHwndConsole, NULL, rect.left, rect.top, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
							}
						}
					}
				}
			}
		}
		return FALSE;
	}

	//function 

public:
	virtual void SetXY(const int x, const int y)
	{
		COORD c = { x,y };
		SetConsoleCursorPosition(m_hConsoleHandle, c);
	}

	virtual void SetTextColor(ConsoleColors col)
	{
		if (m_colText == col)
			return;

		m_colText = col;

		unsigned char nCode = GetColorCode(m_colBkg, m_colText);

		UpdateColor(nCode);
	}

	virtual void SetBackgroundColor(ConsoleColors col)
	{
		if (m_colBkg == col)
			return;

		if (col == DEFAULT)
		{
			m_colBkg = m_colBkgScreen;
		}
		else
		{
			m_colBkg = col;
		}

		unsigned char nCode = GetColorCode(m_colBkg, m_colText);

		UpdateColor(nCode);
	}

	virtual void SetDrawText(const wchar_t* strText)
	{
		std::wcout << strText;
	}

	virtual void ClearColorInfo()
	{

	}

	virtual void SetBackgroundScreen(ConsoleColors col)
	{

	}

	virtual void SetClearColor(ConsoleColors col = DEFAULT)
	{
		m_colBkgScreen = col;
		m_colBkg = m_colBkgScreen;
	}

	virtual void Clear()
	{
		WORD wColor = BLACK;
		wColor = ((m_colBkgScreen & 0x0F) << 4) + (m_colText & 0x0F);

		SetConsoleTextAttribute(m_hConsoleHandle, wColor);

		COORD coordScreen = { 0, 0 };
		DWORD cCharsWritten;
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		DWORD dwConSize;

		GetConsoleScreenBufferInfo(m_hConsoleHandle, &csbi);
		dwConSize = (csbi.dwSize.X + 1 ) * (csbi.dwSize.Y);
		FillConsoleOutputCharacter(m_hConsoleHandle, TEXT(' '), dwConSize, coordScreen, &cCharsWritten);
		GetConsoleScreenBufferInfo(m_hConsoleHandle, &csbi);
		FillConsoleOutputAttribute(m_hConsoleHandle, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
		SetConsoleCursorPosition(m_hConsoleHandle, coordScreen);
		return;
	}

protected:
	HWND				m_hHwndConsole;
	HANDLE				m_hConsoleHandle;
	HANDLE				m_hConsoleInputHandle;
	INPUT_RECORD		m_ipRecord;
	CONSOLE_SCREEN_BUFFER_INFOEX m_ScreenInfo;
	ConsoleColors		m_colText{ WHITE };
	ConsoleColors		m_colBkg{ DEFAULT };
	ConsoleColors		m_colBkgScreen{ BLACK };

	MouseEventInfo		m_MouseEventInfo;
	KeyBoardEventInfo	m_KeyBoardEventInfo;
	std::map<int, int>	m_KeyBoardTracker;
};
