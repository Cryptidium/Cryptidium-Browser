using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.Web.WebView2.Core;
using System;

namespace Cryptidium.WinUI
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();
            AddNewTab("https://google.com");
        }

        private async void AddNewTab(string url)
        {
            var tab = new TabViewItem
            {
                Header = $"Tab {BrowserTabs.TabItems.Count + 1}"
            };
            var webview = new WebView2();
            await webview.EnsureCoreWebView2Async();
            webview.Source = new Uri(url);
            tab.Content = webview;
            BrowserTabs.TabItems.Add(tab);
            BrowserTabs.SelectedItem = tab;
        }

        private void OnNewTabClicked(object sender, RoutedEventArgs e)
        {
            AddNewTab("https://google.com");
        }

        private void OnBackClicked(object sender, RoutedEventArgs e)
        {
            if (BrowserTabs.SelectedItem is TabViewItem tab && tab.Content is WebView2 web)
            {
                if (web.CanGoBack) web.GoBack();
            }
        }

        private void OnForwardClicked(object sender, RoutedEventArgs e)
        {
            if (BrowserTabs.SelectedItem is TabViewItem tab && tab.Content is WebView2 web)
            {
                if (web.CanGoForward) web.GoForward();
            }
        }

        private void OnRefreshClicked(object sender, RoutedEventArgs e)
        {
            if (BrowserTabs.SelectedItem is TabViewItem tab && tab.Content is WebView2 web)
            {
                web.Reload();
            }
        }

        private void OnAddressKeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.Enter)
            {
                if (BrowserTabs.SelectedItem is TabViewItem tab && tab.Content is WebView2 web)
                {
                    if (Uri.TryCreate(AddressBar.Text, UriKind.Absolute, out var uri))
                    {
                        web.Source = uri;
                    }
                }
            }
        }

        private void OnTabSelectionChanged(TabView sender, object args)
        {
            if (BrowserTabs.SelectedItem is TabViewItem tab && tab.Content is WebView2 web)
            {
                AddressBar.Text = web.Source?.ToString() ?? string.Empty;
            }
        }
    }
}
