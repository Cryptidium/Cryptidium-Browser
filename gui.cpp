#include "gui.h"
#include <windows.h>
#include <WebKit/WebKit2_C.h>
#include "buildinfo.h"
#include <string>
#include <algorithm>

static WKViewRef gMainView;

static void ResizeView(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    HWND child = WKViewGetWindow(gMainView);
    MoveWindow(child, 0, 0, rc.right, rc.bottom, TRUE);
}

static void LoadUI()
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    path = path.substr(0, pos);
    std::string url = "file:///" + path + "/index.html";
    std::replace(url.begin(), url.end(), '\\', '/');
    WKURLRef wkurl = WKURLCreateWithUTF8CString(url.c_str());
    WKPageLoadURL(WKViewGetPage(gMainView), wkurl);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        RECT rc;
        GetClientRect(hWnd, &rc);
        WKPageConfigurationRef cfg = WKPageConfigurationCreate();
        WKContextRef ctx = WKContextCreateWithConfiguration(nullptr);
        WKPageConfigurationSetContext(cfg, ctx);
        gMainView = WKViewCreate(rc, cfg, hWnd);
        HWND child = WKViewGetWindow(gMainView);
        ShowWindow(child, SW_SHOW);
        WKViewSetIsInWindow(gMainView, true);
        LoadUI();
        return 0;
    }
    case WM_SIZE:
        ResizeView(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int RunBrowser(HINSTANCE hInst, int nCmdShow)
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"CryptidiumBrowser";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    HICON icon = (HICON)LoadImageW(nullptr, L"assets\\app.ico", IMAGE_ICON,
                                   0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    wc.hIcon = icon;
    wc.hIconSm = icon;
    RegisterClassExW(&wc);
    HWND win = CreateWindowW(wc.lpszClassName, L"Cryptidium", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
                             nullptr, nullptr, hInst, nullptr);
    ShowWindow(win, nCmdShow);
    UpdateWindow(win);
    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}

