# Cryptidium
A modern WebKit based web browser for Windows 11
# Building from Source
1. Download the Source Code
2. Unzip the files
3. Open the unzipped repository in PowerShell or Terminal
4. Run the following command in PowerShell or Terminal
```bash
git clone https://github.com/Cryptidium/WebKit.git
```
5. Restore NuGet packages (`nuget restore Cryptidium.sln` or let Visual Studio do it). The UI uses WinUI 3 (Windows App SDK 1.6, C++/WinRT)
6. Open the solution in Visual Studio 2022
7. Open the Build drop down on the top of the screen
8. Press Build Solution
