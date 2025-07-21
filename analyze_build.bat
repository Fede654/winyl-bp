@echo off
REM Build Log Analyzer for Winyl
REM Usage: analyze_build.bat [logfile]

if "%1"=="" (
    echo Usage: analyze_build.bat [logfile]
    echo.
    echo Available log files:
    dir build_log_*.txt /b 2>nul || echo No build logs found
    echo.
    pause
    exit /b 1
)

set LOGFILE=%1
if not exist "%LOGFILE%" (
    echo Error: Log file '%LOGFILE%' not found
    pause
    exit /b 1
)

echo ======================================================
echo  Winyl Build Log Analysis: %LOGFILE%
echo ======================================================
echo.

echo === LINK ERRORS (LNK2019) ===
findstr /C:"error LNK2019" "%LOGFILE%" | findstr /C:"símbolo externo" > temp_link_errors.txt 2>nul
if exist temp_link_errors.txt (
    for /f %%i in ('find /c /v "" ^< temp_link_errors.txt') do echo Found %%i linker errors
    echo.
    echo First 10 errors:
    head -10 temp_link_errors.txt 2>nul || more +1 temp_link_errors.txt | head -10
    del temp_link_errors.txt
) else (
    echo No LNK2019 errors found
)

echo.
echo === LIBRARY CONFLICTS (LNK4272) ===
findstr /C:"LNK4272" "%LOGFILE%" > temp_lib_conflicts.txt 2>nul
if exist temp_lib_conflicts.txt (
    type temp_lib_conflicts.txt
    del temp_lib_conflicts.txt
) else (
    echo No library conflicts found
)

echo.
echo === DUPLICATE FILES (MSB8027) ===
findstr /C:"MSB8027" "%LOGFILE%" | find /c "archivos con el mismo nombre" > nul
if %errorlevel% equ 0 (
    findstr /C:"MSB8027" "%LOGFILE%" | head -5 2>nul || findstr /C:"MSB8027" "%LOGFILE%"
    echo ... (check full log for complete list)
) else (
    echo No duplicate file warnings found
)

echo.
echo === SUMMARY ===
findstr /C:"error" "%LOGFILE%" | find /c "error" > nul
if %errorlevel% equ 0 (
    for /f %%i in ('findstr /C:"error" "%LOGFILE%" ^| find /c "error"') do echo Total errors: %%i
) else (
    echo No errors found
)

findstr /C:"warning" "%LOGFILE%" | find /c "warning" > nul  
if %errorlevel% equ 0 (
    for /f %%i in ('findstr /C:"warning" "%LOGFILE%" ^| find /c "warning"') do echo Total warnings: %%i
) else (
    echo No warnings found
)

echo.
echo Full log available at: %LOGFILE%
echo.
pause