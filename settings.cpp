#include "settings.h"
#include <WebKit/WebKit2_C.h>
#include <WebKit/WKWebsiteDataStoreRef.h>
#include <WebKit/WKHTTPCookieStoreRef.h>
#include <WebKit/WKResourceCacheManager.h>
#include <shlobj.h>
#include <commdlg.h>
#include "buildinfo.h"
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
        wchar_t path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, path))) {
            result = std::wstring(path) + L"\\Downloads";
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
                      (const BYTE*)path.c_str(), (path.length() + 1) * sizeof(wchar_t));
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

static HWND gDownloadPathEdit = nullptr;
static HWND gAskDownloadCheck = nullptr;

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
        HFONT font = GetUIFont();
        
        int yPos = 10;
        
        // Version label
        HWND label = CreateWindowW(L"STATIC", ver, WS_CHILD | WS_VISIBLE, 10, yPos, 250, 20, hwnd, nullptr, nullptr, nullptr);
        SendMessageW(label, WM_SETFONT, (WPARAM)font, TRUE);
        yPos += 30;
        
        // Cookie and cache buttons
        HWND cookieBtn = CreateWindowW(L"BUTTON", L"Clear Cookies", WS_CHILD | WS_VISIBLE, 10, yPos, 110, 24, hwnd, (HMENU)2001, nullptr, nullptr);
        SendMessageW(cookieBtn, WM_SETFONT, (WPARAM)font, TRUE);
        HWND cacheBtn = CreateWindowW(L"BUTTON", L"Clear Cache", WS_CHILD | WS_VISIBLE, 130, yPos, 110, 24, hwnd, (HMENU)2002, nullptr, nullptr);
        SendMessageW(cacheBtn, WM_SETFONT, (WPARAM)font, TRUE);
        yPos += 34;
        
        // Download settings section
        HWND dlLabel = CreateWindowW(L"STATIC", L"Download Settings:", WS_CHILD | WS_VISIBLE, 10, yPos, 250, 20, hwnd, nullptr, nullptr, nullptr);
        SendMessageW(dlLabel, WM_SETFONT, (WPARAM)font, TRUE);
        yPos += 25;
        
        // Download path label
        HWND pathLabel = CreateWindowW(L"STATIC", L"Download Location:", WS_CHILD | WS_VISIBLE, 10, yPos, 120, 20, hwnd, nullptr, nullptr, nullptr);
        SendMessageW(pathLabel, WM_SETFONT, (WPARAM)font, TRUE);
        yPos += 22;
        
        // Download path edit box
        gDownloadPathEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
                                            10, yPos, 300, 22, hwnd, nullptr, nullptr, nullptr);
        SendMessageW(gDownloadPathEdit, WM_SETFONT, (WPARAM)font, TRUE);
        std::wstring path = GetDownloadPath();
        SetWindowTextW(gDownloadPathEdit, path.c_str());
        
        // Browse button
        HWND browseBtn = CreateWindowW(L"BUTTON", L"Browse...", WS_CHILD | WS_VISIBLE, 320, yPos, 80, 22, hwnd, (HMENU)2003, nullptr, nullptr);
        SendMessageW(browseBtn, WM_SETFONT, (WPARAM)font, TRUE);
        yPos += 32;
        
        // Ask download location checkbox
        gAskDownloadCheck = CreateWindowW(L"BUTTON", L"Ask where to save each file before downloading", 
                                          WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, yPos, 400, 20, hwnd, (HMENU)2004, nullptr, nullptr);
        SendMessageW(gAskDownloadCheck, WM_SETFONT, (WPARAM)font, TRUE);
        SendMessageW(gAskDownloadCheck, BM_SETCHECK, GetAskDownloadLocation() ? BST_CHECKED : BST_UNCHECKED, 0);
        
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
        case 2003: { // Browse button
            BROWSEINFOW bi{};
            bi.hwndOwner = hwnd;
            bi.lpszTitle = L"Select Download Folder";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, path)) {
                    SetWindowTextW(gDownloadPathEdit, path);
                    SetDownloadPath(path);
                }
                CoTaskMemFree(pidl);
            }
            break;
        }
        case 2004: { // Ask download checkbox
            bool checked = (SendMessageW(gAskDownloadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            SetAskDownloadLocation(checked);
            break;
        }
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
                             CW_USEDEFAULT, CW_USEDEFAULT, 420, 240, parent, nullptr, nullptr, nullptr);
    ShowWindow(wnd, SW_SHOW);
}

