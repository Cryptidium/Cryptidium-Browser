using System;
using System.IO;
using System.Windows.Forms;
using WebKit;

namespace Cryptidium.Browser;

static class Program
{
    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);

        var asmPath = Path.Combine(AppContext.BaseDirectory, "WebKitBrowser.dll");
        if (!File.Exists(asmPath))
        {
            MessageBox.Show(
                "WebKitBrowser.dll not found. Ensure the WebKitBrowser_ZC package is restored " +
                "and its files are copied next to the executable.",
                "Missing WebKit dependency",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error);
            return;
        }

        Application.Run(new BrowserForm());
    }
}

class BrowserForm : Form
{
    private readonly TextBox _address;
    private readonly WebKitBrowser _view;

    public BrowserForm()
    {
        Text = "Cryptidium";
        Width = 800;
        Height = 600;

        _address = new TextBox { Text = "https://google.com", Width = 600 };
        _address.KeyDown += (s, e) => { if (e.KeyCode == Keys.Enter) Navigate(); };
        var button = new Button { Text = "Go" };
        button.Click += (s, e) => Navigate();
        var bar = new FlowLayoutPanel { Dock = DockStyle.Top, AutoSize = true };
        bar.Controls.Add(_address);
        bar.Controls.Add(button);

        _view = new WebKitBrowser { Dock = DockStyle.Fill };

        Controls.Add(_view);
        Controls.Add(bar);

        Load += (_, __) => _view.Navigate(_address.Text);
    }

    private void Navigate()
    {
        var url = _address.Text.Trim();
        if (!url.StartsWith("http", StringComparison.OrdinalIgnoreCase))
        {
            url = "https://" + url;
            _address.Text = url;
        }
        _view.Navigate(url);
    }
}
