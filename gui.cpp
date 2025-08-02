#include "gui.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <urlmon.h>
#include <vector>
#include <WebKit/WebKit2_C.h>
#include <string>
#include "buildinfo.h"
#include "settings.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Urlmon.lib")

static std::string MakeUserAgent()
{
    std::wstring wver = BuildInfo::kVersionRaw;
    return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.6 Safari/605.1.15 Cryptidium/" +
           std::string(wver.begin(), wver.end());
}

static std::string gUserAgent = MakeUserAgent();
static HFONT gUIFont;

HFONT GetUIFont()
{
    return gUIFont;
}

struct Tab {
    WKViewRef view;
    HWND closeBtn;
    int imageIndex;
};

static std::vector<Tab> gTabs;
static int gCurrentTab = -1;

static HWND gTabCtrl;
static HWND gUrlBar;
static HWND gBackBtn;
static HWND gForwardBtn;
static HWND gRefreshBtn;
static HWND gNewTabBtn;
static HWND gSettingsBtn;
static HIMAGELIST gTabImages;

static const int TAB_HEIGHT = 24;
static const int NAV_HEIGHT = 28;

static bool gTabWidthAdjusted = false;

WKContextRef GetCurrentContext()
{
    if (gCurrentTab >= 0)
        return WKPageGetContext(WKViewGetPage(gTabs[gCurrentTab].view));
    return nullptr;
}

static void UpdateUrlBarFromPage(WKPageRef page)
{
    WKURLRef url = WKPageCopyActiveURL(page);
    if (!url)
        return;
    WKStringRef str = WKURLCopyString(url);
    size_t max = WKStringGetMaximumUTF8CStringSize(str);
    std::string tmp(max, '\0');
    WKStringGetUTF8CString(str, tmp.data(), max);
    std::string text = tmp.c_str();
    WKRelease(str);
    WKRelease(url);
    if (text.rfind("https://", 0) == 0)
        text.erase(0, 8);
    std::wstring wtext(text.begin(), text.end());
    SetWindowTextW(gUrlBar, wtext.c_str());
}

static std::string GetHostFromURL(const std::string& url)
{
    size_t scheme_end = url.find("://");
    if (scheme_end == std::string::npos)
        return {};
    size_t host_start = scheme_end + 3;
    size_t host_end = url.find('/', host_start);
    return url.substr(host_start, host_end - host_start);
}

