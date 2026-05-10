@echo off
echo ESP32 Virtual Hardware Simulator
echo ================================
echo.

REM Check for Qt6
where qmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Qt6 not found. Please install Qt6 and add to PATH.
    echo Download from: https://www.qt.io/download-qt-installer
    pause
    exit /b 1
)

echo Qt6 found
echo.

REM Create build directory
if not exist build mkdir build
cd build

echo Configuring with CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building simulator...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo Build successful!
echo.
echo Starting ESP32 Virtual Hardware Simulator...
echo.

if exist Release\esp32_simulator.exe (
    echo Running simulator...
    start Release\esp32_simulator.exe
) else (
    echo ERROR: Simulator executable not found
    pause
    exit /b 1
)

pause
