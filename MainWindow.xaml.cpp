#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include "gui.h"
#include "buildinfo.h"
#include "settings.h"
#include "resource.h"
#include <cmath>
#include <thread>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Shell32.lib")

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Controls;

namespace
{
    using Impl = winrt::Cryptidium::implementation::MainWindow;

    Impl* gSelf = nullptr;
    WKContextRef gSharedContext = nullptr;
    std::string gStartupUrl = "https://google.com";

    std::string MakeUserAgent()
    {
        std::wstring wver = BuildInfo::kVersionRaw;
        return "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/26.1 Safari/605.1.15 Cryptidium/" +
               std::string(wver.begin(), wver.end());
    }

    const std::string gUserAgent = MakeUserAgent();

    std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return "";
        std::string result(len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), len, nullptr, nullptr);
        return result;
    }

    std::wstring UTF8ToWide(const std::string& str)
    {
        if (str.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0) return L"";
        std::wstring result(len - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), len);
        return result;
    }

    std::string WKStringToUTF8(WKStringRef str)
    {
        size_t max = WKStringGetMaximumUTF8CStringSize(str);
        std::string buf(max, '\0');
        WKStringGetUTF8CString(str, buf.data(), max);
        return buf.c_str();
    }

    std::string GetHostFromURL(const std::string& url)
    {
        size_t schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos)
            return {};
        size_t hostStart = schemeEnd + 3;
        size_t hostEnd = url.find('/', hostStart);
        return url.substr(hostStart, hostEnd - hostStart);
    }

    std::wstring AssetPath(const wchar_t* name)
    {
        wchar_t exe[MAX_PATH];
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        std::wstring dir = exe;
        dir.erase(dir.find_last_of(L'\\') + 1);
        return dir + L"Assets\\" + name;
    }

    IconSource IconFromFile(const std::wstring& path)
    {
        Media::Imaging::BitmapImage bmp{ Windows::Foundation::Uri{ path } };
        ImageIconSource src;
        src.ImageSource(bmp);
        return src;
    }

    std::wstring AskSaveLocation(HWND parent, const wchar_t* suggestedName)
    {
        OPENFILENAMEW ofn{};
        wchar_t fileName[MAX_PATH] = L"";
        if (suggestedName)
            wcsncpy_s(fileName, suggestedName, MAX_PATH - 1);

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = parent;
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = L"All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrTitle = L"Save Download As";
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameW(&ofn))
            return fileName;
        return L"";
    }

    // Mouse back/forward buttons and browser keys arrive at the top-level window
    LRESULT CALLBACK MainSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        if (gSelf) {
            WKPageRef page = gSelf->CurrentPage();
            if (page) {
                if (msg == WM_XBUTTONDOWN) {
                    WORD button = GET_XBUTTON_WPARAM(wParam);
                    if (button == XBUTTON1)
                        WKPageGoBack(page);
                    else if (button == XBUTTON2)
                        WKPageGoForward(page);
                    return 0;
                }
                if (msg == WM_APPCOMMAND) {
                    switch (GET_APPCOMMAND_LPARAM(lParam)) {
                    case APPCOMMAND_BROWSER_BACKWARD:
                        WKPageGoBack(page);
                        return 0;
                    case APPCOMMAND_BROWSER_FORWARD:
                        WKPageGoForward(page);
                        return 0;
                    }
                }
            }
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
}

WKContextRef GetCurrentContext()
{
    return gSharedContext;
}

void SetStartupUrl(const char* url)
{
    if (url && *url)
        gStartupUrl = url;
}

