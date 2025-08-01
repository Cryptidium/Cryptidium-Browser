#include "gui.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <WebKit/WebKit2_C.h>
#include "buildinfo.h"

#pragma comment(lib, "Comctl32.lib")

struct Tab {
    WKViewRef view;
};

static std::vector<Tab> gTabs;
static int gCurrentTab = -1;

static HWND gTabCtrl;
static HWND gUrlBar;
static HWND gBackBtn;
static HWND gForwardBtn;
static HWND gRefreshBtn;
static HWND gNewTabBtn;

static const int TAB_HEIGHT = 24;
static const int NAV_HEIGHT = 28;

static void ShowCurrentTab()
{
    for (size_t i = 0; i < gTabs.size(); ++i) {
        HWND child = WKViewGetWindow(gTabs[i].view);
        ShowWindow(child, i == static_cast<size_t>(gCurrentTab) ? SW_SHOW : SW_HIDE);
    }
}

static void ResizeChildren(HWND hWnd)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    MoveWindow(gTabCtrl, 0, 0, rc.right - 30, TAB_HEIGHT, TRUE);
    MoveWindow(gNewTabBtn, rc.right - 30, 0, 30, TAB_HEIGHT, TRUE);
    int y = TAB_HEIGHT;
    MoveWindow(gBackBtn, 0, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gForwardBtn, 30, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gRefreshBtn, 60, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gUrlBar, 90, y, rc.right - 90, NAV_HEIGHT, TRUE);
    int top = TAB_HEIGHT + NAV_HEIGHT;
    for (auto& t : gTabs) {
        HWND child = WKViewGetWindow(t.view);
        MoveWindow(child, 0, top, rc.right, rc.bottom - top, TRUE);
    }
}

static void NavigateCurrent(const char* url)
{
    if (gCurrentTab < 0)
        return;
    WKURLRef wkurl = WKURLCreateWithUTF8CString(url);
    WKPageLoadURL(WKViewGetPage(gTabs[gCurrentTab].view), wkurl);
}

static void AddTab(HWND hWnd, const char* url)
{
    RECT rc;
    GetClientRect(hWnd, &rc);
    int top = TAB_HEIGHT + NAV_HEIGHT;
    RECT webRect{ 0, top, rc.right, rc.bottom };
    WKPageConfigurationRef cfg = WKPageConfigurationCreate();
    WKContextRef ctx = WKContextCreateWithConfiguration(nullptr);
    WKPageConfigurationSetContext(cfg, ctx);
    WKViewRef view = WKViewCreate(webRect, cfg, hWnd);
    HWND child = WKViewGetWindow(view);
    ShowWindow(child, SW_HIDE);
    WKViewSetIsInWindow(view, true);
    gTabs.push_back({ view });
    int index = static_cast<int>(gTabs.size()) - 1;
    TCITEMW tie{};
    tie.mask = TCIF_TEXT;
    wchar_t text[32];
    wsprintfW(text, L"Tab %d", index + 1);
    tie.pszText = text;
    TabCtrl_InsertItem(gTabCtrl, index, &tie);
    gCurrentTab = index;
    ShowCurrentTab();
    if (url) {
        NavigateCurrent(url);
    }
    TabCtrl_SetCurSel(gTabCtrl, gCurrentTab);
    ResizeChildren(hWnd);
}

static LRESULT CALLBACK UrlBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                   UINT_PTR, DWORD_PTR)
{
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        wchar_t wbuf[2048];
        GetWindowTextW(hwnd, wbuf, 2048);
        char buf[2048];
        WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), nullptr, nullptr);
        NavigateCurrent(buf);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        InitCommonControls();
        gTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                   0, 0, 0, 0, hWnd, (HMENU)1000, nullptr, nullptr);
        gNewTabBtn = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
                                   0, 0, 0, 0, hWnd, (HMENU)1001, nullptr, nullptr);
        gBackBtn = CreateWindowW(L"BUTTON", L"<", WS_CHILD | WS_VISIBLE,
                                 0, 0, 0, 0, hWnd, (HMENU)1002, nullptr, nullptr);
        gForwardBtn = CreateWindowW(L"BUTTON", L">", WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0, hWnd, (HMENU)1003, nullptr, nullptr);
        gRefreshBtn = CreateWindowW(L"BUTTON", L"R", WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0, hWnd, (HMENU)1004, nullptr, nullptr);
        gUrlBar = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                  0, 0, 0, 0, hWnd, (HMENU)1005, nullptr, nullptr);
        SetWindowSubclass(gUrlBar, UrlBarProc, 0, 0);
        AddTab(hWnd, "https://google.com");
        return 0;
    }
    case WM_SIZE:
        ResizeChildren(hWnd);
        return 0;
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1001:
            AddTab(hWnd, "https://google.com");
            break;
        case 1002:
            if (gCurrentTab >= 0)
                WKPageGoBack(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1003:
            if (gCurrentTab >= 0)
                WKPageGoForward(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1004:
            if (gCurrentTab >= 0)
                WKPageReload(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1005: {
            wchar_t wbuf[2048];
            GetWindowTextW(gUrlBar, wbuf, 2048);
            char buf[2048];
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), nullptr, nullptr);
            NavigateCurrent(buf);
            break;
        }
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR nm = (LPNMHDR)lParam;
        if (nm->hwndFrom == gTabCtrl && nm->code == TCN_SELCHANGE) {
            gCurrentTab = TabCtrl_GetCurSel(gTabCtrl);
            ShowCurrentTab();
        }
        return 0;
    }
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