@echo off
REM Winyl Player - Automated Build Script
REM Usage: build.bat [configuration] [platform] [clean|rebuild|-f|--force] [--no-deploy]
REM Examples:
REM   build.bat                    - Debug x64 (incremental + deploy)
REM   build.bat Release            - Release x64 (incremental + deploy)  
REM   build.bat Debug x64 clean    - Debug x64 (force rebuild + deploy)
REM   build.bat --force            - Debug x64 (force rebuild + deploy)
REM   build.bat Release rebuild    - Release x64 (force rebuild + deploy)
REM   build.bat --no-deploy        - Debug x64 (no deployment)

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
set "CONFIGURATION=Debug"
set "PLATFORM=x64"
set "FORCE_REBUILD=false"
set "SKIP_DEPLOY=false"

REM Parse command line arguments
:parse_args
if "%1"=="" goto :done_parsing
if /i "%1"=="clean" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="rebuild" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="-f" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="--force" set "FORCE_REBUILD=true" & goto :next_arg
if /i "%1"=="--no-deploy" set "SKIP_DEPLOY=true" & goto :next_arg
if /i "%1"=="debug" set "CONFIGURATION=Debug" & goto :next_arg
if /i "%1"=="release" set "CONFIGURATION=Release" & goto :next_arg
if /i "%1"=="x64" set "PLATFORM=x64" & goto :next_arg
if /i "%1"=="x86" set "PLATFORM=x86" & goto :next_arg
REM If not recognized, assume it's configuration
set "CONFIGURATION=%1"
:next_arg
shift
goto :parse_args
:done_parsing

echo Configuration: %CONFIGURATION%
echo Platform: %PLATFORM%
echo Force rebuild: %FORCE_REBUILD%
echo Skip deployment: %SKIP_DEPLOY%
if /i "%FORCE_REBUILD%"=="true" echo Force rebuild: ENABLED
if /i "%SKIP_DEPLOY%"=="true" echo Deployment: SKIPPED
echo.

REM Check and build dependencies
echo ======================================================
echo  CHECKING DEPENDENCIES
echo ======================================================
echo.

REM Check SQLite dependency
set "SQLITE_LIB=Winyl\src\sqlite3\sqlite3\%PLATFORM%\%CONFIGURATION%\sqlite3.lib"
echo Checking SQLite library: %SQLITE_LIB%
if exist "%SQLITE_LIB%" (
    echo ✅ SQLite library found
    goto sqlite_found
) else (
    echo ❌ SQLite library missing - building dependency...
    echo.
    echo Building SQLite...
    pushd "Winyl\src\sqlite3" >nul
    
    REM Try building the solution first
    "%MSBUILD_EXE%" "sqlite3.sln" "/p:Configuration=%CONFIGURATION%" "/p:Platform=%PLATFORM%" /v:minimal >nul 2>&1
    if %errorlevel% equ 0 goto sqlite_success
    
    REM If solution fails, try building just the project with Release configuration
    echo Solution build failed, trying Release configuration...
    "%MSBUILD_EXE%" "sqlite3\sqlite3.vcxproj" "/p:Configuration=Release" "/p:Platform=%PLATFORM%" /v:minimal >nul 2>&1
    if %errorlevel% equ 0 (
        echo ⚠️  Built SQLite in Release mode (Debug not available)
        REM Copy Release lib to Debug location for compatibility
        if not exist "sqlite3\%PLATFORM%\Debug" mkdir "sqlite3\%PLATFORM%\Debug"
        copy "sqlite3\%PLATFORM%\Release\sqlite3.lib" "sqlite3\%PLATFORM%\Debug\sqlite3.lib" >nul 2>&1
        if %errorlevel% equ 0 (
            echo ✅ Copied Release lib to Debug location
        ) else (
            echo ❌ Failed to copy lib file
        )
        goto sqlite_success
    )
    
    REM If both fail, show error
    echo ❌ SQLite build failed with both Debug and Release
    "%MSBUILD_EXE%" "sqlite3\sqlite3.vcxproj" "/p:Configuration=%CONFIGURATION%" "/p:Platform=%PLATFORM%" /v:minimal
    popd >nul
    pause
    exit /b 1
    
    :sqlite_success
    popd >nul
    
    if exist "%SQLITE_LIB%" (
        echo ✅ SQLite library built successfully
    ) else (
        echo ❌ SQLite library still missing after build
        pause
        exit /b 1
    )
)

