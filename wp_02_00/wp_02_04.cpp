#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming 2-4";

const int DEFAULT_WIN_WIDTH = 800;
const int DEFAULT_WIN_HEIGHT = 600;
const int LINE_COUNT = 9;
const int PALETTE_COUNT = 12;

int g_dan = 2;           // 출력할 단
int g_cellIndex = 0;     // 몇 번째 칸에 출력할지
HFONT g_hFont = NULL;    // 출력용 글꼴
COLORREF g_rowColor[LINE_COUNT];

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 줄마다 서로 다른 색을 준비
void PrepareRowColors()
{
    bool used[PALETTE_COUNT] = { false };

    for (int i = 0; i < LINE_COUNT; i++)
    {
        int idx;
        do
        {
            idx = rand() % PALETTE_COUNT;
        } while (used[idx]);

        used[idx] = true;
        g_rowColor[i] = g_palette[idx];
    }
}

// 현재 창 높이에 맞는 글꼴 생성
void UpdateFontByClientSize(HWND hWnd)
{
    RECT rt;
    int clientHeight;
    int fontSize;

    GetClientRect(hWnd, &rt);
    clientHeight = rt.bottom - rt.top;

    fontSize = clientHeight / 28;

    if (fontSize < 8)
        fontSize = 8;
    if (fontSize > 28)
        fontSize = 28;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    srand((unsigned int)time(NULL));

    // 2~20 사이의 단을 랜덤 설정
    g_dan = rand() % 19 + 2;

    // 단 수만큼 나눈 칸 중 하나를 랜덤 선택
    g_cellIndex = rand() % g_dan;

    // 줄 색상 준비
    PrepareRowColors();

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
    RECT rt;
    HFONT hOldFont = NULL;
    TEXTMETRIC tm;
    SIZE sz;
    TCHAR str[80];

    int clientWidth, clientHeight;
    int cellWidth;
    int charWidth, lineHeight;
    int startX, startY;
    int maxNeedWidth = 0;
    int tableHeight;

    switch (iMessage)
    {
    case WM_CREATE:
        // 처음 창 생성 시 글꼴 준비
        UpdateFontByClientSize(hWnd);
        return 0;

    case WM_SIZE:
        // 창 크기 변경 시 글꼴만 다시 조정
        UpdateFontByClientSize(hWnd);

        // 같은 단, 같은 칸 번호로 다시 배치 계산해서 그리기
        InvalidateRect(hWnd, NULL, TRUE);
        return 0;

    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);

        // 현재 클라이언트 영역 크기 구하기
        GetClientRect(hWnd, &rt);
        clientWidth = rt.right - rt.left;
        clientHeight = rt.bottom - rt.top;

        if (g_hFont != NULL)
            hOldFont = (HFONT)SelectObject(hDC, g_hFont);

        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(0, 0, 0));

        // 안내 문구 출력
        wsprintf(str, TEXT("설정한 단: %d"), g_dan);
        TextOut(hDC, 10, 10, str, lstrlen(str));

        wsprintf(str, TEXT("가로 %d등분 중 %d번째 칸"), g_dan, g_cellIndex + 1);
        TextOut(hDC, 150, 10, str, lstrlen(str));

        // 현재 글꼴 기준 문자 폭과 줄 높이
        GetTextMetrics(hDC, &tm);
        charWidth = tm.tmAveCharWidth;
        lineHeight = tm.tmHeight + 4;

        if (charWidth <= 0) charWidth = 8;
        if (lineHeight <= 0) lineHeight = 16;

        // 현재 폭을 단 수만큼 나눔
        cellWidth = clientWidth / g_dan;
        if (cellWidth <= 0)
            cellWidth = 1;

        // 가장 긴 줄 + 들여쓰기 최대폭 계산
        for (int i = 1; i <= 9; i++)
        {
            wsprintf(str, TEXT("%dx%d=%d"), g_dan, i, g_dan * i);
            GetTextExtentPoint32(hDC, str, lstrlen(str), &sz);

            int widthWithIndent = sz.cx + (i - 1) * 3 * charWidth;
            if (widthWithIndent > maxNeedWidth)
                maxNeedWidth = widthWithIndent;
        }

        // 선택된 칸의 시작 x좌표
        startX = g_cellIndex * cellWidth + 5;

        // 오른쪽 밖으로 안 나가게 보정
        if (startX + maxNeedWidth > clientWidth - 5)
            startX = clientWidth - maxNeedWidth - 5;

        if (startX < 5)
            startX = 5;

        // 시작 y좌표
        startY = 60;
        tableHeight = LINE_COUNT * lineHeight;

        // 아래쪽 밖으로 안 나가게 보정
        if (startY + tableHeight > clientHeight - 8)
        {
            startY = clientHeight - tableHeight - 8;
            if (startY < 5)
                startY = 5;
        }

        // 구구단 9줄 출력
        for (int i = 1; i <= 9; i++)
        {
            wsprintf(str, TEXT("%dx%d=%d"), g_dan, i, g_dan * i);

            // 줄마다 다른 색상
            SetTextColor(hDC, g_rowColor[i - 1]);

            // 오른쪽으로 3칸씩 들여쓰기
            TextOut(
                hDC,
                startX + (i - 1) * 2 * charWidth,
                startY + (i - 1) * lineHeight,
                str,
                lstrlen(str)
            );
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