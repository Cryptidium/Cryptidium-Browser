#include "buildinfo.h"
#include <windows.h>
#include <urlmon.h>
#include <fstream>
#include <string>
#include <cwchar>

#pragma comment(lib, "Urlmon.lib")

namespace {

std::wstring DownloadFirstLine(const wchar_t* url) {
    wchar_t tmpPath[MAX_PATH];
    if (FAILED(URLDownloadToCacheFileW(nullptr, url, tmpPath, MAX_PATH, 0, nullptr)))
        return L"";
    std::wifstream file(tmpPath);
    std::wstring line;
    std::getline(file, line);
    if (!line.empty() && line.back() == L'\r')
        line.pop_back();
    DeleteFileW(tmpPath);
    return line;
}

bool IsNewerVersion(const std::wstring& latest, const wchar_t* current) {
    unsigned lm = 0, lmi = 0, lp = 0;
    unsigned cm = 0, cmi = 0, cp = 0;
    if (swscanf(latest.c_str(), L"%u.%u.%u", &lm, &lmi, &lp) != 3)
        return false;
    if (swscanf(current, L"%u.%u.%u", &cm, &cmi, &cp) != 3)
        return false;
    if (lm != cm) return lm > cm;
    if (lmi != cmi) return lmi > cmi;
    return lp > cp;
}

} // namespace

void BuildInfo::ProcessUpdates() {
    std::wstring base = L"https://cryptidium.vercel.app/UpdateUtility/";
    std::wstring latest = DownloadFirstLine((base + (kIsBeta ? L"latestbeta" : L"lateststable")).c_str());
    std::wstring name = DownloadFirstLine((base + (kIsBeta ? L"latestbetaformatted" : L"lateststableformatted")).c_str());
    if (latest.empty() || name.empty())
        return;

    bool needsUpdate = false;
    if (IsNewerVersion(latest, kVersionRaw)) {
        needsUpdate = true;
    } else if (kIsBeta) {
        std::wstring build = DownloadFirstLine((base + L"latestbetabuild").c_str());
        if (!build.empty()) {
            unsigned lb = 0;
            if (swscanf(build.c_str(), L"%u", &lb) == 1 && lb > kBetaBuild)
                needsUpdate = true;
        }
    }

    if (needsUpdate) {
        const wchar_t* site = kIsBeta ? L"https://cryptidium.vercel.app/beta" : L"https://cryptidium.vercel.app";
        std::wstring msg = L"A new update (" + name + L") is available. Visit " + site + L" to download.";
        MessageBoxW(nullptr, msg.c_str(), L"Update available", MB_OK | MB_ICONINFORMATION);
    }
}

