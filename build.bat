@echo off
REM Winyl Player - Automated Build Script
REM Usage: build.bat [configuration] [platform] [clean|rebuild|-f|--force]
REM Examples:
REM   build.bat                    - Debug x64 (incremental)
REM   build.bat Release            - Release x64 (incremental)  
REM   build.bat Debug x64 clean    - Debug x64 (force rebuild)
REM   build.bat --force            - Debug x64 (force rebuild)
REM   build.bat Release rebuild    - Release x64 (force rebuild)

echo ======================================================
echo  Winyl - Professional Audio Player Build System
echo  High-fidelity music player for audio enthusiasts
echo ======================================================
echo.

REM Find MSBuild - try multiple locations
set MSBUILD_EXE=
where msbuild >nul 2>&1
if %errorlevel% equ 0 (
    set MSBUILD_EXE=msbuild
) else (
    REM Try VS2019 Community
    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    ) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe" (
        set "MSBUILD_EXE=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    )
)

if "%MSBUILD_EXE%"=="" (
    echo ERROR: MSBuild not found
    echo Please install Visual Studio 2019 Build Tools or Community Edition
    echo.
    echo Checked locations:
    echo - PATH environment variable
    echo - VS2019 Community/Professional/Enterprise/BuildTools
    pause
    exit /b 1
)

echo Found MSBuild: %MSBUILD_EXE%

REM Set build configuration
set CONFIGURATION=Debug
set PLATFORM=x64
set FORCE_REBUILD=false

REM Parse command line arguments
:parse_args
if "%1"=="" goto :done_parsing
if /i "%1"=="clean" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="rebuild" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="-f" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="--force" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="debug" set CONFIGURATION=Debug & goto :next_arg
if /i "%1"=="release" set CONFIGURATION=Release & goto :next_arg
if /i "%1"=="x64" set PLATFORM=x64 & goto :next_arg
if /i "%1"=="x86" set PLATFORM=x86 & goto :next_arg
REM If not recognized, assume it's configuration
set CONFIGURATION=%1
:next_arg
shift
goto :parse_args
:done_parsing

echo Configuration: %CONFIGURATION%
echo Platform: %PLATFORM%
echo Force rebuild: %FORCE_REBUILD%
if /i "%FORCE_REBUILD%"=="true" echo Force rebuild: ENABLED
echo.

REM Clean previous build if force rebuild requested  
if /i "%FORCE_REBUILD%"=="true" (
    echo Forcing clean rebuild...
    if exist "Winyl\%PLATFORM%\%CONFIGURATION%" (
        rmdir /s /q "Winyl\%PLATFORM%\%CONFIGURATION%"
        echo Previous build cleaned.
    )
    echo.
)

echo Building Winyl Player...
echo Command: "%MSBUILD_EXE%" Winyl\Winyl.vcxproj /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM%
echo.

REM Execute the build with logging
set BUILD_LOG=build_log_%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%.txt
set BUILD_LOG=%BUILD_LOG: =0%
echo Saving build output to: %BUILD_LOG%

REM Store pre-build file time for comparison
set PRE_BUILD_TIME=not_found
if exist "Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe" (
    for %%i in ("Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe") do set PRE_BUILD_TIME=%%~ti
)

if /i "%FORCE_REBUILD%"=="true" (
    "%MSBUILD_EXE%" "Winyl\Winyl.vcxproj" /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% /t:Rebuild /v:minimal > "%BUILD_LOG%" 2>&1
) else (
    "%MSBUILD_EXE%" "Winyl\Winyl.vcxproj" /p:Configuration=%CONFIGURATION% /p:Platform=%PLATFORM% /v:minimal > "%BUILD_LOG%" 2>&1
)

REM Check build result
if %errorlevel% equ 0 (
    echo.
    
    REM Check if executable was actually rebuilt
    set POST_BUILD_TIME=not_found
    set WAS_REBUILT=false
    if exist "Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe" (
        for %%i in ("Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe") do set POST_BUILD_TIME=%%~ti
        if not "%PRE_BUILD_TIME%"=="%POST_BUILD_TIME%" set WAS_REBUILT=true
        if /i "%FORCE_REBUILD%"=="true" set WAS_REBUILT=true
    )
    
    if "%WAS_REBUILT%"=="true" (
        echo ======================================================
        echo  BUILD SUCCESS - EXECUTABLE REBUILT!
        echo ======================================================
        echo.
        echo New executable: Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe
    ) else (
        echo ======================================================
        echo  BUILD SUCCESS - UP TO DATE
        echo ======================================================
        echo.
        echo Existing executable: Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe
        echo No changes detected, using incremental build
    )
    
    echo Build time: %date% %time%
    echo Build log saved: %BUILD_LOG%
    echo.
    
    REM Display executable info
    if exist "Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe" (
        pushd "Winyl\%PLATFORM%\%CONFIGURATION%" >nul
        for %%i in (Winyl.exe) do echo File: %%~ti  %%~zi bytes  %%i
        popd >nul
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
    echo Build log saved: %BUILD_LOG%
    echo.
    echo For troubleshooting, analyze the build log:
    echo   analyze_build.bat "%BUILD_LOG%"
    echo.
)

REM Keep window open if run directly
if "%INTERACTIVE%"=="" pause