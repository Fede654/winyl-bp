@echo off
REM Winyl - Complete Build and Test Workflow
REM Usage: build_and_test.bat [configuration] [platform] [--force]
REM Purpose: Build project, deploy runtime, and prepare for testing

echo ======================================================
echo  Winyl - Complete Build and Test Workflow
echo  Professional Audio Player Development
echo ======================================================
echo.

REM Parse arguments
set CONFIGURATION=Debug
set PLATFORM=x64
set BUILD_FLAGS=

:parse_args
if "%1"=="" goto :done_parsing
if /i "%1"=="debug" set CONFIGURATION=Debug & goto :next_arg
if /i "%1"=="release" set CONFIGURATION=Release & goto :next_arg
if /i "%1"=="x64" set PLATFORM=x64 & goto :next_arg
if /i "%1"=="x86" set PLATFORM=x86 & goto :next_arg
if /i "%1"=="--force" set BUILD_FLAGS=%BUILD_FLAGS% --force & goto :next_arg
if /i "%1"=="-f" set BUILD_FLAGS=%BUILD_FLAGS% --force & goto :next_arg
if /i "%1"=="clean" set BUILD_FLAGS=%BUILD_FLAGS% clean & goto :next_arg
if /i "%1"=="rebuild" set BUILD_FLAGS=%BUILD_FLAGS% rebuild & goto :next_arg
REM If not recognized, assume it's configuration
set CONFIGURATION=%1
:next_arg
shift
goto :parse_args
:done_parsing

echo Workflow Parameters:
echo  Configuration: %CONFIGURATION%
echo  Platform: %PLATFORM%
echo  Build flags: %BUILD_FLAGS%
echo.

echo ======================================================
echo  PHASE 1: BUILDING PROJECT
echo ======================================================
echo.

REM Execute build
call build.bat %CONFIGURATION% %PLATFORM% %BUILD_FLAGS%

REM Check build result
if %errorlevel% neq 0 (
    echo.
    echo ❌ BUILD FAILED - Stopping workflow
    echo Check build logs for details.
    pause
    exit /b %errorlevel%
)

echo.
echo ✅ BUILD SUCCESSFUL - Proceeding to deployment
echo.

echo ======================================================
echo  PHASE 2: DEPLOYING RUNTIME ENVIRONMENT
echo ======================================================
echo.

REM Execute deployment
call deploy_test.bat %CONFIGURATION% %PLATFORM%

echo.
echo ======================================================
echo  PHASE 3: DEVELOPMENT TESTING READY
echo ======================================================
echo.

set BUILD_DIR=Winyl\%PLATFORM%\%CONFIGURATION%

echo 🎯 TESTING CHECKLIST:
echo.
echo  [ ] 1. Basic startup test
echo      Command: cd "%BUILD_DIR%" ^&^& timeout 5 Winyl.exe
echo.
echo  [ ] 2. UI functionality test  
echo      - Check if interface loads correctly
echo      - Verify menu and controls respond
echo.
echo  [ ] 3. Audio file loading test
echo      - Try loading a FLAC file
echo      - Verify metadata display (TagLib integration)
echo.
echo  [ ] 4. Audio playback test
echo      - Test basic play/pause/stop
echo      - Verify BASS audio engine works
echo.
echo  [ ] 5. Advanced testing
echo      - Test different audio formats
echo      - Check WASAPI/ASIO output options
echo.

echo 💡 QUICK TESTS:
echo.
echo  Test 1 - Startup verification:
echo    cd "%BUILD_DIR%" ^&^& echo "Testing startup..." ^&^& timeout 3 Winyl.exe ^&^& echo "✅ Winyl started successfully"
echo.
echo  Test 2 - Manual testing:
echo    cd "%BUILD_DIR%" ^&^& Winyl.exe
echo.

echo ======================================================
echo  WORKFLOW COMPLETE
echo ======================================================
echo.
echo Ready for development testing!
echo Build: %CONFIGURATION% %PLATFORM%
echo Location: %BUILD_DIR%\Winyl.exe
echo.

REM Ask user what to do next
echo What would you like to do next?
echo  [1] Run quick startup test (3 seconds)
echo  [2] Launch Winyl for manual testing
echo  [3] Just exit (testing ready)
echo.
set /p choice="Enter your choice (1-3): "

if "%choice%"=="1" (
    echo.
    echo Running quick startup test...
    cd "%BUILD_DIR%" && timeout 3 Winyl.exe
    echo Test completed.
) else if "%choice%"=="2" (
    echo.
    echo Launching Winyl for manual testing...
    cd "%BUILD_DIR%" && start Winyl.exe
    echo Winyl launched. Test manually and close when done.
) else (
    echo.
    echo Testing environment ready. Use the commands above when ready to test.
)

echo.
pause