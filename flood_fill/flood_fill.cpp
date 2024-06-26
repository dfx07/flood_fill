﻿#include <iostream>
#include <Windows.h>
#include <vector>
#include <assert.h>
#include "ConsoleDevice.h"


template <typename T>
struct Board
{
public:
    void SetSize(int nRow, int nCol)
    {
        m_nRow = nRow;
        m_nCol = nCol;

        int nLength = GetLength();

        m_celldatas.resize(nLength);

        std::fill(m_celldatas.begin(), m_celldatas.end(), 0);
    }

    int GetLength() const
    {
        return m_nCol * m_nRow;
    }

    T GetCell(const int nRow, const int nCol) const
    {
        int nIdx = nRow * m_nCol + nCol;

        if (nIdx >= m_nCol * m_nRow)
            assert(0);

        return m_celldatas[nIdx];
    }

    void SetCell(const int nRow, const int nCol, T nValue)
    {
        int nIdx = nRow * m_nCol + nCol;

        if (nRow * m_nCol + nCol >= m_nCol * m_nRow)
            assert(0);

        m_celldatas[nIdx] = nValue;
    }

protected:
    std::vector<T> m_celldatas;

    int m_nCol;
    int m_nRow;

};

typedef Board<int> MaxtrixBoard;




namespace csgp
{
    void DrawTextOut(int x, int y, char* ch)
    {

    }
}


void DrawBoard(MaxtrixBoard& board)
{

}


int main()
{
    WinConsoleDeviceAPI dv;

    dv.SetTitle(L"flood_fill");
    dv.SetCellSize(12, 12);
    dv.SetConsoleSize(50, 50);

    dv.SetWindowCenter();
    dv.HideScollBar();
    dv.ShowHideCursor(FALSE);

    bool bINput = false;

    while (true)
    {
        dv.SetClearColor(ConsoleColors::YELLOW);
        dv.Clear();

        if (dv.PoolEvent())
        {
            ConsoleDeviceEvent eEvent = dv.GetEvent();

            if (eEvent == ConsoleDeviceEvent::MOUSE_CONSOLE_EVENT)
            {
                auto mouseEvent = dv.GetMouseEvent();

                if (mouseEvent->m_MouseState & MOUSE_MOVE_STATE)
                {

                }
                else if (mouseEvent->m_MouseState == MOUSE_DOWN_STATE)
                {
                    dv.SetXY(mouseEvent->m_MousePos.x, mouseEvent->m_MousePos.y);
                    dv.SetTextColor(ConsoleColors::RED);
                    dv.SetDrawText(L"0");

                    bINput = true;
                }
                else if (mouseEvent->m_MouseState == MOUSE_UP_STATE)
                {
                    bINput = false;
                }
            }
        }
        Sleep(50);

        
    }

    getchar();
}

