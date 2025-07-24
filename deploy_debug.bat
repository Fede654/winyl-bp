@echo off
echo === WINYL DEBUG DEPLOYMENT ===
echo Deploying BASS libraries and data files...

REM Copy BASS DLLs
copy "Winyl\src\bass\x64\*.dll" "Winyl\x64\Debug\" /Y >nul 2>&1
if %errorlevel%==0 (
    echo ✅ BASS libraries copied
) else (
    echo ❌ Failed to copy BASS libraries
)

REM Create and populate data directory  
if not exist "Winyl\x64\Debug\data" mkdir "Winyl\x64\Debug\data"
xcopy "Winyl\data" "Winyl\x64\Debug\data" /E /I /Y >nul 2>&1
if %errorlevel%==0 (
    echo ✅ Data folder deployed
) else (
    echo ❌ Failed to deploy data folder
)

echo.
echo 🎵 Winyl debug deployment complete!
echo Ready to run: Winyl\x64\Debug\Winyl.exe
echo.