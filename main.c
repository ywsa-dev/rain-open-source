#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <olectl.h>
#include <objbase.h>
#define INITGUID

#define DROP_COUNT 200

typedef struct { int x, y, len, speed; } Drop;
Drop drops[DROP_COUNT];
int w, h;
HINSTANCE gInst;

static const char* B64 =
    "/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDABQODxIPDRQSEBIXFRQYHjIhHhwcHj0sLiQySUBMS0dA"
    "RkVQWnNiUFVtVkVGZIhlbXd7gYKBTmCNl4x9lnN+gXz/2wBDARUXFx4aHjshITt8U0ZTfHx8fHx8"
    "fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHx8fHz/wAARCAAyADIDASIAAhEB"
    "AxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQ"
    "IDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0"
    "RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqr"
    "KztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQ"
    "EBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdh"
    "cRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldY"
    "WVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPE"
    "xcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwDsR0FLSDoKq6jd"
    "iytWk4Ln5UB7mgaTbsiq+rpbX9zb3Jwq4MZUZPQcf/XqZNXtyfnWWMf3mXj9M1lQg/ZEnclnd9zs"
    "e4PH+FNkTyCGXiMnBH90+o9q5oYhSnysmtzQeh0oIYAqQQeQR3pax9MuDDKICf3bn5R/dPp9D/P6"
    "1sV0ijJSV0FFFFBQg6Cuf8RSE3MMXZU3fiT/APWroB0Fc54gQ/2gp5O6MYAHuamWxvh7e0VyW0US"
    "6dGh6MhU/qKbvWSELKCS6kMoUk+h4FLpkFww+zuksABLl2THHoM8ZzU1gGs9cubZ3LiVA8Zbrx2/"
    "U1wQw0uZuW1yalm3boVrG0ur21EkbJGMYV2ySxHcY6DI610QzgZ696WivRMIxUdgooooKEHQVE9v"
    "G9xHOw+eMEL+NSjoKWgArI1gfZ7uxvhwEk8tz/sn/JrXqtqNt9ssZoO7r8v17frSY07Ms0VzmWat"
    "cXmo2kQLACIrcRkdGHfP5V0dMcouO4UUUUEiDoKWiigAooooAYsMSOzpGiu/3mCgFvrT6KKACiii"
    "gD//2Q==";

static const char b64chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64val(char c) {
    for (int i = 0; i < 64; i++)
        if (b64chars[i] == c) return i;
    return -1;
}

HBITMAP DecodeB64ToBitmap(HDC hdc) {
    int inLen = 0;
    const char *p = B64;
    while (*p) { if (*p != '\n' && *p != '\r') inLen++; p++; }

    int outLen = (inLen / 4) * 3;
    unsigned char *buf = (unsigned char*)malloc(outLen + 4);
    int oi = 0;
    const char *s = B64;
    unsigned char tmp[4];
    int tc = 0;
    while (*s) {
        char c = *s++;
        if (c == '\n' || c == '\r') continue;
        int v = (c == '=') ? 0 : b64val(c);
        tmp[tc++] = (unsigned char)v;
        if (tc == 4) {
            buf[oi++] = (tmp[0] << 2) | (tmp[1] >> 4);
            buf[oi++] = (tmp[1] << 4) | (tmp[2] >> 2);
            buf[oi++] = (tmp[2] << 6) | tmp[3];
            tc = 0;
        }
    }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, oi);
    void *pMem = GlobalLock(hMem);
    memcpy(pMem, buf, oi);
    GlobalUnlock(hMem);
    free(buf);

    IStream *stream = NULL;
    CreateStreamOnHGlobal(hMem, FALSE, &stream);

    IPicture *pic = NULL;
    OleLoadPicture(stream, 0, FALSE, (REFIID)&IID_IPicture, (LPVOID*)&pic);
    stream->lpVtbl->Release(stream);

    HBITMAP hbmp = NULL;
    if (pic) {
        OLE_XSIZE_HIMETRIC hmW;
        OLE_YSIZE_HIMETRIC hmH;
        pic->lpVtbl->get_Width(pic, &hmW);
        pic->lpVtbl->get_Height(pic, &hmH);
        int pw = MulDiv(hmW, GetDeviceCaps(hdc, LOGPIXELSX), 2540);
        int ph = MulDiv(hmH, GetDeviceCaps(hdc, LOGPIXELSY), 2540);
        HDC memDC = CreateCompatibleDC(hdc);
        hbmp = CreateCompatibleBitmap(hdc, pw, ph);
        SelectObject(memDC, hbmp);
        RECT r = {0, 0, pw, ph};
        FillRect(memDC, &r, (HBRUSH)GetStockObject(WHITE_BRUSH));
        pic->lpVtbl->Render(pic, memDC, 0, 0, pw, ph, 0, hmH, hmW, -hmH, NULL);
        DeleteDC(memDC);
        pic->lpVtbl->Release(pic);
    }
    GlobalFree(hMem);
    return hbmp;
}

void SpawnPopup();

