#pragma once

namespace BuildInfo {
    constexpr bool kIsBeta = true;

    // Incremented for each beta build; ignored for stable releases.
    constexpr unsigned kBetaBuild = 1;

    constexpr wchar_t kVersionFormatted[] = L"1.0 Beta 1";

    constexpr wchar_t kVersionRaw[] = L"1.0.1";

    // Checks for updates and notifies the user when a newer version exists.
    void ProcessUpdates();
}
