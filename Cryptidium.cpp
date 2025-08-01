#include <windows.h>
#include <cstdlib>
#include <WebKit/WebKit2_C.h>
#include "gui.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    return RunBrowser(hInst, nCmdShow);
}

static WKViewRef gView;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        RECT rc; GetClientRect(hWnd, &rc);
        WKPageConfigurationRef cfg = WKPageConfigurationCreate();
        WKContextRef ctx = WKContextCreateWithConfiguration(nullptr);
        WKPageConfigurationSetContext(cfg, ctx);
        gView = WKViewCreate(rc, cfg, hWnd);
        HWND child = WKViewGetWindow(gView);
        ShowWindow(child, SW_SHOW);
        WKViewSetIsInWindow(gView, true);

        int argc; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        const char* def = "https://google.com";
        const char* url;
        if (argc > 1) {
            char buf[2048];
            WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, buf, sizeof(buf), nullptr, nullptr);
            url = _strdup(buf);
        }
        else {
            url = def;
        }
        WKURLRef wkurl = WKURLCreateWithUTF8CString(url);
        WKPageLoadURL(WKViewGetPage(gView), wkurl);
        return 0;
    }
    case WM_SIZE: {
        if (gView) {
            RECT rc; GetClientRect(hWnd, &rc);
            HWND child = WKViewGetWindow(gView);
            MoveWindow(child,
                rc.left, rc.top,
                rc.right - rc.left,
                rc.bottom - rc.top,
                TRUE);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
