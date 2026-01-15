#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator" -ForegroundColor Red
    Write-Host "Right-click and select 'Run as Administrator'" -ForegroundColor Yellow
    pause
    exit 1
}

Write-Host "================================" -ForegroundColor Cyan
Write-Host "  BackStream - Install" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

$exePath = Join-Path $PSScriptRoot "build\Portable"

if (-not (Test-Path $exePath)) {
    Write-Host "ERROR: Executable directory not found: $exePath" -ForegroundColor Red
    Write-Host "Build the project first using: .\build.ps1" -ForegroundColor Yellow
    pause
    exit 1
}

if (-not (Test-Path "$exePath\backup.exe")) {
    Write-Host "ERROR: backup.exe not found in: $exePath" -ForegroundColor Red
    Write-Host "Build the project first using: .\build.ps1" -ForegroundColor Yellow
    pause
    exit 1
}

$fullPath = (Resolve-Path $exePath).Path

Write-Host "Executable location: $fullPath" -ForegroundColor Gray
Write-Host ""

$currentPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::Machine)

if ($currentPath -split ';' | Where-Object { $_ -eq $fullPath }) {
    Write-Host "Already installed - Path already in system PATH" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can run 'backup' from anywhere now" -ForegroundColor Cyan
    pause
    exit 0
}

# Add to PATH
$newPath = $currentPath + ";" + $fullPath

try {
    [Environment]::SetEnvironmentVariable("Path", $newPath, [EnvironmentVariableTarget]::Machine)
    Write-Host ""
    Write-Host "================================" -ForegroundColor Green
    Write-Host "  INSTALLATION SUCCESSFUL!" -ForegroundColor Green
    Write-Host "================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Path added: $fullPath" -ForegroundColor Gray
    Write-Host ""
    Write-Host "IMPORTANT: Restart your terminal for changes to take effect" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Then you can run 'backup' from any directory" -ForegroundColor Cyan
    Write-Host ""
} catch {
    Write-Host ""
    Write-Host "ERROR: Failed to update PATH" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    pause
    exit 1
}

pause
