#include "pch.h"
#include "SettingsWindow.xaml.h"
#if __has_include("SettingsWindow.g.cpp")
#include "SettingsWindow.g.cpp"
#endif
#include "buildinfo.h"
#include "settings.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

namespace
{
    winrt::Cryptidium::SettingsWindow gSettings{ nullptr };
}

void ShowSettings()
{
    if (gSettings) {
        gSettings.Activate();
        return;
    }
    gSettings = winrt::make<winrt::Cryptidium::implementation::SettingsWindow>();
    gSettings.Closed([](auto&&, auto&&) { gSettings = nullptr; });
    gSettings.Activate();
}

void CloseSettings()
{
    if (gSettings) {
        auto w = gSettings;
        gSettings = nullptr;
        w.Close();
    }
}

namespace winrt::Cryptidium::implementation
{
    SettingsWindow::SettingsWindow()
    {
        InitializeComponent();
        Title(L"Settings");
        AppWindow().Resize({ 560, 420 });

        VersionText().Text(L"Version: " + hstring{ BuildInfo::kVersionFormatted });
        PathBox().Text(GetDownloadPath());
        AskCheck().IsChecked(GetAskDownloadLocation());
    }

    void SettingsWindow::ShowStatus(hstring const& message)
    {
        Status().Message(message);
        Status().IsOpen(true);
    }

    void SettingsWindow::OnClearCookies(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        ClearCookies();
        ShowStatus(L"Cookies cleared.");
    }

    void SettingsWindow::OnClearCache(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        ClearCache();
        ShowStatus(L"Cache cleared.");
    }

    void SettingsWindow::OnBrowse(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        HWND hwnd{};
        check_hresult(this->try_as<::IWindowNative>()->get_WindowHandle(&hwnd));

        BROWSEINFOW bi{};
        bi.hwndOwner = hwnd;
        bi.lpszTitle = L"Select Download Folder";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl)
            return;
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) {
            PathBox().Text(path);
            SetDownloadPath(path);
        }
        CoTaskMemFree(pidl);
    }

    void SettingsWindow::OnAskToggled(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        SetAskDownloadLocation(AskCheck().IsChecked().GetBoolean());
    }
}
