@echo off
setlocal
cd /d "%~dp0"

set "QT_DIR=C:\Qt\6.12.0\msvc2022_64"
set "VS_DIR=C:\Program Files\Microsoft Visual Studio\18\Community"
set "CMAKE_EXE=%VS_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "NINJA_EXE=%VS_DIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"

if not exist "%QT_DIR%\bin\qmake.exe" (
    echo Qt was not found at %QT_DIR%.
    exit /b 1
)

taskkill /IM Cloppy.exe /F >nul 2>&1

call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

set "PATH=%QT_DIR%\bin;%PATH%"

"%CMAKE_EXE%" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"
if errorlevel 1 exit /b 1

if exist "build\Cloppy.exe" del /f /q "build\Cloppy.exe"

"%CMAKE_EXE%" --build build
if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

if not exist "build\Cloppy.exe" (
    echo.
    echo Build failed: Cloppy.exe was not produced.
    exit /b 1
)

"%QT_DIR%\bin\windeployqt.exe" --qmldir qml --no-translations build\Cloppy.exe
if errorlevel 1 exit /b 1

echo.
echo Build complete: %~dp0build\Cloppy.exe
exit /b 0