void DrawPopup(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int W = rc.right, H = rc.bottom;

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, W, H);
    SelectObject(memDC, bmp);

    HBRUSH bgBrush = CreateSolidBrush(RGB(210, 230, 255));
    FillRect(memDC, &rc, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(30, 30, 120));
    HFONT font = CreateFont(20, 0, 0, 0, FW_BOLD, 0, 0, 0,
                            DEFAULT_CHARSET, 0, 0, 0, 0, "Arial");
    SelectObject(memDC, font);
    TextOut(memDC, 10, 8, "It is rainy!!!", 14);
    DeleteObject(font);

    srand(12345);
    HPEN rainPen = CreatePen(PS_SOLID, 2, RGB(100, 160, 230));
    SelectObject(memDC, rainPen);
    for (int i = 0; i < 15; i++) {
        int rx = rand() % W;
        int ry = 35 + rand() % (H - 35);
        MoveToEx(memDC, rx, ry, NULL);
        LineTo(memDC, rx - 3, ry + 18);
    }
    srand(time(NULL));
    DeleteObject(rainPen);

    HBITMAP umbrella = DecodeB64ToBitmap(hdc);
    if (umbrella) {
        HDC imgDC = CreateCompatibleDC(hdc);
        SelectObject(imgDC, umbrella);
        int imgW = 90, imgH = 90;
        int ix = (W - imgW) / 2;
        int iy = (H - imgH) / 2 + 15;
        StretchBlt(memDC, ix, iy, imgW, imgH, imgDC, 0, 0, 50, 50, SRCCOPY);
        DeleteDC(imgDC);
        DeleteObject(umbrella);
    }

    BitBlt(hdc, 0, 0, W, H, memDC, 0, 0, SRCCOPY);
    DeleteObject(bmp);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
}

void SpawnPopup() {
    int rx = 100 + rand() % 700;
    int ry = 100 + rand() % 400;
    HWND newHw = CreateWindowEx(0, "PopupClass", "It is rainy!!!",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rx, ry, 360, 200,
        NULL, NULL, gInst, NULL);
    ShowWindow(newHw, SW_SHOW);
    UpdateWindow(newHw);
}

LRESULT CALLBACK PopupProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        DrawPopup(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
if (msg == WM_CLOSE) {
    RECT rc;
    GetWindowRect(hwnd, &rc);
    DestroyWindow(hwnd);
    HWND l = CreateWindowEx(0, "PopupClass", "It is rainy!!!",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left - 30, rc.top, 360, 200,
        NULL, NULL, gInst, NULL);
    ShowWindow(l, SW_SHOW);
    UpdateWindow(l);
    HWND r = CreateWindowEx(0, "PopupClass", "It is rainy!!!",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        rc.left + 30, rc.top, 360, 200,
        NULL, NULL, gInst, NULL);
    ShowWindow(r, SW_SHOW);
    UpdateWindow(r);
    return 0;
}
    if (msg == WM_DESTROY) return 0;
    return DefWindowProc(hwnd, msg, wp, lp);
}

void initDrop(Drop *d) {
    d->x = rand() % w;
    d->y = -(rand() % 500);
    d->len = rand() % 20 + 10;
    d->speed = rand() % 15 + 8;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wp, lp);
}

DWORD WINAPI HelloThread(LPVOID param) {
    gInst = GetModuleHandle(NULL);

    WNDCLASS pc = {0};
    pc.lpfnWndProc   = PopupProc;
    pc.hInstance     = gInst;
    pc.lpszClassName = "PopupClass";
    pc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    pc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&pc);

    SpawnPopup();

    DWORD lastTime = GetTickCount();
    while (1) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        DWORD now = GetTickCount();
        if (now - lastTime >= 1500) {
            SpawnPopup();
            lastTime = now;
        }
        Sleep(10);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    srand(time(NULL));
    w = GetSystemMetrics(SM_CXSCREEN);
    h = GetSystemMetrics(SM_CYSCREEN);

    for (int i = 0; i < DROP_COUNT; i++)
        initDrop(&drops[i]);

    CreateThread(NULL, 0, HelloThread, NULL, 0, NULL);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "Rain";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "Rain", NULL, WS_POPUP,
        -w, -h, w, h,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    HDC screenDC = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP bmp = CreateCompatibleBitmap(screenDC, w, h);
    SelectObject(memDC, bmp);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, w, h, SWP_SHOWWINDOW);

    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
        RECT rect = {0, 0, w, h};
        FillRect(memDC, &rect, bg);
        DeleteObject(bg);
        for (int i = 0; i < DROP_COUNT; i++) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(100, 149, 237));
            HPEN old = SelectObject(memDC, pen);
            MoveToEx(memDC, drops[i].x, drops[i].y, NULL);
            LineTo(memDC, drops[i].x, drops[i].y + drops[i].len);
            SelectObject(memDC, old);
            DeleteObject(pen);
            drops[i].y += drops[i].speed;
            if (drops[i].y > h)
                initDrop(&drops[i]);
        }
        BitBlt(screenDC, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
        Sleep(16);
    }
    return 0;
}
