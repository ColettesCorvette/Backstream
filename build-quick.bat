@echo off

echo ================================
echo   Launching build...
echo ================================
echo.

where pwsh >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Using PowerShell Core...
    pwsh -ExecutionPolicy Bypass -File "%~dp0build.ps1"
) else (
    where powershell >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        echo Using Windows PowerShell...
        powershell -ExecutionPolicy Bypass -File "%~dp0build.ps1"
    ) else (
        echo ERROR: PowerShell not available
        pause
        exit /b 1
    )
)

pause
