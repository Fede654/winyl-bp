@echo off
REM Winyl Player - Automated Build Script
REM Milestone 1+: Modern build automation for Winyl audio player

echo ======================================================
echo  Winyl Player - Automated Build System
echo  Milestone 1 Complete: Build System Modernized
echo ======================================================
echo.

REM Check if MSBuild is available
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: MSBuild not found in PATH
    echo Please ensure Visual Studio 2019 Build Tools are installed
    echo and Developer Command Prompt is being used.
    echo.
    echo Quick fix: Run this from "Developer Command Prompt for VS 2019"
    pause
    exit /b 1
)

REM Set build configuration
set CONFIGURATION=Debug
set PLATFORM=x64
if not "%1"=="" set CONFIGURATION=%1
if not "%2"=="" set PLATFORM=%2

echo Configuration: %CONFIGURATION%
echo Platform: %PLATFORM%
echo.

REM Clean previous build if requested
if "%3"=="clean" (
    echo Cleaning previous build...
    if exist "Winyl\%PLATFORM%\%CONFIGURATION%" (
        rmdir /s /q "Winyl\%PLATFORM%\%CONFIGURATION%"
        echo Previous build cleaned.
    )
    echo.
)

echo Building Winyl Player...
echo Command: msbuild Winyl\Winyl.vcxproj /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM%
echo.

REM Execute the build
msbuild "Winyl\Winyl.vcxproj" /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% /v:minimal

REM Check build result
if %errorlevel% equ 0 (
    echo.
    echo ======================================================
    echo  BUILD SUCCESS!
    echo ======================================================
    echo.
    echo Executable generated: Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe
    echo Build time: %date% %time%
    echo.
    
    REM Display executable info
    if exist "Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe" (
        echo Executable details:
        dir "Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe" | findstr "Winyl.exe"
        echo.
    )
    
    echo Ready for development and testing!
    echo.
) else (
    echo.
    echo ======================================================
    echo  BUILD FAILED - Error Level: %errorlevel%
    echo ======================================================
    echo.
    echo Check the build output above for specific errors.
    echo Common issues:
    echo - Missing dependencies (TagLib, BASS libraries)
    echo - Incorrect Visual Studio version
    echo - Platform/Configuration mismatch
    echo.
)

REM Keep window open if run directly
if "%INTERACTIVE%"=="" pause