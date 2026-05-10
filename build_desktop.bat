@echo off
REM ESP32 Virtual Hardware Simulator Build Script for Windows
REM Builds the desktop Qt application separately

echo "ESP32 Virtual Hardware Simulator Build Script"
echo "==============================================="

REM Configuration
set SIM_DIR=%~dp0DesktopApp
set BUILD_DIR=%SIM_DIR%\cmake-build-debug
set QT_PATH=C:\Qt\6.9.0\msvc2022_64

REM Check for Qt
if not exist "%QT_PATH%\bin\qt6.exe" (
    echo Error: Qt6 not found at %QT_PATH%
    echo Please install Qt6 and update QT_PATH in this script
    echo Or use: vcpkg install qt6-base
    pause
    exit /b 1
)

REM Add Qt to PATH temporarily
set PATH=%QT_PATH%\bin;%PATH%

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 ^
      -DCMAKE_PREFIX_PATH="%QT_PATH%" ^
      -DCMAKE_BUILD_TYPE=Debug ^
      "%SIM_DIR%"

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

echo Building...
cmake --build . --config Debug --target ALL_BUILD

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo Build successful!
echo Executable is at: %BUILD_DIR%\Debug\esp32_virtual_simulator.exe
echo.

pause
