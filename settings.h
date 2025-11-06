#pragma once
#include <windows.h>
#include <string>

void ShowSettings(HWND parent);

// Download settings
std::wstring GetDownloadPath();
void SetDownloadPath(const std::wstring& path);
bool GetAskDownloadLocation();
void SetAskDownloadLocation(bool ask);
