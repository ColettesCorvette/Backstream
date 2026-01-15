#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

Write-Host "================================" -ForegroundColor Cyan
Write-Host "  BackStream - Build" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Checking requirements..." -ForegroundColor Yellow
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: CMake is not installed" -ForegroundColor Red
    Write-Host "   Installe CMake depuis: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "   Ou avec winget: winget install Kitware.CMake" -ForegroundColor Yellow
    exit 1
}

# Vérification MSVC
$vsPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -property installationPath 2>$null

if (-not $vsPath) {
    Write-Host "ERROR: Visual Studio 2022 not found" -ForegroundColor Red
    Write-Host "   Installe Visual Studio 2022 avec le workload 'Desktop development with C++'" -ForegroundColor Yellow
    Write-Host "   Ou lance ce script depuis 'Developer PowerShell for VS 2022'" -ForegroundColor Yellow
    exit 1
}

Write-Host "OK CMake: $(cmake --version | Select-Object -First 1)" -ForegroundColor Green
Write-Host "OK Visual Studio: $vsPath" -ForegroundColor Green
Write-Host ""

if (Test-Path "build") {
    Write-Host "Cleaning existing build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "build"
}

Write-Host "Creating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path "build" | Out-Null
Set-Location "build"

Write-Host ""
Write-Host "Configuring project with CMake..." -ForegroundColor Yellow
cmake .. -G "Visual Studio 17 2022" -A x64

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: CMake configuration failed" -ForegroundColor Red
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "Building in Release mode..." -ForegroundColor Yellow
cmake --build . --config Release

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "ERROR: Build failed" -ForegroundColor Red
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "================================" -ForegroundColor Green
Write-Host "BUILD SUCCESS" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""

$exePath = Join-Path (Get-Location) "Portable\backup.exe"
Write-Host "Executable: $exePath" -ForegroundColor Cyan
Write-Host ""

if (Test-Path "Portable\backup.exe") {
    Write-Host "OK File created successfully" -ForegroundColor Green
    $fileInfo = Get-Item "Portable\backup.exe"
    Write-Host "  Taille: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Gray
    Write-Host "  Date: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
} else {
    Write-Host "WARNING: Executable not found" -ForegroundColor Yellow
}

if (Test-Path "Portable\zstd.exe") {
    Write-Host "✓ zstd.exe copié" -ForegroundColor Green
} else {
    Write-Host "⚠️  zstd.exe manquant - copie-le dans tools/zstd.exe" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Pour tester:" -ForegroundColor Cyan
Write-Host "  cd build\Portable" -ForegroundColor Gray
Write-Host "  .\backup.exe" -ForegroundColor Gray
Write-Host ""

Set-Location ..
