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
            std::string buf(len, '\0');
            WideCharToMultiByte(CP_UTF8, 0, arg.c_str(), -1, buf.data(), len, nullptr, nullptr);
            buf.resize(len - 1);
            url = std::move(buf);
        } else {
            wchar_t full[MAX_PATH];
            if (GetFullPathNameW(arg.c_str(), MAX_PATH, full, nullptr)) {
                int len = WideCharToMultiByte(CP_UTF8, 0, full, -1, nullptr, 0, nullptr, nullptr);
                std::string tmp(len, '\0');
                WideCharToMultiByte(CP_UTF8, 0, full, -1, tmp.data(), len, nullptr, nullptr);
                tmp.resize(len - 1);
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
