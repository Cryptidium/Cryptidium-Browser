// This is a cleaned up version of gui.cpp with merge conflicts resolved and
// simplified tab handling.  It provides a basic multi‑tab WebKit browser
// without version number in the window title.

#include "gui.h"
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <WebKit/WebKit2_C.h>
#include <dwmapi.h> // For DwmExtendFrameIntoClientArea
#include "buildinfo.h"

#pragma comment(lib, "Comctl32.lib")
// Link against Dwmapi for glass frame extension
#pragma comment(lib, "Dwmapi.lib")

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
static HWND gSettingsBtn;
static HWND gNewTabBtn;

// Custom caption buttons
static HWND gMinBtn;
static HWND gMaxBtn;
static HWND gCloseBtn;

// Height of the tab bar integrated into the window's top border.
static const int TAB_HEIGHT = 30;
static const int NAV_HEIGHT = 28;

// Custom user agent string used for all pages.
static const char kUserAgent[] =
"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
"AppleWebKit/537.36 (KHTML, like Gecko) "
"Chrome/121.0.0.0 Safari/537.36 Cryptidium/1.0";

// Loader client used to receive page events (e.g. titles).
// loader client has been removed since it caused instability.

static void ShowCurrentTab();
static void ResizeChildren(HWND hWnd);
static void NavigateCurrent(const char* url);
static void AddTab(HWND hWnd, const char* url);

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
    // Width for caption buttons (min, max, close).  Use 40px each for a nicer feel.
    const int captionButtonWidth = 40;
    // Compute space consumed by caption buttons and the new-tab button.
    int captionButtonsTotal = captionButtonWidth * 3;
    int newTabWidth = 30;
    int tabBarWidth = rc.right - captionButtonsTotal - newTabWidth;

    // Position the tab control and new-tab button along the top row.
    MoveWindow(gTabCtrl, 0, 0, tabBarWidth, TAB_HEIGHT, TRUE);
    MoveWindow(gNewTabBtn, tabBarWidth, 0, newTabWidth, TAB_HEIGHT, TRUE);

    // Position the custom caption buttons to the right of the new-tab button.
    MoveWindow(gMinBtn, tabBarWidth + newTabWidth + 0 * captionButtonWidth, 0,
        captionButtonWidth, TAB_HEIGHT, TRUE);
    MoveWindow(gMaxBtn, tabBarWidth + newTabWidth + 1 * captionButtonWidth, 0,
        captionButtonWidth, TAB_HEIGHT, TRUE);
    MoveWindow(gCloseBtn, tabBarWidth + newTabWidth + 2 * captionButtonWidth, 0,
        captionButtonWidth, TAB_HEIGHT, TRUE);

    // Lay out the navigation bar below the tabs.
    int y = TAB_HEIGHT;
    MoveWindow(gBackBtn, 0, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gForwardBtn, 30, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gRefreshBtn, 60, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gSettingsBtn, rc.right - 30, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gUrlBar, 90, y, rc.right - 120, NAV_HEIGHT, TRUE);

    // Size the WKViews to fill the remaining client area.
    int top = TAB_HEIGHT + NAV_HEIGHT;
    for (auto& t : gTabs) {
        HWND child = WKViewGetWindow(t.view);
        MoveWindow(child, 0, top, rc.right, rc.bottom - top, TRUE);
    }
}

