#include "settings.h"
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKWebsiteDataStoreRef.h>
#include <WebKit/WKHTTPCookieStoreRef.h>
#include <WebKit/WKResourceCacheManager.h>
#include "buildinfo.h"
#include "gui.h"

static void ClearCookies() {
    WKWebsiteDataStoreRef store = WKWebsiteDataStoreGetDefaultDataStore();
    WKHTTPCookieStoreRef cookies = WKWebsiteDataStoreGetHTTPCookieStore(store);
    WKHTTPCookieStoreDeleteAllCookies(cookies, nullptr, nullptr);
}

static void ClearCache() {
    WKContextRef ctx = GetCurrentContext();
    if (!ctx)
        return;
    WKResourceCacheManagerRef manager = WKContextGetResourceCacheManager(ctx);
    WKResourceCacheManagerClearCacheForAllOrigins(manager, WKResourceCachesToClearAll);
}

static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        wchar_t ver[64];
        wsprintfW(ver, L"Version: %s", BuildInfo::kVersionFormatted);
        CreateWindowW(L"STATIC", ver, WS_CHILD | WS_VISIBLE, 10, 10, 250, 20, hwnd, nullptr, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Clear Cookies", WS_CHILD | WS_VISIBLE, 10, 40, 110, 24, hwnd, (HMENU)2001, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"Clear Cache", WS_CHILD | WS_VISIBLE, 130, 40, 110, 24, hwnd, (HMENU)2002, nullptr, nullptr);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 2001:
            ClearCookies();
            MessageBoxW(hwnd, L"Cookies cleared.", L"Settings", MB_OK);
            break;
        case 2002:
            ClearCache();
            MessageBoxW(hwnd, L"Cache cleared.", L"Settings", MB_OK);
            break;
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void ShowSettings(HWND parent) {
    static bool registered = false;
    static const wchar_t CLASS_NAME[] = L"SettingsWindow";
    if (!registered) {
        WNDCLASSW wc{};
        wc.lpfnWndProc = SettingsWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        RegisterClassW(&wc);
        registered = true;
    }
    HWND wnd = CreateWindowW(CLASS_NAME, L"Settings", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT, 260, 120, parent, nullptr, nullptr, nullptr);
    ShowWindow(wnd, SW_SHOW);
}

