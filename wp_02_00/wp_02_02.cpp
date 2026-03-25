#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming 2-2";

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;

int g_divCount = 2;
HFONT g_hFont = NULL;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    srand((unsigned int)time(NULL));
    g_divCount = rand() % 15 + 2;   // 2~16

    WndClass.cbSize = sizeof(WndClass);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = lpszClass;
    WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&WndClass);

    hWnd = CreateWindow(
        lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW,
        100, 50, WIN_WIDTH, WIN_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&Message, 0, 0, 0))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int)Message.wParam;
}

void DrawDan(HDC hDC, int dan, int startX, int startY)
{
    TCHAR str[50];

    for (int i = 1; i <= 9; i++)
    {
        wsprintf(str, TEXT("%dx%d=%d"), dan, i, dan * i);
        TextOut(hDC, startX, startY + (i - 1) * 18, str, lstrlen(str));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;
    TCHAR str[50];
    HFONT hOldFont;
    int colWidth;
    int topY = 40;
    int bottomY = WIN_HEIGHT / 2 + 20;

    switch (iMessage)
    {
    case WM_CREATE:
        g_hFont = CreateFont(
            14, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            TEXT("Consolas")
        );
        return 0;

    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);

        if (g_hFont != NULL)
            hOldFont = (HFONT)SelectObject(hDC, g_hFont);
        else
            hOldFont = NULL;

        wsprintf(str, TEXT("가로 등분 수: %d"), g_divCount);
        TextOut(hDC, 10, 10, str, lstrlen(str));

        colWidth = WIN_WIDTH / g_divCount;

        // 윗줄: 2단 -> 최대단
        for (int i = 0; i < g_divCount; i++)
        {
            int dan = i + 2;
            int x = i * colWidth + 5;
            DrawDan(hDC, dan, x, topY);
        }

        // 아랫줄: 최대단 -> 2단
        for (int i = 0; i < g_divCount; i++)
        {
            int dan = g_divCount + 1 - i;
            int x = i * colWidth + 5;
            DrawDan(hDC, dan, x, bottomY);
        }

        if (g_hFont != NULL && hOldFont != NULL)
            SelectObject(hDC, hOldFont);

        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        if (g_hFont != NULL)
            DeleteObject(g_hFont);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}