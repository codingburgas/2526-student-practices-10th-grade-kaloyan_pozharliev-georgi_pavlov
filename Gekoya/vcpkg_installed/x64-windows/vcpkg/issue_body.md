Package: miniaudio:x64-windows@0.11.24

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35221.0
- CMake Version: 3.31.10
-    vcpkg-tool version: 2025-12-16-44bb3ce006467fc13ba37ca099f64077b8bbf84d
    vcpkg-scripts version: b2f068faf4 2026-02-25 (4 weeks ago)

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/mackron/miniaudio/archive/0.11.24.tar.gz -> mackron-miniaudio-0.11.24.tar.gz
warning: Download https://github.com/mackron/miniaudio/archive/0.11.24.tar.gz failed -- retrying after 1000ms
warning: Download https://github.com/mackron/miniaudio/archive/0.11.24.tar.gz failed -- retrying after 2000ms
warning: Download https://github.com/mackron/miniaudio/archive/0.11.24.tar.gz failed -- retrying after 4000ms
error: https://github.com/mackron/miniaudio/archive/0.11.24.tar.gz: WinHttpSendRequest failed with exit code 12007. The server name or address could not be resolved
note: If you are using a proxy, please ensure your proxy settings are correct.
Possible causes are:
1. You are actually using an HTTP proxy, but setting HTTPS_PROXY variable to `https://address:port`.
This is not correct, because `https://` prefix claims the proxy is an HTTPS proxy, while your proxy (v2ray, shadowsocksr, etc...) is an HTTP proxy.
Try setting `http://address:port` to both HTTP_PROXY and HTTPS_PROXY instead.
2. If you are using Windows, vcpkg will automatically use your Windows IE Proxy Settings set by your proxy software. See: https://github.com/microsoft/vcpkg-tool/pull/77
The value set by your proxy might be wrong, or have same `https://` prefix issue.
3. Your proxy's remote server is out of service.
If you believe this is not a temporary download server failure and vcpkg needs to be changed to download this file from a different location, please submit an issue to https://github.com/Microsoft/vcpkg/issues
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:136 (message):
  Download failed, halting portfile.
Call Stack (most recent call first):
  scripts/cmake/vcpkg_from_github.cmake:120 (vcpkg_download_distfile)
  ports/miniaudio/portfile.cmake:2 (vcpkg_from_github)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "gekoya",
  "version": "0.1.0",
  "dependencies": [
    "raylib"
  ]
}

```
</details>
