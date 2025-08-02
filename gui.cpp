#include "gui.h"
#include "buildinfo.h"

#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Windows.UI.h>
#include <MddBootstrap.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

struct MainWindow : winrt::Microsoft::UI::Xaml::WindowT<MainWindow>
{
    MainWindow()
    {
        Title(L"Cryptidium");
        ExtendsContentIntoTitleBar(true);

        Grid root;
        root.RowDefinitions().Append(RowDefinition());
        root.RowDefinitions().Append(RowDefinition());

        TabView tabs;
        auto item = TabViewItem();
        item.Header(box_value(L"Tab 1"));
        TextBlock block;
        block.Text(L"Blank");
        item.Content(block);
        tabs.TabItems().Append(item);
        root.Children().Append(tabs);
        Grid::SetRow(tabs, 0);
        SetTitleBar(tabs);
        Content(root);
    }
};

int RunBrowser(HINSTANCE, int)
{
    winrt::init_apartment(winrt::apartment_type::single_threaded);
    MddBootstrapInitialize(0, nullptr);
    Application::Start([](auto &&) { make<MainWindow>().Activate(); });
    MddBootstrapShutdown();
    return 0;
}
