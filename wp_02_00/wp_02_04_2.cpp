#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming 2-4-2";

const int DEFAULT_WIN_WIDTH = 800;
const int DEFAULT_WIN_HEIGHT = 600;
const int PALETTE_COUNT = 12;
const int MAX_GRID = 300;

HFONT g_hFont = NULL;

// 사각형 위치와 크기
int g_rectX = 50;
int g_rectY = 50;
int g_rectW = 200;
int g_rectH = 150;

// 테두리에 사용할 문자열
TCHAR g_borderText[50] = TEXT("WINDOW");

// 가로 방향과 세로 방향 색상 배열
COLORREF g_xColor[MAX_GRID];
COLORREF g_yColor[MAX_GRID];

// 보기 좋은 색상표
COLORREF g_palette[PALETTE_COUNT] =
{
    RGB(255, 0, 0),
    RGB(0, 0, 255),
    RGB(0, 128, 0),
    RGB(255, 128, 0),
    RGB(128, 0, 128),
    RGB(0, 128, 128),
    RGB(128, 0, 0),
    RGB(255, 0, 255),
    RGB(0, 0, 0),
    RGB(0, 100, 200),
    RGB(200, 50, 120),
    RGB(80, 80, 80)
};

// 문자열 후보
LPCTSTR g_textPool[6] =
{
    TEXT("WINDOW"),
    TEXT("PROGRAM"),
    TEXT("TEXT"),
    TEXT("GDI"),
    TEXT("HELLO"),
    TEXT("BOX")

};

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 현재 창 높이에 맞게 글꼴 생성
void UpdateFontByClientSize(HWND hWnd)
{
    RECT rt;
    int clientHeight;
    int fontSize;

    GetClientRect(hWnd, &rt);
    clientHeight = rt.bottom - rt.top;

    fontSize = clientHeight / 30;

    if (fontSize < 10)
        fontSize = 10;
    if (fontSize > 22)
        fontSize = 22;

    if (g_hFont != NULL)
    {
        DeleteObject(g_hFont);
        g_hFont = NULL;
    }

    g_hFont = CreateFont(
        fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FIXED_PITCH | FF_MODERN,
        TEXT("Consolas")
    );
}

// 랜덤 문자열 선택
void PrepareRandomText()
{
    lstrcpy(g_borderText, g_textPool[rand() % 6]);
}

// 창 크기에 맞춰 사각형 위치와 크기 준비
void PrepareRectangleData(HWND hWnd)
{
    RECT rt;
    int clientWidth, clientHeight;
    int maxW, maxH, maxX, maxY;

    GetClientRect(hWnd, &rt);
    clientWidth = rt.right - rt.left;
    clientHeight = rt.bottom - rt.top;

    maxW = clientWidth / 2;
    maxH = clientHeight / 2;

    if (maxW <= 110) maxW = 111;
    if (maxH <= 110) maxH = 111;

    // 문제 조건: 100 < w < 창 폭의 절반
    g_rectW = rand() % (maxW - 101) + 101;

    // 문제 조건: 100 < h < 창 높이의 절반
    g_rectH = rand() % (maxH - 101) + 101;

    maxX = clientWidth / 2;
    maxY = clientHeight / 2;

    if (maxX <= 1) maxX = 2;
    if (maxY <= 1) maxY = 2;

    // 문제 조건: 0 < x < 창 폭의 절반
    g_rectX = rand() % (maxX - 1) + 1;

    // 문제 조건: 0 < y < 창 높이의 절반
    g_rectY = rand() % (maxY - 1) + 1;

    // 사각형이 화면 밖으로 나가지 않게 보정
    if (g_rectX + g_rectW > clientWidth - 5)
        g_rectW = clientWidth - g_rectX - 5;

    if (g_rectY + g_rectH > clientHeight - 5)
        g_rectH = clientHeight - g_rectY - 5;

    if (g_rectW < 50) g_rectW = 50;
    if (g_rectH < 50) g_rectH = 50;
}

