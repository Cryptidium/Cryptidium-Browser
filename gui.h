#pragma once
#include <windows.h>
#include <WebKit/WebKit2_C.h>

int RunBrowser(HINSTANCE hInstance, int nCmdShow);
WKContextRef GetCurrentContext();
HFONT GetUIFont();