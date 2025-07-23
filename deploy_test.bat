@echo off
REM Winyl - Automated Test Deployment Script
REM Usage: deploy_test.bat [configuration] [platform]
REM Purpose: Copy all required DLLs and plugins after build for testing

echo ======================================================
echo  Winyl Test Deployment
echo  Setting up runtime environment for testing
echo ======================================================
echo.

REM Set build configuration
set CONFIGURATION=Debug
set PLATFORM=x64
if not "%1"=="" set CONFIGURATION=%1
if not "%2"=="" set PLATFORM=%2

set BUILD_DIR=Winyl\%PLATFORM%\%CONFIGURATION%
set BASS_SOURCE=Winyl\src\bass\x64
set PLUGINS_SOURCE=temp_downloads\x64

echo Configuration: %CONFIGURATION%
echo Platform: %PLATFORM%
echo Build directory: %BUILD_DIR%
echo.

REM Check if executable exists
if not exist "%BUILD_DIR%\Winyl.exe" (
    echo ERROR: Winyl.exe not found in %BUILD_DIR%
    echo Please run build.bat first to compile the project.
    echo.
    pause
    exit /b 1
)

echo Deploying BASS libraries...
REM Copy main BASS DLLs to executable directory
copy "%BASS_SOURCE%\bass.dll" "%BUILD_DIR%\" >nul
copy "%BASS_SOURCE%\bassmix.dll" "%BUILD_DIR%\" >nul  
copy "%BASS_SOURCE%\basswasapi.dll" "%BUILD_DIR%\" >nul
copy "%BASS_SOURCE%\bassasio.dll" "%BUILD_DIR%\" >nul

REM Create Bass plugins directory
if not exist "%BUILD_DIR%\Bass" mkdir "%BUILD_DIR%\Bass"

echo Deploying BASS plugins...
REM Copy available BASS plugins
if exist "%PLUGINS_SOURCE%\bassflac.dll" (
    copy "%PLUGINS_SOURCE%\bassflac.dll" "%BUILD_DIR%\Bass\" >nul
    echo  ✓ FLAC support (bassflac.dll)
) else (
    echo  ⚠ FLAC plugin not found - download bassflac24.zip from un4seen.com
)

REM Check for other common plugins and copy if available
if exist "%PLUGINS_SOURCE%\bass_ape.dll" (
    copy "%PLUGINS_SOURCE%\bass_ape.dll" "%BUILD_DIR%\Bass\" >nul
    echo  ✓ APE support (bass_ape.dll)
) else (
    echo  ⚠ APE plugin not available
)

if exist "%PLUGINS_SOURCE%\bass_aac.dll" (
    copy "%PLUGINS_SOURCE%\bass_aac.dll" "%BUILD_DIR%\Bass\" >nul
    echo  ✓ AAC support (bass_aac.dll)
) else (
    echo  ⚠ AAC plugin not available
)

if exist "%PLUGINS_SOURCE%\bassopus.dll" (
    copy "%PLUGINS_SOURCE%\bassopus.dll" "%BUILD_DIR%\Bass\" >nul
    echo  ✓ Opus support (bassopus.dll)
) else (
    echo  ⚠ Opus plugin not available
)

echo.
echo ======================================================
echo  DEPLOYMENT COMPLETE
echo ======================================================
echo.

REM Display deployment summary
echo Runtime Environment Status:
echo  📁 Executable: %BUILD_DIR%\Winyl.exe
for %%i in ("%BUILD_DIR%\Winyl.exe") do echo     Size: %%~zi bytes  Modified: %%~ti

echo.
echo  🔊 BASS Audio Libraries:
if exist "%BUILD_DIR%\bass.dll" echo     ✓ bass.dll (core audio engine)
if exist "%BUILD_DIR%\bassmix.dll" echo     ✓ bassmix.dll (mixing capabilities)
if exist "%BUILD_DIR%\basswasapi.dll" echo     ✓ basswasapi.dll (WASAPI support)
if exist "%BUILD_DIR%\bassasio.dll" echo     ✓ bassasio.dll (ASIO support)

echo.
echo  🎵 Format Support Plugins:
if exist "%BUILD_DIR%\Bass\bassflac.dll" (
    echo     ✓ FLAC support enabled
) else (
    echo     ❌ FLAC support missing
)

dir "%BUILD_DIR%\Bass\" /b 2>nul | find ".dll" >nul
if %errorlevel% equ 0 (
    echo     📂 Available plugins in Bass\ directory:
    for %%f in ("%BUILD_DIR%\Bass\*.dll") do echo        - %%~nxf
) else (
    echo     ⚠ No plugins found in Bass\ directory
)

echo.
echo ======================================================
echo  TESTING COMMANDS
echo ======================================================
echo.
echo Ready for testing! Use these commands:
echo.
echo  🚀 Launch Winyl:
echo     cd "%BUILD_DIR%" ^&^& Winyl.exe
echo.
echo  📊 Quick test (3 seconds):
echo     cd "%BUILD_DIR%" ^&^& timeout 3 Winyl.exe
echo.
echo  🔍 Test with specific audio file:
echo     cd "%BUILD_DIR%" ^&^& Winyl.exe "path\to\your\file.flac"
echo.
echo  📝 View build log:
echo     notepad build_log_*.txt
echo.

REM Keep window open
pause