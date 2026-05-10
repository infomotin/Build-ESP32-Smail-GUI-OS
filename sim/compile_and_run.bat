@echo off
echo Compiling and Running ESP32 Small OS Simulation Test
echo ====================================================
echo.

REM Compile the simulation
echo Compiling simulation...
gcc -DSIMULATION -Wall -Wextra -std=c99 -O2 -o simple_test.exe simple_test.c

if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed!
    pause
    exit /b 1
)

echo Compilation successful!
echo.
echo Running ESP32 Small OS Simulation Test...
echo Duration: 10 seconds
echo.
simple_test.exe

echo.
echo Simulation test completed!
pause
