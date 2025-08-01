#include "gui.h"
#include <windowsx.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <urlmon.h>
#include <cstdlib>
#include <cstring>
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKPageLoaderClient.h>
#include <WebKit/WKString.h>
#include "buildinfo.h"

#ifndef EN_RETURN
#define EN_RETURN 0x2000
#endif

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Urlmon.lib")

struct Tab {
    WKViewRef view;
    std::string url;
    int imageIndex;
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
static HWND gCloseTabBtn;
static HIMAGELIST gTabImages;

static const int TAB_HEIGHT = 24;
static const int NAV_HEIGHT = 28;
static const char kUserAgent[] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/121.0.0.0 Firefox/121.0 Safari/537.36 Cryptidium/1.0";

static WKPageLoaderClientV0 gLoaderClient{};
static bool gLoaderClientInit = false;

static void EnsureLoaderClient();
static void UpdateFavicon(int index);
static int FindTabByPage(WKPageRef page);
static void DidReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo);
static void CloseTab(HWND hWnd, int index);
static void ResizeChildren(HWND hWnd);
static void ShowCurrentTab();

static void EnsureLoaderClient()
{
    if (gLoaderClientInit)
        return;
    memset(&gLoaderClient, 0, sizeof(gLoaderClient));
    gLoaderClient.base.version = 0;
    gLoaderClient.didReceiveTitleForFrame = DidReceiveTitleForFrame;
    gLoaderClientInit = true;
}

static int FindTabByPage(WKPageRef page)
{
    for (size_t i = 0; i < gTabs.size(); ++i)
        if (WKViewGetPage(gTabs[i].view) == page)
            return static_cast<int>(i);
    return -1;
}

