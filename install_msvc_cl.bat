@echo off
setlocal

echo ============================================
echo   Installing MSVC C++ Build Tools (cl.exe)
echo ============================================
echo.

:: ---------------------------------------------------------
:: 1. Locate Visual Studio Installer
:: ---------------------------------------------------------
echo Searching for Visual Studio Installer...
set VSINSTALLER=

for /r "C:\Program Files (x86)\Microsoft Visual Studio\Installer" %%F in (vs_installer.exe) do (
    set VSINSTALLER=%%F
)

if "%VSINSTALLER%"=="" (
    echo Installer not found in Program Files (x86). Searching Program Files...
    for /r "C:\Program Files\Microsoft Visual Studio" %%F in (vs_installer.exe) do (
        set VSINSTALLER=%%F
    )
)

if "%VSINSTALLER%"=="" (
    echo [ERROR] Could not find vs_installer.exe on this system.
    echo Install Visual Studio Build Tools manually from:
    echo https://visualstudio.microsoft.com/downloads/
    pause
    exit /b 1
)

echo Found installer:
echo %VSINSTALLER%
echo.

:: ---------------------------------------------------------
:: 2. Install the MSVC C++ workload
:: ---------------------------------------------------------
echo Installing Microsoft.VisualStudio.Workload.VCTools...
"%VSINSTALLER%" modify ^
  --installPath "C:\Program Files\Microsoft Visual Studio\2022\BuildTools" ^
  --add Microsoft.VisualStudio.Workload.VCTools ^
  --includeRecommended ^
  --quiet --wait --norestart

echo.
echo Workload installation complete.
echo.

:: ---------------------------------------------------------
:: 3. Locate cl.exe
:: ---------------------------------------------------------
echo Searching for cl.exe...
set CLPATH=

for /r "C:\Program Files\Microsoft Visual Studio" %%F in (cl.exe) do (
    set CLPATH=%%F
)

if "%CLPATH%"=="" (
    echo [ERROR] cl.exe not found after installation.
    echo You may need to reboot and run this script again.
    pause
    exit /b 1
)

echo Found cl.exe:
echo %CLPATH%
echo.

:: ---------------------------------------------------------
:: 4. Add cl.exe directory to PATH
:: ---------------------------------------------------------
for %%D in ("%CLPATH%") do set CLDIR=%%~dpD

echo Adding to PATH:
echo %CLDIR%

setx PATH "%PATH%;%CLDIR%"

echo.
echo PATH updated. You must restart your terminal for changes to take effect.
echo.

:: ---------------------------------------------------------
:: 5. Final verification
:: ---------------------------------------------------------
echo Verifying MSVC compiler...
echo (Restart your terminal if this fails)

cl
echo.

echo ============================================
echo   MSVC C++ Build Tools Installed Successfully
echo ============================================
echo.

pause
endlocal
