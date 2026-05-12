# ESP32 Virtual Simulator Build Script for Windows with Qt6
# Usage: .\build.ps1

$ErrorActionPreference = "Stop"

# Configuration
$QtDir = "C:\Qt\6.11.0\mingw_64"
$CmakeDir = "C:\Qt\Tools\CMake_64\bin"
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$DesktopAppDir = Join-Path $ProjectRoot "DesktopApp"
$BuildDir = Join-Path $DesktopAppDir "cmake-build-debug"

Write-Host "=== ESP32 Virtual Hardware Simulator Build ===" -ForegroundColor Cyan
Write-Host ""

# Check Qt
$qmake = Join-Path $QtDir "bin\qmake.exe"
if (-not (Test-Path $qmake)) {
    Write-Host "ERROR: Qt6 not found at $QtDir" -ForegroundColor Red
    Write-Host "Expected: $qmake"
    Write-Host "Please install Qt6 with MinGW64 toolchain"
    exit 1
}

Write-Host "Qt found at: $QtDir"
Write-Host "CMake: $CmakeDir\cmake.exe"
Write-Host ""

# Setup environment
$env:PATH = "$($QtDir)\bin;$($CmakeDir);" + $env:PATH

# Verify cmake
$cmakeVersion = & "$CmakeDir\cmake.exe" --version
Write-Host $cmakeVersion

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

Set-Location $BuildDir

Write-Host "Configuring CMake..."
& "$CmakeDir\cmake.exe" -G "MinGW Makefiles" `
    -DCMAKE_PREFIX_PATH="$QtDir" `
    -DCMAKE_BUILD_TYPE=Debug `
    "$DesktopAppDir"

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nCMake configuration FAILED!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`nBuilding..."
& "$CmakeDir\cmake.exe" --build . --config Debug -- -j4

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild FAILED!" -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`n============================================" -ForegroundColor Green
Write-Host "Build SUCCESSFUL!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "Executable: $BuildDir\esp32_virtual_simulator.exe"
Write-Host ""

# Run the application?
$run = Read-Host "Run the simulator now? (Y/N)"
if ($run -eq 'Y' -or $run -eq 'y') {
    Write-Host "Starting simulator..."
    Start-Process "$BuildDir\esp32_virtual_simulator.exe"
}

Write-Host "Done."