static void UpdateTabFromPage(WKPageRef page)
{
    for (size_t i = 0; i < gTabs.size(); ++i) {
        if (WKViewGetPage(gTabs[i].view) != page)
            continue;

        WKStringRef titleRef = WKPageCopyTitle(page);
        size_t tmax = WKStringGetMaximumUTF8CStringSize(titleRef);
        std::string tbuf(tmax, '\0');
        WKStringGetUTF8CString(titleRef, tbuf.data(), tmax);
        std::string title = tbuf.c_str();
        std::wstring wtitle(title.begin(), title.end());
        WKRelease(titleRef);
        TCITEMW tie{};
        tie.mask = TCIF_TEXT;
        tie.pszText = wtitle.data();
        TabCtrl_SetItem(gTabCtrl, i, &tie);

        WKURLRef urlRef = WKPageCopyActiveURL(page);
        if (urlRef) {
            WKStringRef str = WKURLCopyString(urlRef);
            size_t umax = WKStringGetMaximumUTF8CStringSize(str);
            std::string ubuf(umax, '\0');
            WKStringGetUTF8CString(str, ubuf.data(), umax);
            WKRelease(str);
            WKRelease(urlRef);
            std::string url = ubuf.c_str();
            std::string host = GetHostFromURL(url);
            if (!host.empty()) {
                std::string iconUrl = "https://" + host + "/favicon.ico";
                std::wstring wicon(iconUrl.begin(), iconUrl.end());
                wchar_t cachePath[MAX_PATH];
                if (SUCCEEDED(URLDownloadToCacheFileW(nullptr, wicon.c_str(), cachePath, MAX_PATH, 0, nullptr))) {
                    HICON hico = (HICON)LoadImageW(nullptr, cachePath, IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
                    if (hico) {
                        ImageList_ReplaceIcon(gTabImages, gTabs[i].imageIndex, hico);
                        DestroyIcon(hico);
                    }
                }
            }
        }
        break;
    }
}


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
    MoveWindow(gUrlBar, 90, y, rc.right - 120, NAV_HEIGHT, TRUE);
    MoveWindow(gSettingsBtn, rc.right - 30, y, 30, NAV_HEIGHT, TRUE);
    int top = TAB_HEIGHT + NAV_HEIGHT;
    for (auto& t : gTabs) {
        HWND child = WKViewGetWindow(t.view);
        MoveWindow(child, 0, top, rc.right, rc.bottom - top, TRUE);
    }
    for (size_t i = 0; i < gTabs.size(); ++i) {
        RECT tr;
        TabCtrl_GetItemRect(gTabCtrl, i, &tr);
        MapWindowPoints(gTabCtrl, hWnd, (POINT*)&tr, 2);
        MoveWindow(gTabs[i].closeBtn, tr.right - 5, tr.top + 4, 16, 16, TRUE);
        SetWindowPos(gTabs[i].closeBtn, HWND_TOP, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE);
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
    WKPageRef page = WKViewGetPage(view);
    WKStringRef ua = WKStringCreateWithUTF8CString(gUserAgent.c_str());
    WKPageSetCustomUserAgent(page, ua);
    WKRelease(ua);
    static WKPageNavigationClientV0 navClient;
    navClient.base.version = 0;
    navClient.didFinishNavigation = [](WKPageRef p, WKNavigationRef, WKTypeRef, const void*) {
        UpdateUrlBarFromPage(p);
        UpdateTabFromPage(p);
    };
    WKPageSetPageNavigationClient(page, &navClient.base);
    HWND child = WKViewGetWindow(view);
    ShowWindow(child, SW_HIDE);
    WKViewSetIsInWindow(view, true);
    HICON ph = (HICON)LoadImageW(nullptr, L"assets\\app.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    int img = ImageList_AddIcon(gTabImages, ph);
    DestroyIcon(ph);
    HWND closeBtn = CreateWindowW(L"BUTTON", L"x", WS_CHILD | WS_VISIBLE,
                                  0, 0, 16, 16, hWnd, nullptr, nullptr, nullptr);
    SendMessageW(closeBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
    gTabs.push_back({ view, closeBtn, img });
    int index = static_cast<int>(gTabs.size()) - 1;
    TCITEMW tie{};
    tie.mask = TCIF_TEXT | TCIF_IMAGE;
    tie.iImage = img;
    tie.pszText = const_cast<wchar_t*>(L"Loading");
    TabCtrl_InsertItem(gTabCtrl, index, &tie);
    if (!gTabWidthAdjusted) {
        RECT tr;
        TabCtrl_GetItemRect(gTabCtrl, index, &tr);
        TabCtrl_SetMinTabWidth(gTabCtrl, (tr.right - tr.left) + 15);
        gTabWidthAdjusted = true;
    }
    gCurrentTab = index;
    ShowCurrentTab();
    if (url) {
        NavigateCurrent(url);
    }
    TabCtrl_SetCurSel(gTabCtrl, gCurrentTab);
    ResizeChildren(hWnd);
}

static void CloseTab(HWND hWnd, int index)
{
    if (index < 0 || index >= static_cast<int>(gTabs.size()))
        return;
    DestroyWindow(gTabs[index].closeBtn);
    WKViewRef view = gTabs[index].view;
    HWND child = WKViewGetWindow(view);
    DestroyWindow(child);
    WKRelease(view);
    gTabs.erase(gTabs.begin() + index);
    TabCtrl_DeleteItem(gTabCtrl, index);
    if (gTabs.empty()) {
        PostMessageW(hWnd, WM_CLOSE, 0, 0);
        return;
    }
    if (gCurrentTab >= index)
        gCurrentTab--;
    ShowCurrentTab();
    TabCtrl_SetCurSel(gTabCtrl, gCurrentTab);
    if (gCurrentTab >= 0)
        UpdateUrlBarFromPage(WKViewGetPage(gTabs[gCurrentTab].view));
    else
        SetWindowTextW(gUrlBar, L"");
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
        std::string input = buf;
        std::string display = input;
        std::string url;
        if (input.find(' ') != std::string::npos) {
            for (char& c : input)
                if (c == ' ')
                    c = '+';
            url = "https://www.google.com/search?q=" + input;
        } else {
            if (input.rfind("https://", 0) == 0)
                input.erase(0, 8);
            url = "https://" + input;
            display = input;
        }
        std::wstring wdisp(display.begin(), display.end());
        SetWindowTextW(gUrlBar, wdisp.c_str());
        NavigateCurrent(url.c_str());
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        InitCommonControls();
        gUIFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        gTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
                                   0, 0, 0, 0, hWnd, (HMENU)1000, nullptr, nullptr);
        SendMessageW(gTabCtrl, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gTabImages = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 1, 1);
        TabCtrl_SetImageList(gTabCtrl, gTabImages);
        gNewTabBtn = CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE,
                                   0, 0, 0, 0, hWnd, (HMENU)1001, nullptr, nullptr);
        SendMessageW(gNewTabBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gBackBtn = CreateWindowW(L"BUTTON", L"<", WS_CHILD | WS_VISIBLE,
                                 0, 0, 0, 0, hWnd, (HMENU)1002, nullptr, nullptr);
        SendMessageW(gBackBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gForwardBtn = CreateWindowW(L"BUTTON", L">", WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0, hWnd, (HMENU)1003, nullptr, nullptr);
        SendMessageW(gForwardBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gRefreshBtn = CreateWindowW(L"BUTTON", L"R", WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0, hWnd, (HMENU)1004, nullptr, nullptr);
        SendMessageW(gRefreshBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gSettingsBtn = CreateWindowW(L"BUTTON", L"S", WS_CHILD | WS_VISIBLE,
                                     0, 0, 0, 0, hWnd, (HMENU)1006, nullptr, nullptr);
        SendMessageW(gSettingsBtn, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        gUrlBar = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                  0, 0, 0, 0, hWnd, (HMENU)1005, nullptr, nullptr);
        SendMessageW(gUrlBar, WM_SETFONT, (WPARAM)gUIFont, TRUE);
        SetWindowSubclass(gUrlBar, UrlBarProc, 0, 0);
        AddTab(hWnd, "https://google.com");
        return 0;
    }
    case WM_SIZE:
        ResizeChildren(hWnd);
        return 0;
    case WM_COMMAND: {
        if ((HWND)lParam) {
            for (size_t i = 0; i < gTabs.size(); ++i) {
                if ((HWND)lParam == gTabs[i].closeBtn) {
                    CloseTab(hWnd, static_cast<int>(i));
                    return 0;
                }
            }
        }
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
        case 1006:
            ShowSettings(hWnd);
            break;
        }
        return 0;
    }
    case WM_NOTIFY: {
        LPNMHDR nm = (LPNMHDR)lParam;
        if (nm->hwndFrom == gTabCtrl && nm->code == TCN_SELCHANGE) {
            gCurrentTab = TabCtrl_GetCurSel(gTabCtrl);
            ShowCurrentTab();
            if (gCurrentTab >= 0)
                UpdateUrlBarFromPage(WKViewGetPage(gTabs[gCurrentTab].view));
        }
        return 0;
    }
    case WM_DESTROY:
        ImageList_Destroy(gTabImages);
        DeleteObject(gUIFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int RunBrowser(HINSTANCE hInst, int nCmdShow)
{
    BuildInfo::ProcessUpdates();
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