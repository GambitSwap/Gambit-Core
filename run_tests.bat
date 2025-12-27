@echo off
echo Running Gambit unit tests...
echo.

if exist "build\Release\gambit_tests.exe" (
    build\Release\gambit_tests.exe --gtest_color=yes
) else if exist "build\tests\Release\gambit_tests.exe" (
    build\tests\Release\gambit_tests.exe --gtest_color=yes
) else (
    echo ERROR: gambit_tests.exe not found!
    echo Please run build_tests.bat first.
    exit /b 1
)

echo.
echo Tests complete!