// Initialize the loader client once.  Registers a callback to update tab titles.

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
    // Do not hide the view immediately; make it part of the window.  The tab
    // selection logic will show/hide the appropriate views.
    ShowWindow(child, SW_SHOW);
    WKViewSetIsInWindow(view, true);

    // Set a custom user agent for the page.
    WKPageRef page = WKViewGetPage(view);
    WKStringRef ua = WKStringCreateWithUTF8CString(kUserAgent);
    WKPageSetCustomUserAgent(page, ua);
    WKRelease(ua);

    gTabs.push_back({ view });
    int index = static_cast<int>(gTabs.size()) - 1;

    TCITEMW tie{};
    tie.mask = TCIF_TEXT;
    wchar_t text[32];
    swprintf(text, 32, L"Tab %d", index + 1);
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
    // When the user presses Enter in the URL bar, navigate to the typed URL directly.
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
            0, 0, 0, 0, hWnd, (HMENU)1007, nullptr, nullptr);
        gNewTabBtn = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1006, nullptr, nullptr);
        gBackBtn = CreateWindowW(L"BUTTON", L"<", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1001, nullptr, nullptr);
        gForwardBtn = CreateWindowW(L"BUTTON", L">", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1002, nullptr, nullptr);
        gRefreshBtn = CreateWindowW(L"BUTTON", L"R", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1003, nullptr, nullptr);
        gSettingsBtn = CreateWindowW(L"BUTTON", L"⚙", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1005, nullptr, nullptr);
        gUrlBar = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, hWnd, (HMENU)1004, nullptr, nullptr);
        SetWindowSubclass(gUrlBar, UrlBarProc, 0, 0);

        // Create custom caption buttons (minimize, maximize/restore, close)
        gMinBtn = CreateWindowW(L"BUTTON", L"\u2013", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)2001, nullptr, nullptr);
        gMaxBtn = CreateWindowW(L"BUTTON", L"\u25A1", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)2002, nullptr, nullptr);
        gCloseBtn = CreateWindowW(L"BUTTON", L"\u00D7", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)2003, nullptr, nullptr);

        AddTab(hWnd, "https://google.com");
        ResizeChildren(hWnd);
        return 0;
    }
    case WM_SIZE: {
        ResizeChildren(hWnd);
        return 0;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 1001:
            if (gCurrentTab >= 0)
                WKPageGoBack(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1002:
            if (gCurrentTab >= 0)
                WKPageGoForward(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1003:
            if (gCurrentTab >= 0)
                WKPageReload(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1005:
            MessageBoxW(hWnd, L"Settings not implemented", L"Settings", MB_OK);
            break;
        case 1006:
            AddTab(hWnd, "https://google.com");
            break;
        case 1004: {
            wchar_t wbuf[2048];
            GetWindowTextW(gUrlBar, wbuf, 2048);
            char buf[2048];
            WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), nullptr, nullptr);
            NavigateCurrent(buf);
            break;
        }
        case 2001: // minimize
            ShowWindow(hWnd, SW_MINIMIZE);
            break;
        case 2002: // maximize/restore
            if (IsZoomed(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
            else
                ShowWindow(hWnd, SW_MAXIMIZE);
            break;
        case 2003: // close
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        return 0;
    }
    case WM_NOTIFY: {
        if (((LPNMHDR)lParam)->idFrom == 1007 && ((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
            gCurrentTab = TabCtrl_GetCurSel(gTabCtrl);
            ShowCurrentTab();
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_NCHITTEST: {
        // Let the default window procedure determine hit-test for edges and corners first.
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            // If within the tab bar, allow dragging except on caption buttons and new-tab button.
            if (pt.y < TAB_HEIGHT) {
                // Compute widths similar to ResizeChildren.
                RECT rc;
                GetClientRect(hWnd, &rc);
                const int captionButtonWidth = 40;
                int captionButtonsTotal = captionButtonWidth * 3;
                int newTabWidth = 30;
                int tabBarWidth = rc.right - captionButtonsTotal - newTabWidth;
                // If the mouse is over the caption buttons (min, max, close) or the new-tab button, don't treat as caption.
                if (pt.x >= tabBarWidth + newTabWidth) {
                    // Over caption buttons, treat as client so the buttons can be clicked.
                    return HTCLIENT;
                }
                if (pt.x >= tabBarWidth && pt.x < tabBarWidth + newTabWidth) {
                    // Over the new-tab button.
                    return HTCLIENT;
                }
                // Otherwise, treat as caption (dragging area).
                return HTCAPTION;
            }
        }
        return hit;
    }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int RunBrowser(HINSTANCE hInst, int nCmdShow)
{
    const wchar_t cls[] = L"Cryptidium";
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cls;

    HICON icon = (HICON)LoadImageW(nullptr, L"assets\\app.ico", IMAGE_ICON,
        0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    wc.hIcon = icon;
    wc.hIconSm = icon;
    RegisterClassExW(&wc);

    wchar_t title[] = L"Cryptidium";
    // Create a popup window with a thick frame.  This removes the default caption
    // and system menu while still allowing resizing.
    HWND win = CreateWindowExW(0, cls, title,
        WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(win, nCmdShow);
    UpdateWindow(win);

    // Extend the frame into the client area so the default title bar area becomes part of
    // our client region.  This avoids Windows drawing its own caption and accent color.
    {
        MARGINS margins = { 0, 0, TAB_HEIGHT, 0 };
        DwmExtendFrameIntoClientArea(win, &margins);
    }

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}