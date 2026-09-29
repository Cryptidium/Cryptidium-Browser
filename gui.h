#pragma once
#include <string>
#include <WebKit/WebKit2_C.h>

WKContextRef GetCurrentContext();
void SetStartupUrl(const char* url);
