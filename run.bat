@echo off
cd /d "%~dp0"
if not exist "build\Cloppy.exe" (
    echo Cloppy is not built yet. Running build.bat first...
    call build.bat
    if errorlevel 1 exit /b 1
)
start "" "%~dp0build\Cloppy.exe"
