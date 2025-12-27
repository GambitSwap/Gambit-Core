@echo off
echo Building Gambit with tests...
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DGAMBIT_BUILD_TESTS=ON
cmake --build . --config Release
echo.
echo Build complete!

