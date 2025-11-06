#pragma once

namespace BuildInfo {
    constexpr bool kIsBeta = true;

    constexpr unsigned kBetaBuild = 2;

    constexpr wchar_t kVersionFormatted[] = L"1.0 Beta 2";

    constexpr wchar_t kVersionRaw[] = L"1.0";

    void ProcessUpdates();
}
