#include "pch.h"
#include "settings.h"
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKWebsiteDataStoreRef.h>
#include <WebKit/WKHTTPCookieStoreRef.h>
#include <WebKit/WKResourceCacheManager.h>
#include "gui.h"

#pragma comment(lib, "Shell32.lib")

// Registry key for settings
static const wchar_t* REG_KEY = L"Software\\Cryptidium";
static const wchar_t* REG_DOWNLOAD_PATH = L"DownloadPath";
static const wchar_t* REG_ASK_DOWNLOAD = L"AskDownloadLocation";

std::wstring GetDownloadPath() {
    HKEY hKey;
    std::wstring result;
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        DWORD type;
        
        if (RegQueryValueExW(hKey, REG_DOWNLOAD_PATH, nullptr, &type, 
                            (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
            result = buffer;
        }
        RegCloseKey(hKey);
    }
    
    // Default to Downloads folder if not set
    if (result.empty()) {
        PWSTR path = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path))) {
            result = path;
            CoTaskMemFree(path);
        }
    }
    
    return result;
}

void SetDownloadPath(const std::wstring& path) {
    HKEY hKey;
    DWORD disposition;
    
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, nullptr, 0,
                       KEY_WRITE, nullptr, &hKey, &disposition) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, REG_DOWNLOAD_PATH, 0, REG_SZ,
                      (const BYTE*)path.c_str(), static_cast<DWORD>((path.length() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}

bool GetAskDownloadLocation() {
    HKEY hKey;
    bool result = false;  // Default to not asking
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        DWORD type;
        
        if (RegQueryValueExW(hKey, REG_ASK_DOWNLOAD, nullptr, &type,
                            (LPBYTE)&value, &size) == ERROR_SUCCESS && type == REG_DWORD) {
            result = (value != 0);
        }
        RegCloseKey(hKey);
    }
    
    return result;
}

void SetAskDownloadLocation(bool ask) {
    HKEY hKey;
    DWORD disposition;
    
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, nullptr, 0,
                       KEY_WRITE, nullptr, &hKey, &disposition) == ERROR_SUCCESS) {
        DWORD value = ask ? 1 : 0;
        RegSetValueExW(hKey, REG_ASK_DOWNLOAD, 0, REG_DWORD,
                      (const BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

void ClearCookies() {
    WKWebsiteDataStoreRef store = WKWebsiteDataStoreGetDefaultDataStore();
    WKHTTPCookieStoreRef cookies = WKWebsiteDataStoreGetHTTPCookieStore(store);
    WKHTTPCookieStoreDeleteAllCookies(cookies, nullptr, nullptr);
}

void ClearCache() {
    WKContextRef ctx = GetCurrentContext();
    if (!ctx)
        return;
    WKResourceCacheManagerRef manager = WKContextGetResourceCacheManager(ctx);
    WKResourceCacheManagerClearCacheForAllOrigins(manager, WKResourceCachesToClearAll);
}

