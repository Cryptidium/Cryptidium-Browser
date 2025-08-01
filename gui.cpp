#include "gui.h"
#include <windows.h>
#include <WebKit/WebKit2_C.h>

#pragma comment(lib, "Comctl32.lib")

static WKViewRef gView;
static HWND gViewWindow;

static const char kHtml[] = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cryptidium UI</title>
    <script crossorigin src="https://unpkg.com/react@17/umd/react.development.js"></script>
    <script crossorigin src="https://unpkg.com/react-dom@17/umd/react-dom.development.js"></script>
    <style>
        body, html { margin: 0; padding: 0; height: 100%; overflow: hidden; font-family: sans-serif; }
        .tab-bar { display: flex; align-items: center; height: 32px; background: #2e005e; color: white; user-select: none; }
        .tab { padding: 6px 12px; margin-right: 2px; background: rgba(255,255,255,0.1); border-radius: 4px 4px 0 0; cursor: pointer; }
        .tab.active { background: #ffffff; color: #2e005e; }
        .tab .close { margin-left: 6px; color: inherit; cursor: pointer; }
        .new-tab-button { margin-left: auto; padding: 0 12px; cursor: pointer; }
        .nav-bar { display: flex; align-items: center; height: 32px; background: #ececec; padding: 0 4px; }
        .nav-bar button { margin-right: 4px; }
        .nav-bar input { flex: 1; height: 24px; margin-right: 4px; }
        .content { position: absolute; top: 64px; left: 0; right: 0; bottom: 0; }
        .content iframe { width: 100%; height: 100%; border: none; }
    </style>
</head>
<body>
    <div id="root"></div>
    <script>
        const { useState } = React;
        function BrowserApp() {
            const [tabs, setTabs] = useState(() => { return [{ id: 1, url: 'https://google.com' }]; });
            const [currentId, setCurrentId] = useState(1);
            const [urlInput, setUrlInput] = useState('https://google.com');
            const addTab = () => {
                const nextId = tabs.length ? Math.max(...tabs.map(t => t.id)) + 1 : 1;
                const newTab = { id: nextId, url: 'https://google.com' };
                setTabs([...tabs, newTab]);
                setCurrentId(nextId);
                setUrlInput(newTab.url);
            };
            const closeTab = (id) => {
                let newTabs = tabs.filter(t => t.id !== id);
                if (newTabs.length === 0) newTabs = [{ id: 1, url: 'https://google.com' }];
                setTabs(newTabs);
                if (currentId === id) {
                    setCurrentId(newTabs[0].id);
                    setUrlInput(newTabs[0].url);
                }
            };
            const selectTab = (id) => {
                setCurrentId(id);
                const t = tabs.find(t => t.id === id);
                setUrlInput(t.url);
            };
            const navigate = () => {
                setTabs(tabs.map(t => { if (t.id === currentId) return { ...t, url: urlInput }; return t; }));
            };
            const onKeyDown = (e) => { if (e.key === 'Enter') navigate(); };
            return React.createElement('div', { style: { height: '100%', display: 'flex', flexDirection: 'column' } },
                React.createElement('div', { className: 'tab-bar' },
                    tabs.map(tab => React.createElement('div', {
                        key: tab.id,
                        className: 'tab' + (tab.id === currentId ? ' active' : ''),
                        onClick: () => selectTab(tab.id)
                    },
                        'Tab ', tab.id,
                        React.createElement('span', {
                            className: 'close',
                            onClick: (e) => { e.stopPropagation(); closeTab(tab.id); }
                        }, '\u00d7')
                    )),
                    React.createElement('div', { className: 'new-tab-button', onClick: addTab }, '+')
                ),
                React.createElement('div', { className: 'nav-bar' },
                    React.createElement('button', { onClick: () => {} }, '<'),
                    React.createElement('button', { onClick: () => {} }, '>'),
                    React.createElement('button', { onClick: () => {} }, 'R'),
                    React.createElement('input', { value: urlInput, onChange: e => setUrlInput(e.target.value), onKeyDown }),
                    React.createElement('button', { onClick: navigate }, 'Go')
                ),
                React.createElement('div', { className: 'content' },
                    tabs.map(tab => tab.id === currentId ? React.createElement('iframe', { key: tab.id, src: tab.url, sandbox: '' }) : null)
                )
            );
        }
        ReactDOM.render(React.createElement(BrowserApp), document.getElementById('root'));
    </script>
</body>
</html>
)";

static void ResizeChildren(HWND hWnd)
{
    if (!gView)
        return;
    RECT rc;
    GetClientRect(hWnd, &rc);
    MoveWindow(gViewWindow, 0, 0, rc.right, rc.bottom, TRUE);
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
        gView = WKViewCreate(rc, cfg, hWnd);
        gViewWindow = WKViewGetWindow(gView);
        ShowWindow(gViewWindow, SW_SHOW);
        WKViewSetIsInWindow(gView, true);

        WKStringRef htmlStr = WKStringCreateWithUTF8CString(kHtml);
        WKPageLoadHTMLString(WKViewGetPage(gView), htmlStr, nullptr);
        WKRelease(htmlStr);
        return 0;
    }
    case WM_SIZE:
        ResizeChildren(hWnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int RunBrowser(HINSTANCE hInst, int nCmdShow)
{
    const wchar_t cls[] = L"CryptidiumReact";
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = cls;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    HICON icon = (HICON)LoadImageW(nullptr, L"assets\\app.ico", IMAGE_ICON,
                                   0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    wc.hIcon = icon;
    wc.hIconSm = icon;
    RegisterClassExW(&wc);
    HWND win = CreateWindowW(cls, L"Cryptidium", WS_OVERLAPPEDWINDOW,
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