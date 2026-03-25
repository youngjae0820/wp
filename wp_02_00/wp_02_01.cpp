#include <windows.h>
#include <tchar.h>
#include <ctime>
#include <cstdlib>

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming 2-1";

const int WIN_WIDTH = 800;
const int WIN_HEIGHT = 600;
const int POINT_COUNT = 15;

int posX[POINT_COUNT];
int posY[POINT_COUNT];

int centerX = WIN_WIDTH / 2;
int centerY = WIN_HEIGHT / 2;

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

bool isTooClose(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2;
    int dy = y1 - y2;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < 140 && dy < 30)
        return true;

    return false;
}

void makeRandomPositions()
{
    int i, j;
    int x, y;
    bool overlap;
    int tryCount;

    for (i = 0; i < POINT_COUNT; i++)
    {
        tryCount = 0;

        while (true)
        {
            x = (rand() % ((WIN_WIDTH - 160) / 10)) * 10;
            y = (rand() % ((WIN_HEIGHT - 40) / 10)) * 10;

            overlap = false;

            if (isTooClose(x, y, centerX, centerY))
                overlap = true;

            for (j = 0; j < i; j++)
            {
                if (isTooClose(x, y, posX[j], posY[j]))
                {
                    overlap = true;
                    break;
                }
            }

            if (!overlap)
            {
                posX[i] = x;
                posY[i] = y;
                break;
            }

            tryCount++;

            if (tryCount > 1000)
            {
                posX[i] = 20 + (i % 5) * 150;
                posY[i] = 20 + (i / 5) * 100;
                break;
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass;
    g_hInst = hInstance;

    srand((unsigned int)time(NULL));
    makeRandomPositions();

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

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    HDC hDC;
    PAINTSTRUCT ps;
    TCHAR str[100];

    switch (iMessage)
    {
    case WM_PAINT:
        hDC = BeginPaint(hWnd, &ps);

        for (int i = 0; i < POINT_COUNT; i++)
        {
            wsprintf(str, TEXT("%d: (%d, %d)"), i + 1, posX[i], posY[i]);
            TextOut(hDC, posX[i], posY[i], str, lstrlen(str));
        }

        wsprintf(str, TEXT("0: (%d, %d)"), centerX, centerY);
        TextOut(hDC, centerX, centerY, str, lstrlen(str));

        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}