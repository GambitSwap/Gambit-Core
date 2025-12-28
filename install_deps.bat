@echo off
REM Install required dependencies for Gambit build

echo Checking for Strawberry Perl...
where perl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Perl is already installed
    perl --version
) else (
    echo Perl not found. Installing Strawberry Perl...
    echo.
    echo You have two options:
    echo 1. Automatic (requires Administrator): Run this script as Administrator
    echo 2. Manual: Download from https://strawberryperl.com/ and install
    echo.
    
    REM Try to install via chocolatey if available
    where choco >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo Chocolatey found, installing Perl...
        choco install strawberryperl -y
    ) else (
        echo Please install Perl manually or via Chocolatey, then run build.bat again
        echo.
        echo For Chocolatey: https://chocolatey.org/install
        echo For Strawberry Perl: https://strawberryperl.com/
        pause
        exit /b 1
    )
)

echo.
echo Dependencies check complete