static void UpdateFavicon(int index)
{
    if (index < 0 || index >= static_cast<int>(gTabs.size()))
        return;
    const std::string& url = gTabs[index].url;
    size_t pos = url.find("://");
    if (pos == std::string::npos)
        return;
    size_t hostStart = pos + 3;
    size_t hostEnd = url.find('/', hostStart);
    std::string host = url.substr(hostStart, hostEnd - hostStart);
    std::string scheme = url.substr(0, pos);
    std::string favUrl = scheme + "://" + host + "/favicon.ico";
    std::wstring wFavUrl(favUrl.begin(), favUrl.end());
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    wchar_t file[MAX_PATH];
    GetTempFileNameW(tmp, L"fav", 0, file);
    if (SUCCEEDED(URLDownloadToFileW(nullptr, wFavUrl.c_str(), file, 0, nullptr))) {
        HICON icon = (HICON)LoadImageW(nullptr, file, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
        if (icon)
            ImageList_ReplaceIcon(gTabImages, gTabs[index].imageIndex, icon);
    }
}

static void DidReceiveTitleForFrame(WKPageRef page, WKStringRef title, WKFrameRef, WKTypeRef, const void*)
{
    int index = FindTabByPage(page);
    if (index < 0)
        return;
    size_t len = WKStringGetMaximumUTF8CStringSize(title);
    std::vector<char> buf(len);
    WKStringGetUTF8CString(title, buf.data(), len);
    wchar_t wbuf[256];
    MultiByteToWideChar(CP_UTF8, 0, buf.data(), -1, wbuf, 256);

    TCITEMW tie{};
    tie.mask = TCIF_TEXT;
    tie.pszText = wbuf;
    TabCtrl_SetItem(gTabCtrl, index, &tie);

    UpdateFavicon(index);
}

static void CloseTab(HWND hWnd, int index)
{
    if (index < 0 || index >= static_cast<int>(gTabs.size()))
        return;
    HWND child = WKViewGetWindow(gTabs[index].view);
    DestroyWindow(child);
    WKRelease(gTabs[index].view);
    ImageList_Remove(gTabImages, gTabs[index].imageIndex);
    gTabs.erase(gTabs.begin() + index);
    TabCtrl_DeleteItem(gTabCtrl, index);
    if (gTabs.empty()) {
        gCurrentTab = -1;
    } else {
        if (gCurrentTab >= index)
            gCurrentTab--;
        if (gCurrentTab < 0)
            gCurrentTab = 0;
    }
    TabCtrl_SetCurSel(gTabCtrl, gCurrentTab);
    ShowCurrentTab();
    ResizeChildren(hWnd);
}

static void ResizeChildren(HWND hWnd)
{
    RECT rc; GetClientRect(hWnd, &rc);
    MoveWindow(gTabCtrl, 0, 0, rc.right - 60, TAB_HEIGHT, TRUE);
    MoveWindow(gNewTabBtn, rc.right - 60, 0, 30, TAB_HEIGHT, TRUE);
    MoveWindow(gCloseTabBtn, rc.right - 30, 0, 30, TAB_HEIGHT, TRUE);

    int y = TAB_HEIGHT;
    MoveWindow(gBackBtn, 0, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gForwardBtn, 30, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gRefreshBtn, 60, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gSettingsBtn, rc.right - 30, y, 30, NAV_HEIGHT, TRUE);
    MoveWindow(gUrlBar, 90, y, rc.right - 120, NAV_HEIGHT, TRUE);

    int top = TAB_HEIGHT + NAV_HEIGHT;
    for (auto& t : gTabs) {
        HWND child = WKViewGetWindow(t.view);
        MoveWindow(child, 0, top, rc.right, rc.bottom - top, TRUE);
    }
}

static void ShowCurrentTab()
{
    for (size_t i = 0; i < gTabs.size(); ++i) {
        HWND child = WKViewGetWindow(gTabs[i].view);
        ShowWindow(child, i == (size_t)gCurrentTab ? SW_SHOW : SW_HIDE);
    }
}

static void NavigateCurrent(const char* url)
{
    if (gCurrentTab < 0) return;
    gTabs[gCurrentTab].url = url;
    WKURLRef wkurl = WKURLCreateWithUTF8CString(url);
    WKPageLoadURL(WKViewGetPage(gTabs[gCurrentTab].view), wkurl);
}

static void AddTab(HWND hWnd, const char* url)
{
    RECT rc; GetClientRect(hWnd, &rc);
    int top = TAB_HEIGHT + NAV_HEIGHT;
    RECT webRect{ 0, top, rc.right, rc.bottom };

    WKPageConfigurationRef cfg = WKPageConfigurationCreate();
    WKContextRef ctx = WKContextCreateWithConfiguration(nullptr);
    WKPageConfigurationSetContext(cfg, ctx);
    WKViewRef view = WKViewCreate(webRect, cfg, hWnd);
    HWND child = WKViewGetWindow(view);
    ShowWindow(child, SW_HIDE);
    WKViewSetIsInWindow(view, true);

    EnsureLoaderClient();
    WKPageRef page = WKViewGetPage(view);
    WKPageSetPageLoaderClient(page, &gLoaderClient.base);
    WKStringRef ua = WKStringCreateWithUTF8CString(kUserAgent);
    WKPageSetCustomUserAgent(page, ua);
    WKRelease(ua);

    int imgIndex = ImageList_AddIcon(gTabImages, LoadIconW(NULL, IDI_APPLICATION));
    gTabs.push_back({ view, url ? url : "", imgIndex });
    int index = static_cast<int>(gTabs.size()) - 1;

    TCITEMW tie{};
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    wchar_t text[32];
    swprintf(text, 32, L"Tab %d", index + 1);
    tie.pszText = text;
    tie.iImage = imgIndex;
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
        HWND parent = GetParent(hwnd);
        SendMessageW(parent, WM_COMMAND, MAKEWPARAM(1004, EN_RETURN), (LPARAM)hwnd);
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
        gTabImages = ImageList_Create(16, 16, ILC_COLOR32, 10, 10);
        TabCtrl_SetImageList(gTabCtrl, gTabImages);
        gNewTabBtn = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1006, nullptr, nullptr);
        gCloseTabBtn = CreateWindowW(L"BUTTON", L"x", WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hWnd, (HMENU)1008, nullptr, nullptr);
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
            if (gCurrentTab >= 0) WKPageGoBack(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1002:
            if (gCurrentTab >= 0) WKPageGoForward(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1003:
            if (gCurrentTab >= 0) WKPageReload(WKViewGetPage(gTabs[gCurrentTab].view));
            break;
        case 1005:
            MessageBoxW(hWnd, L"Settings not implemented", L"Settings", MB_OK);
            break;
        case 1006:
            AddTab(hWnd, "https://google.com");
            break;
        case 1008:
            CloseTab(hWnd, gCurrentTab);
            break;
        case 1004:
            if (HIWORD(wParam) == EN_RETURN) {
                wchar_t wbuf[2048];
                GetWindowTextW(gUrlBar, wbuf, 2048);
                char buf[2048];
                WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), nullptr, nullptr);
                NavigateCurrent(buf);
            }
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
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
        if (hit == HTCLIENT) {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            if (pt.y < TAB_HEIGHT)
                return HTCAPTION;
        }
        return hit;
    }
    case WM_DESTROY:
        if (gTabImages)
            ImageList_Destroy(gTabImages);
        PostQuitMessage(0);
        return 0;
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

    RegisterClassExW(&wc);

    HWND win = CreateWindowExW(0, cls, L"", WS_OVERLAPPEDWINDOW & ~WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInst, nullptr);
    ShowWindow(win, nCmdShow);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}