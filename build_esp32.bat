@echo off
REM ESP32 Small OS Build and Flash Script for Windows
REM Builds and flashes the ESP32 Small OS to an ESP32 board

setlocal enabledelayedexpansion

REM Default values
set PORT=COM3
set BAUD=921600
set TARGET=esp32
set CLEAN_BUILD=false
set FLASH_ONLY=false
set MONITOR_ONLY=false

REM Help function
:show_help
echo ESP32 Small OS Build and Flash Script for Windows
echo Usage: %~nx0 [OPTIONS]
echo.
echo Options:
echo   -p, --port PORT      Serial port (default: COM3)
echo   -b, --baud BAUD      Baud rate (default: 921600)
echo   -t, --target TARGET  ESP32 target (esp32, esp32s2, esp32s3, esp32c3)
echo   -c, --clean          Clean build before compiling
echo   -f, --flash-only     Flash only (skip build)
echo   -m, --monitor-only   Monitor only (skip build and flash)
echo   -h, --help           Show this help message
echo.
echo Examples:
echo   %~nx0                 Build and flash with default settings
echo   %~nx0 -p COM4         Use different serial port
echo   %~nx0 -c              Clean build and flash
echo   %~nx0 -f              Flash only (skip build)
echo   %~nx0 -m              Monitor only
goto :eof

REM Parse command line arguments
:parse_args
if "%1"=="" goto :main_loop
if "%1"=="-p" set PORT=%2 & shift & shift & goto :parse_args
if "%1"=="--port" set PORT=%2 & shift & shift & goto :parse_args
if "%1"=="-b" set BAUD=%2 & shift & shift & goto :parse_args
if "%1"=="--baud" set BAUD=%2 & shift & shift & goto :parse_args
if "%1"=="-t" set TARGET=%2 & shift & shift & goto :parse_args
if "%1"=="--target" set TARGET=%2 & shift & shift & goto :parse_args
if "%1"=="-c" set CLEAN_BUILD=true & shift & goto :parse_args
if "%1"=="--clean" set CLEAN_BUILD=true & shift & goto :parse_args
if "%1"=="-f" set FLASH_ONLY=true & shift & goto :parse_args
if "%1"=="--flash-only" set FLASH_ONLY=true & shift & goto :parse_args
if "%1"=="-m" set MONITOR_ONLY=true & shift & goto :parse_args
if "%1"=="--monitor-only" set MONITOR_ONLY=true & shift & goto :parse_args
if "%1"=="-h" goto :show_help
if "%1"=="--help" goto :show_help
echo Unknown option: %1
goto :show_help

:main_loop
echo ESP32 Small OS Build and Flash Script for Windows
echo ================================================

REM Check if ESP-IDF environment is set up
if "%IDF_PATH%"=="" (
    echo Error: ESP-IDF environment not set up
    echo Please run:
    echo   %IDF_PATH%\export.bat
    echo or your ESP-IDF export script
    pause
    exit /b 1
)

if not exist "%IDF_PATH%" (
    echo Error: IDF_PATH not found: %IDF_PATH%
    pause
    exit /b 1
)

echo ESP-IDF found at: %IDF_PATH%

REM Clean build directory
if "%CLEAN_BUILD%"=="true" (
    echo Cleaning build directory...
    if exist build rmdir /s /q build
)

REM Build the project
if not "%FLASH_ONLY%"=="true" if not "%MONITOR_ONLY%"=="true" (
    echo Building ESP32 Small OS...
    
    REM Set target
    idf.py set-target %TARGET%
    
    REM Build
    idf.py build
    
    if !errorlevel! neq 0 (
        echo Build failed!
        pause
        exit /b 1
    )
    
    echo Build successful!
)

REM Flash the firmware
if not "%MONITOR_ONLY%"=="true" (
    echo Flashing firmware to ESP32...
    echo Port: %PORT%, Baud: %BAUD%
    
    REM Flash
    idf.py -p %PORT% -b %BAUD% flash
    
    if !errorlevel! neq 0 (
        echo Flash failed!
        pause
        exit /b 1
    )
    
    echo Flash successful!
)

REM Start monitor
echo Starting serial monitor...
echo Press Ctrl+C to exit monitor

idf.py -p %PORT% monitor

pause
