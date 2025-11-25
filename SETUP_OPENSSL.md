# Setup OpenSSL for Windows

To proceed with Phase 9 (HTTPS/TLS), we need the OpenSSL headers and libraries.

## Option 1: Chocolatey (Recommended)
If you have Chocolatey installed, run this in an Administrator PowerShell:
```powershell
choco install openssl
```
This usually installs to `C:\Program Files\OpenSSL-Win64`.

## Option 2: vcpkg
If you use vcpkg:
```powershell
vcpkg install openssl:x64-windows
```

## Option 3: Manual Installer
1. Download "Win64 OpenSSL v3.x" from [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html).
2. Install to default location (`C:\Program Files\OpenSSL-Win64`).
3. Ensure "The OpenSSL binaries (/bin) directory" is added to the system PATH (optional but good for testing).

## Verification
After installation, please confirm the path. We expect:
- Headers: `C:\Program Files\OpenSSL-Win64\include`
- Libs: `C:\Program Files\OpenSSL-Win64\lib`

If installed elsewhere, please update `build.bat` accordingly.
