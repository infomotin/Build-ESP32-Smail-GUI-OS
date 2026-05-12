@echo off
REM Build ESP32 Virtual Hardware Simulator for Windows
REM Uses Qt6 MinGW64 toolchain from C:\Qt\6.11.0\mingw_64

setlocal

echo === ESP32 Virtual Hardware Simulator Build ===
echo.

REM Configuration
set SCRIPT_DIR=%~dp0
set DESKTOPAPP_DIR=%SCRIPT_DIR%DesktopApp
set BUILD_DIR=%DESKTOPAPP_DIR%\cmake-build-debug
set QT_DIR=C:\Qt\6.11.0\mingw_64

REM Check Qt installation
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo Error: Qt6 not found at %QT_DIR%
    echo Please install Qt6 with MinGW64 toolchain
    echo Expected: %QT_DIR%\bin\qmake.exe
    pause
    exit /b 1
)

REM Add Qt tools to PATH
set PATH=%QT_DIR%\bin;%QT_DIR%\lib;%PATH%

REM Also add CMake from QtTools
set CMAKE_PATH=C:\Qt\Tools\CMake_64\bin
if exist "%CMAKE_PATH%\cmake.exe" (
    set PATH=%CMAKE_PATH%;%PATH%
) else (
    echo Warning: CMake not found at %CMAKE_PATH%
    echo Make sure CMake is installed and in PATH
)

echo Qt directory: %QT_DIR%
echo Build directory: %BUILD_DIR%
echo.

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

echo Configuring CMake...
cmake -G "MinGW Makefiles" ^
      -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
      -DCMAKE_BUILD_TYPE=Debug ^
      "%DESKTOPAPP_DIR%"

if errorlevel 1 (
    echo.
    echo CMake configuration FAILED!
    echo.
    echo Troubleshooting:
    echo 1. Ensure Qt6 is installed at %QT_DIR%
    echo 2. Ensure CMake is in PATH
    echo 3. Check that you have MinGW-w64 compiler
    echo.
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Debug -- -j4

if errorlevel 1 (
    echo.
    echo Build FAILED!
    echo Check the error messages above for details.
    echo.
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build SUCCESSFUL!
echo ============================================
echo.
echo Executable location:
echo   %BUILD_DIR%\esp32_virtual_simulator.exe
echo.
echo To run the simulator:
echo   1. cd "%BUILD_DIR%"
echo   2. esp32_virtual_simulator.exe
echo.
echo OR double-click the executable in Explorer.
echo.

REM Offer to run now
set /p RUN_NOW="Run the simulator now? (Y/N): "
if /i "%RUN_NOW%"=="Y" (
    echo Starting simulator...
    start "" "%BUILD_DIR%\esp32_virtual_simulator.exe"
)

pause

endlocal
