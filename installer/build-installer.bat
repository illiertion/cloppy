@echo off
setlocal
cd /d "%~dp0.."

set "IFW_DIR=C:\Qt\Tools\QtInstallerFramework\4.11\bin"

if not exist "%IFW_DIR%\binarycreator.exe" (
    echo Qt Installer Framework was not found at %IFW_DIR%.
    echo Install it from the Qt Maintenance Tool ^(Qt Installer Framework^) first.
    exit /b 1
)

if not exist "dist\Cloppy.exe" (
    echo dist\Cloppy.exe is missing. Run package.bat first.
    exit /b 1
)

set "DATA_DIR=installer\packages\com.cloppy.app\data"
if exist "%DATA_DIR%" rmdir /s /q "%DATA_DIR%"
mkdir "%DATA_DIR%"

xcopy /e /i /y /q "dist" "%DATA_DIR%" >nul
if errorlevel 1 (
    echo Copying dist failed.
    exit /b 1
)

rem License texts for the installer's license agreement page.
type LICENSE THIRD-PARTY-NOTICES.md LICENSES\LGPL-3.0.txt LICENSES\GPL-3.0.txt > installer\packages\com.cloppy.app\meta\license.txt
if errorlevel 1 (
    echo Writing the installer license text failed.
    exit /b 1
)

"%IFW_DIR%\binarycreator.exe" -c installer\config\config.xml -p installer\packages CloppySetup.exe
if errorlevel 1 (
    echo Installer creation failed.
    exit /b 1
)

echo.
echo Installer ready: %~dp0..\CloppySetup.exe
exit /b 0