// 가로 방향 색상 준비
void PrepareXColors(int count)
{
    for (int i = 0; i < count; i++)
        g_xColor[i] = g_palette[rand() % PALETTE_COUNT];
}

// 세로 방향 색상 준비
void PrepareYColors(int count)
{
    for (int i = 0; i < count; i++)
        g_yColor[i] = g_palette[rand() % PALETTE_COUNT];
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    srand((unsigned int)time(NULL));

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
        lpszClass,
        lpszWindowName,
        WS_OVERLAPPEDWINDOW,
        100, 50,
        DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT,
        NULL, NULL,
        hInstance,
        NULL
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;
    HFONT hOldFont = NULL;
    TEXTMETRIC tm;
    int charWidth, lineHeight;
    int cols, rows;
    int textLen;
    TCHAR ch[2];

    switch (iMessage)
    {
    case WM_CREATE:
        // 처음 시작할 때 글꼴과 사각형 데이터 준비
        UpdateFontByClientSize(hWnd);
        PrepareRandomText();
        PrepareRectangleData(hWnd);
        return 0;

    case WM_SIZE:
        // 창 크기가 바뀌면 다시 계산
        UpdateFontByClientSize(hWnd);
        PrepareRandomText();
        PrepareRectangleData(hWnd);
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);

        if (g_hFont != NULL)
            hOldFont = (HFONT)SelectObject(hDC, g_hFont);

        SetBkMode(hDC, TRANSPARENT);

        // 현재 문자 폭과 줄 높이 계산
        GetTextMetrics(hDC, &tm);
        charWidth = tm.tmAveCharWidth;
        lineHeight = tm.tmHeight + 2;

        if (charWidth <= 0) charWidth = 8;
        if (lineHeight <= 0) lineHeight = 16;

        // 사각형을 문자 칸 기준으로 계산
        cols = g_rectW / charWidth;
        rows = g_rectH / lineHeight;

        if (cols < 2) cols = 2;
        if (rows < 2) rows = 2;
        if (cols > MAX_GRID) cols = MAX_GRID;
        if (rows > MAX_GRID) rows = MAX_GRID;

        // 현재 크기에 맞게 색상 준비
        PrepareXColors(cols);
        PrepareYColors(rows);

        textLen = lstrlen(g_borderText);
        ch[1] = '\0';

        // 윗변 출력
        for (int c = 0; c < cols; c++)
        {
            ch[0] = g_borderText[c % textLen];
            SetTextColor(hDC, g_xColor[c]);
            TextOut(hDC, g_rectX + c * charWidth, g_rectY, ch, 1);
        }

        // 아랫변 출력
        for (int c = 0; c < cols; c++)
        {
            ch[0] = g_borderText[c % textLen];
            SetTextColor(hDC, g_xColor[c]);
            TextOut(hDC, g_rectX + c * charWidth, g_rectY + (rows - 1) * lineHeight, ch, 1);
        }

        // 왼변 출력
        for (int r = 1; r < rows - 1; r++)
        {
            ch[0] = g_borderText[r % textLen];
            SetTextColor(hDC, g_yColor[r]);
            TextOut(hDC, g_rectX, g_rectY + r * lineHeight, ch, 1);
        }

        // 오른변 출력
        for (int r = 1; r < rows - 1; r++)
        {
            ch[0] = g_borderText[r % textLen];
            SetTextColor(hDC, g_yColor[r]);
            TextOut(hDC, g_rectX + (cols - 1) * charWidth, g_rectY + r * lineHeight, ch, 1);
        }

        // 내부는 비워둔다

        if (g_hFont != NULL && hOldFont != NULL)
            SelectObject(hDC, hOldFont);

        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        // 만든 글꼴이 있으면 삭제
        if (g_hFont != NULL)
            DeleteObject(g_hFont);

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}