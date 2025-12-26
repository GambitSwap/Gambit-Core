@echo off
setlocal

echo ============================================
echo   Gambit Windows Build Environment Setup
echo ============================================
echo.

:: ---------------------------------------------------------
:: 1. Check for winget
:: ---------------------------------------------------------
where winget >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] winget not found. Install Windows App Installer from Microsoft Store.
    pause
    exit /b 1
)

:: ---------------------------------------------------------
:: 2. Install CMake if missing
:: ---------------------------------------------------------
echo Checking for CMake...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo CMake not found. Installing...
    winget install Kitware.CMake --silent --accept-package-agreements --accept-source-agreements
) else (
    echo CMake already installed.
)

:: ---------------------------------------------------------
:: 3. Install Git if missing
:: ---------------------------------------------------------
echo Checking for Git...
where git >nul 2>&1
if %errorlevel% neq 0 (
    echo Git not found. Installing...
    winget install Git.Git --silent --accept-package-agreements --accept-source-agreements
) else (
    echo Git already installed.
)

:: ---------------------------------------------------------
:: 4. Install Visual Studio Build Tools (MSVC)
:: ---------------------------------------------------------
echo Checking for MSVC compiler...
where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo MSVC not found. Installing Build Tools...
    winget install Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements
) else (
    echo MSVC compiler already installed.
)

:: ---------------------------------------------------------
:: 5. Add CMake to PATH if needed
:: ---------------------------------------------------------
echo Checking PATH for CMake...
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo Adding CMake to PATH...
    setx PATH "%PATH%;C:\Program Files\CMake\bin"
)

:: ---------------------------------------------------------
:: 6. Verify installation
:: ---------------------------------------------------------
echo.
echo Verifying tools...
cmake --version
if %errorlevel% neq 0 (
    echo [ERROR] CMake still not found. Restart your terminal and try again.
    pause
    exit /b 1
)

cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] MSVC compiler not found. You may need to reboot.
    pause
    exit /b 1
)

echo All tools installed successfully.
echo.

:: ---------------------------------------------------------
:: 7. Build Gambit
:: ---------------------------------------------------------
echo Starting Gambit build...
cd /d "%~dp0"

if not exist build (
    mkdir build
)

cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release

echo.
echo ============================================
echo   Gambit Build Complete
echo ============================================
echo.

pause
endlocal