#pragma once
#include <string>

void ShowSettings();
void CloseSettings();

// Download settings
std::wstring GetDownloadPath();
void SetDownloadPath(const std::wstring& path);
bool GetAskDownloadLocation();
void SetAskDownloadLocation(bool ask);

// Browsing data
void ClearCookies();
void ClearCache();