:sqlite_found

REM Check BASS libraries  
echo.
echo Checking BASS libraries...
set BASS_DLLS_EXIST=true
set BASS_HEADERS_EXIST=true

REM Check platform-specific DLLs
if "%PLATFORM%"=="x64" (
    if not exist "Winyl\src\bass\x64\bass.dll" set BASS_DLLS_EXIST=false
    if not exist "Winyl\src\bass\x64\bassmix.dll" set BASS_DLLS_EXIST=false
) else (
    if not exist "Winyl\src\bass\bass.dll" set BASS_DLLS_EXIST=false
    if not exist "Winyl\src\bass\bassmix.dll" set BASS_DLLS_EXIST=false
)

REM Check headers
if not exist "Winyl\src\bass\bass.h" set BASS_HEADERS_EXIST=false
if not exist "Winyl\src\bass\bassmix.h" set BASS_HEADERS_EXIST=false

if "%BASS_DLLS_EXIST%"=="false" (
    echo ❌ BASS DLLs missing in Winyl\src\bass\%PLATFORM%\
    echo Please ensure BASS audio library files are properly installed
    echo Required files: bass.dll, bassmix.dll, bassasio.dll, basswasapi.dll
    pause
    exit /b 1
) else if "%BASS_HEADERS_EXIST%"=="false" (
    echo ❌ BASS headers missing in Winyl\src\bass\
    echo Required files: bass.h, bassmix.h, bassasio.h, basswasapi.h
    pause
    exit /b 1
) else (
    echo ✅ BASS libraries and headers found
)

echo.
echo ======================================================
echo  DEPENDENCIES CHECK COMPLETE
echo ======================================================
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
    
    REM Deploy BASS DLLs and data files if not skipped
    if /i "%SKIP_DEPLOY%"=="false" (
        echo.
        echo ======================================================
        echo  DEPLOYING RUNTIME DEPENDENCIES
        echo ======================================================
        echo.
        echo Copying BASS libraries and data files...
        
        REM Copy BASS DLLs
        if "%PLATFORM%"=="x64" (
            copy "Winyl\src\bass\x64\*.dll" "Winyl\%PLATFORM%\%CONFIGURATION%\" /Y >nul 2>&1
        ) else (
            copy "Winyl\src\bass\*.dll" "Winyl\%PLATFORM%\%CONFIGURATION%\" /Y >nul 2>&1
        )
        if %errorlevel%==0 (
            echo ✅ BASS libraries copied
        ) else (
            echo ❌ Failed to copy BASS libraries
        )
        
        REM Create and populate data directory  
        if not exist "Winyl\%PLATFORM%\%CONFIGURATION%\data" mkdir "Winyl\%PLATFORM%\%CONFIGURATION%\data"
        xcopy "Winyl\data" "Winyl\%PLATFORM%\%CONFIGURATION%\data" /E /I /Y /EXCLUDE:Winyl\exclude_recursive.txt >nul 2>&1
        if %errorlevel%==0 (
            echo ✅ Data folder deployed
        ) else (
            echo ❌ Failed to deploy data folder
        )
        
        echo.
        echo 🎵 Deployment complete! Ready to run: Winyl\%PLATFORM%\%CONFIGURATION%\Winyl.exe
        echo.
    ) else (
        echo.
        echo ⚠️  Deployment skipped - use deploy_debug.bat manually if needed
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