@echo off
echo Cleaning build folder...
if exist build (
    rmdir /s /q build
    echo Build folder cleared.
) else (
    echo Build folder does not exist.
)
mkdir build
echo Empty build folder created.

