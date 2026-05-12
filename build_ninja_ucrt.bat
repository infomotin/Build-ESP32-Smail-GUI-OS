@echo off
REM Build ESP32 Virtual Simulator using Ninja + MSYS2 UCRT64

setlocal

echo === ESP32 Virtual Hardware Simulator Build (Ninja) ===
echo.

REM Paths
set SCRIPT_DIR=%~dp0
set DESKTOPAPP_DIR=%SCRIPT_DIR%DesktopApp
set BUILD_DIR=%DESKTOPAPP_DIR%\cmake-build-ninja
set CMAKE_EXE=C:\Qt\Tools\CMake_64\bin\cmake.exe
set QT_DIR=C:\Qt\6.11.0\mingw_64

REM Check Ninja
where ninja >nul 2>&1
if errorlevel 1 (
    echo Error: ninja not found in PATH
    pause
    exit /b 1
)

echo Using CMake: %CMAKE_EXE%
echo Using Qt: %QT_DIR%
echo Using Ninja: where ninja
echo Build dir: %BUILD_DIR%
echo.

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cd /d "%BUILD_DIR%"

echo Configuring with Ninja...
"%CMAKE_EXE%" -G Ninja ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_CXX_COMPILER="C:\msys64\ucrt64\bin\g++.exe" ^
    -DCMAKE_C_COMPILER="C:\msys64\ucrt64\bin\gcc.exe" ^
    "%DESKTOPAPP_DIR%"

if errorlevel 1 (
    echo.
    echo CMake configuration FAILED!
    pause
    exit /b 1
)

echo.
echo Building...
ninja

if errorlevel 1 (
    echo.
    echo Build FAILED!
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build SUCCESSFUL!
echo ============================================
echo.
echo Executable: %BUILD_DIR%\esp32_virtual_simulator.exe
echo.

REM Run?
set /p RUN="Run simulator? (Y/N): "
if /i "%RUN%"=="Y" (
    start "" "%BUILD_DIR%\esp32_virtual_simulator.exe"
)

pause

endlocal
