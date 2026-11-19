@echo off
setlocal
cd /d "%~dp0"

set "QT_DIR=C:\Qt\6.12.0\msvc2022_64"
set "VS_DIR=C:\Program Files\Microsoft Visual Studio\18\Community"

taskkill /IM Cloppy.exe /F >nul 2>&1
timeout /t 1 /nobreak >nul

if not exist "build\Cloppy.exe" (
    call build.bat
    if errorlevel 1 exit /b 1
)

if exist "dist" rmdir /s /q "dist"
mkdir "dist"
copy /y "build\Cloppy.exe" "dist\" >nul
if errorlevel 1 exit /b 1

"%QT_DIR%\bin\windeployqt.exe" --release --no-translations --qmldir qml --dir dist "dist\Cloppy.exe"
if errorlevel 1 exit /b 1

for /d %%V in ("%VS_DIR%\VC\Redist\MSVC\*") do (
    for /d %%C in ("%%V\x64\Microsoft.VC*.CRT") do (
        copy /y "%%C\*.dll" "dist\" >nul
    )
)

echo.
echo Portable folder ready: %~dp0dist
echo Run dist\Cloppy.exe - the whole folder is required.
exit /b 0
