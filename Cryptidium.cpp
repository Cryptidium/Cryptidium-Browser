#include <windows.h>
#include <winternl.h>
#include <shellapi.h>
#include <string>
#include "gui.h"

// Function to check if Windows version is supported
static bool IsWindowsVersionSupported() {
    typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return false;
    
    RtlGetVersionPtr RtlGetVersion = (RtlGetVersionPtr)GetProcAddress(ntdll, "RtlGetVersion");
    if (!RtlGetVersion)
        return false;
    
    RTL_OSVERSIONINFOW osvi{};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    
    NTSTATUS status = RtlGetVersion(&osvi);
    if (!NT_SUCCESS(status))
        return false;
    
    // Check if Windows 11 (version 10.0 with build >= 22000)
    if (osvi.dwMajorVersion != 10 || osvi.dwMinorVersion != 0)
        return false;
    
    if (osvi.dwBuildNumber < 22000)
        return false;
    
    // Check if build is 23H2 (22631) or newer
    // Supported: 23H2 (22631), 24H2 (26100), 25H2 (future), etc.
    if (osvi.dwBuildNumber < 22631)
        return false;
    
    return true;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    // Check Windows version before proceeding
    if (!IsWindowsVersionSupported()) {
        MessageBoxW(nullptr,
            L"This application requires Windows 11 version 23H2 (build 22631) or newer.\n"
            L"Supported versions: 23H2, 24H2, 25H2 or newer.\n\n"
            L"Please update your operating system to run this application.",
            L"Unsupported Windows Version",
            MB_OK | MB_ICONERROR);
        return 1;
    }
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