namespace winrt::Cryptidium::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        gSelf = this;

        Title(L"Cryptidium");
        AppWindow().Resize({ 1024, 768 });
        AppWindow().SetIcon(AssetPath(L"app.ico"));

        auto native = this->try_as<::IWindowNative>();
        check_hresult(native->get_WindowHandle(&m_hwnd));
        SetWindowSubclass(m_hwnd, MainSubclassProc, 1, 0);

        Closed([this](auto&&, auto&&) { OnClosed(); });

        // Web views need a valid layout rect, so create the first tab once XAML has laid out
        WebHost().Loaded([this](auto&&, auto&&) {
            if (m_tabs.empty())
                AddTab(gStartupUrl.c_str());
        });
    }

    void MainWindow::OnClosed()
    {
        RemoveWindowSubclass(m_hwnd, MainSubclassProc, 1);
        CloseSettings();
        for (auto& t : m_tabs) {
            DestroyWindow(WKViewGetWindow(t.view));
            WKRelease(t.view);
        }
        m_tabs.clear();
        m_current = -1;
        if (gSharedContext) {
            WKRelease(gSharedContext);
            gSharedContext = nullptr;
        }
        gSelf = nullptr;
    }

    RECT MainWindow::WebRect()
    {
        auto host = WebHost();
        auto root = host.XamlRoot();
        if (!root)
            return {};
        double scale = root.RasterizationScale();
        auto pt = host.TransformToVisual(Content()).TransformPoint({ 0, 0 });
        int x = static_cast<int>(std::lround(pt.X * scale));
        int y = static_cast<int>(std::lround(pt.Y * scale));
        int w = static_cast<int>(std::lround(host.ActualWidth() * scale));
        int h = static_cast<int>(std::lround(host.ActualHeight() * scale));
        return { x, y, x + w, y + h };
    }

    void MainWindow::UpdateWebLayout()
    {
        RECT r = WebRect();
        if (EqualRect(&r, &m_lastRect))
            return;
        m_lastRect = r;
        for (auto& t : m_tabs)
            MoveWindow(WKViewGetWindow(t.view), r.left, r.top, r.right - r.left, r.bottom - r.top, TRUE);
    }

    void MainWindow::OnWebHostLayoutUpdated(Windows::Foundation::IInspectable const&, Windows::Foundation::IInspectable const&)
    {
        UpdateWebLayout();
    }

    void MainWindow::ShowCurrentTab()
    {
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            HWND child = WKViewGetWindow(m_tabs[i].view);
            if (static_cast<int>(i) == m_current) {
                ShowWindow(child, SW_SHOW);
                SetWindowPos(child, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            } else {
                ShowWindow(child, SW_HIDE);
            }
        }
    }

    WKPageRef MainWindow::CurrentPage()
    {
        if (m_current < 0 || m_current >= static_cast<int>(m_tabs.size()))
            return nullptr;
        return WKViewGetPage(m_tabs[m_current].view);
    }

    int MainWindow::FindTabByItem(Windows::Foundation::IInspectable const& item)
    {
        if (!item)
            return -1;
        for (size_t i = 0; i < m_tabs.size(); ++i)
            if (m_tabs[i].item == item)
                return static_cast<int>(i);
        return -1;
    }

    int MainWindow::FindTabByPage(WKPageRef page)
    {
        for (size_t i = 0; i < m_tabs.size(); ++i)
            if (WKViewGetPage(m_tabs[i].view) == page)
                return static_cast<int>(i);
        return -1;
    }

    void MainWindow::NavigateCurrent(const char* url)
    {
        WKPageRef page = CurrentPage();
        if (!page)
            return;
        WKURLRef wkurl = WKURLCreateWithUTF8CString(url);
        WKPageLoadURL(page, wkurl);
        WKRelease(wkurl);
    }

    void MainWindow::UpdateUrlBar(WKPageRef page)
    {
        WKURLRef url = WKPageCopyActiveURL(page);
        if (!url)
            return;
        WKStringRef str = WKURLCopyString(url);
        std::string text = WKStringToUTF8(str);
        WKRelease(str);
        WKRelease(url);
        if (text.rfind("https://", 0) == 0)
            text.erase(0, 8);
        UrlBox().Text(UTF8ToWide(text));
    }

    void MainWindow::PageChanged(WKPageRef page)
    {
        int idx = FindTabByPage(page);
        if (idx < 0)
            return;

        WKStringRef titleRef = WKPageCopyTitle(page);
        std::string title = WKStringToUTF8(titleRef);
        WKRelease(titleRef);

        std::string host;
        WKURLRef urlRef = WKPageCopyActiveURL(page);
        if (urlRef) {
            WKStringRef str = WKURLCopyString(urlRef);
            host = GetHostFromURL(WKStringToUTF8(str));
            WKRelease(str);
            WKRelease(urlRef);
        }

        m_tabs[idx].item.Header(box_value(UTF8ToWide(title.empty() ? host : title)));
        if (idx == m_current)
            UpdateUrlBar(page);
        if (!host.empty() && host != m_tabs[idx].iconHost) {
            m_tabs[idx].iconHost = host;
            UpdateFavicon(m_tabs[idx].view, host);
        }
    }

    void MainWindow::UpdateFavicon(WKViewRef view, const std::string& host)
    {
        auto queue = DispatcherQueue();
        std::thread([queue, view, host] {
            std::wstring iconUrl = L"https://" + UTF8ToWide(host) + L"/favicon.ico";
            wchar_t cachePath[MAX_PATH];
            if (FAILED(URLDownloadToCacheFileW(nullptr, iconUrl.c_str(), cachePath, MAX_PATH, 0, nullptr)))
                return;
            std::wstring path = cachePath;
            queue.TryEnqueue([view, path] {
                if (!gSelf)
                    return;
                for (auto& t : gSelf->m_tabs) {
                    if (t.view == view) {
                        try {
                            t.item.IconSource(IconFromFile(path));
                        } catch (...) {
                        }
                        break;
                    }
                }
            });
        }).detach();
    }

    void MainWindow::InitializeSharedContext()
    {
        if (gSharedContext)
            return;

        gSharedContext = WKContextCreateWithConfiguration(nullptr);
        WKContextSetMaximumNumberOfProcesses(gSharedContext, 1);

        static WKContextDownloadClientV0 downloadClient;
        downloadClient.base.version = 0;
        downloadClient.base.clientInfo = m_hwnd;

        downloadClient.didStart = [](WKContextRef, WKDownloadRef, const void*) {
            if (gSelf) {
                if (WKPageRef page = gSelf->CurrentPage())
                    WKPageGoBack(page);
            }
        };

        downloadClient.decideDestinationWithSuggestedFilename = [](WKContextRef, WKDownloadRef, WKStringRef filename, bool* allowOverwrite, const void* clientInfo) -> WKStringRef {
            HWND parent = (HWND)clientInfo;

            std::wstring suggested = UTF8ToWide(WKStringToUTF8(filename));
            std::wstring destination;

            if (GetAskDownloadLocation()) {
                destination = AskSaveLocation(parent, suggested.c_str());
                if (destination.empty())
                    return nullptr;
            } else {
                std::wstring folder = GetDownloadPath();
                if (!CreateDirectoryW(folder.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
                    return nullptr;
                destination = folder + L"\\" + suggested;
            }

            *allowOverwrite = true;
            return WKStringCreateWithUTF8CString(WideToUTF8(destination).c_str());
        };

        WKContextSetDownloadClient(gSharedContext, &downloadClient.base);
    }

    void MainWindow::AddTab(const char* url)
    {
        InitializeSharedContext();

        WKPageConfigurationRef cfg = WKPageConfigurationCreate();
        WKPageConfigurationSetContext(cfg, gSharedContext);

        WKViewRef view = WKViewCreate(WebRect(), cfg, m_hwnd);
        WKPageRef page = WKViewGetPage(view);
        WKStringRef ua = WKStringCreateWithUTF8CString(gUserAgent.c_str());
        WKPageSetCustomUserAgent(page, ua);
        WKRelease(ua);

        static WKPageNavigationClientV0 navClient;
        navClient.base.version = 0;
        navClient.didCommitNavigation = [](WKPageRef p, WKNavigationRef, WKTypeRef, const void*) {
            if (gSelf) gSelf->PageChanged(p);
        };
        navClient.didFinishNavigation = [](WKPageRef p, WKNavigationRef, WKTypeRef, const void*) {
            if (gSelf) gSelf->PageChanged(p);
        };
        navClient.didSameDocumentNavigation = [](WKPageRef p, WKNavigationRef, WKSameDocumentNavigationType, WKTypeRef, const void*) {
            if (gSelf) gSelf->PageChanged(p);
        };
        WKPageSetPageNavigationClient(page, &navClient.base);

        static WKPageUIClientV0 uiClient;
        uiClient.base.version = 0;
        uiClient.base.clientInfo = m_hwnd;
        WKPageSetPageUIClient(page, &uiClient.base);

        HWND child = WKViewGetWindow(view);
        ShowWindow(child, SW_HIDE);
        WKViewSetIsInWindow(view, true);

        TabViewItem item;
        item.Header(box_value(L"Loading"));
        item.IconSource(IconFromFile(AssetPath(L"app.ico")));

        m_tabs.push_back({ view, item, {} });
        int index = static_cast<int>(m_tabs.size()) - 1;
        m_current = index;
        Tabs().TabItems().Append(item);
        Tabs().SelectedItem(item);
        ShowCurrentTab();
        UpdateWebLayout();
        if (url)
            NavigateCurrent(url);
    }

    void MainWindow::CloseTab(TabViewItem const& item)
    {
        int index = FindTabByItem(item);
        if (index < 0)
            return;

        WKViewRef view = m_tabs[index].view;
        m_tabs.erase(m_tabs.begin() + index);
        if (m_current >= index)
            --m_current;

        // Removing the item makes the TabView pick a new selection, which re-enters OnTabSelectionChanged
        uint32_t pos = 0;
        if (Tabs().TabItems().IndexOf(item, pos))
            Tabs().TabItems().RemoveAt(pos);

        DestroyWindow(WKViewGetWindow(view));
        WKRelease(view);

        if (m_tabs.empty()) {
            Close();
            return;
        }
        if (m_current < 0)
            m_current = 0;
        ShowCurrentTab();
        if (WKPageRef page = CurrentPage())
            UpdateUrlBar(page);
    }

    void MainWindow::OnAddTab(TabView const&, Windows::Foundation::IInspectable const&)
    {
        AddTab("https://google.com");
    }

    void MainWindow::OnTabCloseRequested(TabView const&, TabViewTabCloseRequestedEventArgs const& args)
    {
        CloseTab(args.Tab());
    }

    void MainWindow::OnTabSelectionChanged(Windows::Foundation::IInspectable const&, SelectionChangedEventArgs const&)
    {
        int idx = FindTabByItem(Tabs().SelectedItem());
        if (idx < 0)
            return;
        m_current = idx;
        ShowCurrentTab();
        if (WKPageRef page = CurrentPage())
            UpdateUrlBar(page);
        else
            UrlBox().Text(L"");
    }

    void MainWindow::OnBack(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        if (WKPageRef page = CurrentPage())
            WKPageGoBack(page);
    }

    void MainWindow::OnForward(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        if (WKPageRef page = CurrentPage())
            WKPageGoForward(page);
    }

    void MainWindow::OnRefresh(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        if (WKPageRef page = CurrentPage())
            WKPageReload(page);
    }

    void MainWindow::OnSettings(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        ShowSettings();
    }

    void MainWindow::OnUrlGotFocus(Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        UrlBox().SelectAll();
    }

    void MainWindow::OnUrlKeyDown(Windows::Foundation::IInspectable const&, Input::KeyRoutedEventArgs const& e)
    {
        if (e.Key() != Windows::System::VirtualKey::Enter)
            return;
        e.Handled(true);

        std::string input = WideToUTF8(std::wstring(UrlBox().Text()));
        if (input.empty())
            return;

        std::string display = input;
        std::string url;
        if (input.find(' ') != std::string::npos) {
            for (char& c : input)
                if (c == ' ')
                    c = '+';
            url = "https://www.google.com/search?q=" + input;
        } else {
            if (input.rfind("https://", 0) == 0)
                input.erase(0, 8);
            url = "https://" + input;
            display = input;
        }
        UrlBox().Text(UTF8ToWide(display));
        NavigateCurrent(url.c_str());
    }
}
