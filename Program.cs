using System;
using System.Windows.Forms;
using Microsoft.Web.WebView2.WinForms;

namespace Cryptidium.Browser;

static class Program
{
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new BrowserForm());
    }
}

class BrowserForm : Form
{
    private readonly TextBox _address;
    private readonly WebView2 _view;

    public BrowserForm()
    {
        Text = "Cryptidium Browser";
        Width = 800;
        Height = 600;

        _address = new TextBox { Text = "https://example.com", Width = 600 };
        _address.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) Navigate(); };
        var button = new Button { Text = "Go" };
        button.Click += (s, e) => Navigate();
        var bar = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        bar.Controls.Add(_address);
        bar.Controls.Add(button);

        _view = new WebView2 { Dock = DockStyle.Fill };

        Controls.Add(_view);
        Controls.Add(bar);

        Load += async (_, __) =>
        {
            await _view.EnsureCoreWebView2Async();
            _view.Source = new Uri(_address.Text);
        };
    }

    private void Navigate()
    {
        var url = _address.Text.Trim();
        if (!url.StartsWith("http", StringComparison.OrdinalIgnoreCase))
        {
            url = "https://" + url;
            _address.Text = url;
        }
        _view.Source = new Uri(url);
    }
}
