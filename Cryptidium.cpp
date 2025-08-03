#include <windows.h>
#include <shellapi.h>
#include <string>
#include "gui.h"

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::string url;
    const char* initial = nullptr;
    if (argv && argc > 1) {
        std::wstring arg = argv[1];
        if (arg.rfind(L"https://", 0) == 0) {
            int len = WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, nullptr, 0, nullptr, nullptr);
            url.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, url.data(), len, nullptr, nullptr);
        } else {
            wchar_t full[MAX_PATH];
            if (GetFullPathNameW(arg.c_str(), MAX_PATH, full, nullptr)) {
                int len = WideCharToMultiByte(CP_UTF8, 0, full, -1, nullptr, 0, nullptr, nullptr);
                std::string tmp(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, full, -1, tmp.data(), len, nullptr, nullptr);
                for (char& c : tmp)
                    if (c == '\\')
                        c = '/';
                url = "file:///" + tmp;
            }
        }
        initial = url.c_str();
    }
    if (argv)
        LocalFree(argv);
    return RunBrowser(hInst, nCmdShow, initial);
}
