﻿#pragma once

#include <assert.h>

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
	MOUSE_NONE_STATE			= 0x0000,
	MOUSE_MOVE_STATE			= 0x0001,
	MOUSE_MOVE_NO_OVER_STATE	= 0x0002,
	MOUSE_UP_STATE				= 0x0004,
	MOUSE_DOWN_STATE			= 0x0008,
};

enum ConsoleKeyboardState
{
	KEYBOARD_NONE_STATE	= 0x0000,
	KEYBOARD_DOWN_STATE	= 0x0001,
	KEYBOARD_UP_STATE	= 0x0002,
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
	int					m_MouseState;
	ConsoleMousePos		m_MousePos;
};

struct KeyBoardEventInfo
{
	int		m_nKey;
	int		m_nState;
};


enum ConsoleColors {
	BLACK = 0, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, GREY,
	LIGHTGREY, LIGHTRED, LIGHTGREEN, LIGHTYELLOW, LIGHTBLUE,
	LIGHTPURPLE, LIGHTCYAN, WHITE, DEFAULT
};


interface ConsoleDeviecDrawable
{
	virtual void SetXY(const int x, const int y) = 0;
	virtual void SetTextColor(ConsoleColors col) = 0;
	virtual void SetDrawText(const wchar_t* strText) = 0;
	virtual void SetBackgroundColor(ConsoleColors col) = 0;
	virtual void ClearColorInfo() = 0;
	virtual void SetBackgroundScreen(ConsoleColors col) = 0;
	virtual void Clear() = 0;
	virtual void SetClearColor(ConsoleColors col = DEFAULT) = 0;
};

interface ConsoleDevice
{
	virtual bool SetConsoleSize(const int nRow, const int nCol) = 0;
	virtual void SetCellSize(const int nWidth, const int nHeight) = 0;
	virtual void SetFont(const wchar_t* strFontFamily) = 0;
	virtual void SetTitle(const wchar_t* strTitle) = 0;
	virtual void SetWindowPosition(const int xPos, const int yPos) = 0;
	virtual void SetWindowCenter() = 0;
	virtual ConsoleDeviceEvent GetEvent() = 0;
	virtual MouseEventInfo* GetMouseEvent() = 0;
	virtual KeyBoardEventInfo* GetKeyboardEvent() = 0;

	// function 

};