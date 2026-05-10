@echo off
echo ESP32 Virtual Hardware Simulator Build Script
echo ==============================================
echo.

REM Check if Qt6 is available
where qmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Qt6 qmake not found in PATH
    echo Please install Qt6 and add it to your PATH
    echo Download from: https://www.qt.io/download-qt-installer
    pause
    exit /b 1
)

echo Qt6 found: 
qmake -version
echo.

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake and add it to your PATH
    echo Download from: https://cmake.org/download/
    pause
    exit /b 1
)

echo CMake found:
cmake --version | head -n 1
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo Configuring project...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building simulator...
cmake --build . --config Release -j4
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Running ESP32 Virtual Hardware Simulator...
echo.

REM Run the simulator
Release\esp32_simulator.exe

pause
