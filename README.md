# Cryptidium
WebKit Browser for Windows 11

## Simple Browser

This repository now includes a very small WebKit browser example built with WinForms.

### Run

From a Windows 11 system with the .NET 8 SDK installed, run:

```powershell
dotnet run --project SimpleBrowser.csproj
```

Enter a URL in the address bar and press **Go** to navigate.

If a `System.IO.FileNotFoundException` appears for `WebKitBrowser.dll`, ensure the
`WebKitBrowser_ZC` package has restored correctly and that the `WebKitBrowser.dll`
file is present next to the executable.
