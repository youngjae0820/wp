#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming 2-3";

const int WIN_WIDTH = 1200;
const int WIN_HEIGHT = 800;
const int MAX_DIV = 16;
const int MAX_LINE = 9;

int g_divCount = 2;
int g_modeNumber = 1;   // È¦¼ö/Â¦¼ö ÆÇÁ¤¿ë
HFONT g_hFont = NULL;

COLORREF danColor[MAX_DIV];
COLORREF lineColor[MAX_LINE];

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

COLORREF makeSafeColor()
{
    int r = rand() % 200;
    int g = rand() % 200;
    int b = rand() % 200;
    return RGB(r, g, b);
}

void prepareColors()
{
    for (int i = 0; i < MAX_DIV; i++)
        danColor[i] = makeSafeColor();

    for (int i = 0; i < MAX_LINE; i++)
        lineColor[i] = makeSafeColor();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    srand((unsigned int)time(NULL));
    g_divCount = rand() % 15 + 2;     // 2~16
    g_modeNumber = rand() % 100 + 1;  // È¦Â¦ ÆÇÁ¤¿ë
    prepareColors();

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

void DrawDanColored(HDC hDC, int dan, int startX, int startY, int colorIndex, bool oddMode)
{
    TCHAR str[50];

    for (int i = 1; i <= 9; i++)
    {
        if (oddMode)
            SetTextColor(hDC, danColor[colorIndex]);
        else
            SetTextColor(hDC, lineColor[i - 1]);

        wsprintf(str, TEXT("%dx%d=%d"), dan, i, dan * i);
        TextOut(hDC, startX, startY + (i - 1) * 18, str, lstrlen(str));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;
    TCHAR str[80];
    HFONT hOldFont;
    int colWidth;
    int topY = 40;
    int bottomY = WIN_HEIGHT / 2 + 20;
    bool oddMode = (g_modeNumber % 2 == 1);

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

        wsprintf(str, TEXT("°¡·Î µîºÐ ¼ö: %d"), g_divCount);
        TextOut(hDC, 10, 10, str, lstrlen(str));

        if (oddMode)
            wsprintf(str, TEXT("·£´ý ¼ö %d -> È¦¼ö ¸ðµå (´Üº° °°Àº »ö)"), g_modeNumber);
        else
            wsprintf(str, TEXT("·£´ý ¼ö %d -> Â¦¼ö ¸ðµå (ÁÙº° ´Ù¸¥ »ö)"), g_modeNumber);

        TextOut(hDC, 220, 10, str, lstrlen(str));

        colWidth = WIN_WIDTH / g_divCount;

        // À­ÁÙ
        for (int i = 0; i < g_divCount; i++)
        {
            int dan = i + 2;
            int x = i * colWidth + 5;
            DrawDanColored(hDC, dan, x, topY, i, oddMode);
        }

        // ¾Æ·§ÁÙ
        for (int i = 0; i < g_divCount; i++)
        {
            int dan = g_divCount + 1 - i;
            int x = i * colWidth + 5;
            DrawDanColored(hDC, dan, x, bottomY, g_divCount - 1 - i, oddMode);
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