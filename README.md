# Cryptidium

Cryptidium is a minimal WebKit-based browser written in C++. It opens a window titled **Cryptidium** and loads a web page using WebKit2GTK.

## Building on Linux
1. Install dependencies (Ubuntu):
   ```bash
   sudo apt-get install libwebkit2gtk-4.1-dev build-essential
   ```
2. Compile:
   ```bash
   g++ Cryptidium.cpp -std=c++17 $(pkg-config --cflags --libs webkit2gtk-4.1) -o cryptidium
   ```

## Building on Windows
1. Open `Cryptidium.sln` in Visual Studio 2022.
2. Ensure the **x64** configuration is selected.
3. Build and run the `Cryptidium` project. The window will display `https://example.com` by default.

## Usage
Run the compiled binary optionally passing a URL:
```bash
./cryptidium https://example.com
```
If no URL is supplied, `https://example.com` is loaded.

