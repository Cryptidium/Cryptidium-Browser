#pragma once
#include <windows.h>
#include <WebKit/WebKit2_C.h>

int RunBrowser(HINSTANCE hInstance, int nCmdShow, const char* initialUrl = nullptr);
WKContextRef GetCurrentContext();
HFONT GetUIFont();
