#include <windows.h>


// global variable
LPCWSTR WindowsTitle = L"Netmable Overlay";
LPCWSTR WindowsClass = L"Netmable Overlay";

LPCWSTR TargetWindowsTitle = L"넷마블 사천성 Ver0.65";

HWND hWnd;
HWND tWnd;
RECT tSize;

int Width, Height;

// memory
DWORD pId;
HANDLE pHandle;
DWORD address;
DWORD value;
BYTE bValue;
SHORT sValue;
DWORD baseAddress = 0x00400000;
DWORD ecxAddress = baseAddress + 0x0030DEFC;
DWORD arrayStartPoint;
BYTE result[12][22] = { 0 };
BYTE mask[12][22] = { 0 };

// timer
int idTimer = -1;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


DWORD WINAPI SetWindowToTarget(LPVOID lpParam)
{
    while (true)
    {
        tWnd = FindWindow(0, TargetWindowsTitle);
        if (tWnd)
        {
            GetWindowRect(tWnd, &tSize);
            Width = tSize.right - tSize.left;
            Height = tSize.bottom - tSize.top;
            DWORD dwStyle = GetWindowLong(tWnd, GWL_STYLE);
            if (dwStyle & WS_BORDER)
            {
                tSize.top += 85;
                Height -= 20;
            }
            MoveWindow(hWnd, tSize.left, tSize.top, Width, Height, true);
        }
        else
        {
            exit(1);
        }
        Sleep(100);
    }
}

DWORD WINAPI scanMemory(LPVOID lpParam) {
    while (true) {
        // 배열 값 설정
        for (int y = 0; y < 22; y++) {
            for (int x = 0; x < 12; x++) {
                DWORD nextPoint = arrayStartPoint + (0x34 * x) + (0x270 * y);
                ReadProcessMemory(pHandle, (void*)nextPoint, &bValue, sizeof(bValue), 0);
                result[x][y] = bValue;
            }
        }

        // 마스킹 배열 초기화
        for (int x = 0; x < 12; x++) {
            for (int y = 0; y < 22; y++) {
                mask[x][y] = 0;
            }
        }

        // 이웃 가로축 마스킹
        for (int x = 0; x < 12; x++) {
            for (int y = 0; y < 22; y++) {
                if (y + 1 != 22 && result[x][y] != 0 && result[x][y] != 67 && result[x][y] == result[x][y + 1]) {
                    mask[x][y] = 11;
                    mask[x][y + 1] = 11;
                    goto EXIT;
                }
            }
        }

        // 이웃 세로축 마스킹
        for (int y = 0; y < 22; y++) {
            for (int x = 0; x < 12; x++) {
                if (x + 1 != 12 && result[x][y] != 0 && result[x][y] != 67 && result[x][y] == result[x + 1][y]) {
                    mask[x][y] = 11;
                    mask[x + 1][y] = 11;
                    goto EXIT;
                }
            }
        }

        EXIT:
            Sleep(100);
            InvalidateRect(hWnd, NULL, TRUE);
    }
}

void drawOverlay(HWND hWnd) {
    RECT  rect;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    HBRUSH myBrush = (HBRUSH)CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, myBrush);

    for (int x = 0; x < 12; x++) {
        for (int y = 0; y < 22; y++) {
            if (mask[x][y] == 11) {
                rect.left = 155 + (y * 32);
                rect.top = 125 + (x * 41);
                rect.right = 190 + (y * 32);
                rect.bottom = 160 + (x * 41);
                Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
            }
        }
    }

    SelectObject(hdc, oldBrush);
    DeleteObject(myBrush);

    EndPaint(hWnd, &ps);
}

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // default
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = WindowsClass;
    wcex.hIconSm = NULL;
    RegisterClassExW(&wcex);

    // overlay
    tWnd = FindWindowW(NULL, TargetWindowsTitle);
    if (!tWnd)
    {
        return FALSE;
    }
    GetWindowRect(tWnd, &tSize);
    Width = tSize.right - tSize.left;
    Height = tSize.bottom - tSize.top;
    hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT, WindowsClass, WindowsTitle, WS_POPUP, 0, 0, Width, Height, nullptr, nullptr, hInstance, nullptr);
    SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) ^ WS_EX_LAYERED);
    SetLayeredWindowAttributes(hWnd, 0, RGB(0, 0, 0), LWA_COLORKEY);

    // memory hack
    GetWindowThreadProcessId(tWnd, &pId);
    pHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pId);
    if (!pHandle) {
        return 0;
    }
    address = ecxAddress;
    ReadProcessMemory(pHandle, (void*)address, &value, sizeof(value), 0);

    DWORD ecxValue = value;
    DWORD gabAddress = 0x0000A168;
    DWORD realAddress = ecxValue + gabAddress;
    arrayStartPoint = realAddress - 0xA127;
    
    // call thread
    HANDLE threadSM = CreateThread(NULL, 0, scanMemory, NULL, 0, NULL);

    // call thread
    HANDLE threadSWTT = CreateThread(NULL, 0, SetWindowToTarget, NULL, 0, NULL);

    // default
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_PAINT:
            drawOverlay(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

