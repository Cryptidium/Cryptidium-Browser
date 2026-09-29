#pragma once
#include "MainWindow.g.h"
#include <string>
#include <vector>
#include <WebKit/WebKit2_C.h>

namespace winrt::Cryptidium::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void OnAddTab(Microsoft::UI::Xaml::Controls::TabView const&, Windows::Foundation::IInspectable const&);
        void OnTabCloseRequested(Microsoft::UI::Xaml::Controls::TabView const&, Microsoft::UI::Xaml::Controls::TabViewTabCloseRequestedEventArgs const&);
        void OnTabSelectionChanged(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const&);
        void OnBack(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnForward(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnRefresh(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnSettings(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnUrlKeyDown(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::Input::KeyRoutedEventArgs const&);
        void OnUrlGotFocus(Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::RoutedEventArgs const&);
        void OnWebHostLayoutUpdated(Windows::Foundation::IInspectable const&, Windows::Foundation::IInspectable const&);

        // Called from WebKit client callbacks and the window subclass
        void PageChanged(WKPageRef page);
        WKPageRef CurrentPage();
        HWND Hwnd() const { return m_hwnd; }

    private:
        struct Tab
        {
            WKViewRef view;
            Microsoft::UI::Xaml::Controls::TabViewItem item;
            std::string iconHost;
        };

        void InitializeSharedContext();
        void AddTab(const char* url);
        void CloseTab(Microsoft::UI::Xaml::Controls::TabViewItem const& item);
        void NavigateCurrent(const char* url);
        void ShowCurrentTab();
        void UpdateWebLayout();
        RECT WebRect();
        void UpdateUrlBar(WKPageRef page);
        void UpdateFavicon(WKViewRef view, const std::string& host);
        int FindTabByItem(Windows::Foundation::IInspectable const& item);
        int FindTabByPage(WKPageRef page);
        void OnClosed();

        HWND m_hwnd{};
        RECT m_lastRect{};
        std::vector<Tab> m_tabs;
        int m_current{ -1 };
    };
}

namespace winrt::Cryptidium::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
