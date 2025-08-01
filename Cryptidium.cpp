#include <windows.h>
#include <cstdlib>
#include <WebKit/WebKit2_C.h>

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
            WKViewSetFrame(gView, &rc);
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow)
{
    const wchar_t cls[] = L"Cryptidium";
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cls;
    RegisterClassExW(&wc);

    HWND win = CreateWindowW(cls, L"Cryptidium", WS_OVERLAPPEDWINDOW,
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
