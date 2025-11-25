@echo off
setlocal

set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VS_PATH%" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)
if not exist "%VS_PATH%" (
    echo Error: Could not find vcvars64.bat. Please install Visual Studio 2022 C++ Build Tools.
    exit /b 1
)

echo Calling vcvars64.bat...
call "%VS_PATH%" > nul

echo Building DK Server (Release Mode)...
cl /EHsc /std:c++latest /O2 /I src /I vendor\vcpkg\installed\x64-windows\include src\main.cpp src\sys\WindowsIOCP.cpp src\core\BufferPool.cpp src\http\Parser.cpp /link /out:server.exe /LIBPATH:vendor\vcpkg\installed\x64-windows\lib ws2_32.lib mswsock.lib libssl.lib libcrypto.lib crypt32.lib user32.lib advapi32.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build Success!
    echo Run with: .\server.exe
) else (
    echo.
    echo Build Failed!
)

endlocal
