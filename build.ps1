# Winyl Player - PowerShell Build Script
# Alternative build script for MINGW64/Git Bash users
# Usage: powershell -ExecutionPolicy Bypass -File build.ps1 [Configuration] [Platform]

param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$Clean
)

Write-Host "======================================================" -ForegroundColor Cyan
Write-Host " Winyl Player - PowerShell Build System" -ForegroundColor Cyan
Write-Host " Milestone 1+ Complete: Build System Modernized" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# Find MSBuild
$msbuildPaths = @(
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
)

$msbuild = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path $path) {
        $msbuild = $path
        break
    }
}

if (-not $msbuild) {
    # Try to find in PATH
    $msbuild = Get-Command msbuild.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

if (-not $msbuild) {
    Write-Host "ERROR: MSBuild not found" -ForegroundColor Red
    Write-Host "Please install Visual Studio 2019+ Build Tools or Community Edition" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Checked locations:" -ForegroundColor Yellow
    $msbuildPaths | ForEach-Object { Write-Host "- $_" -ForegroundColor Gray }
    exit 1
}

Write-Host "Found MSBuild: $msbuild" -ForegroundColor Green
Write-Host "Configuration: $Configuration" -ForegroundColor Yellow
Write-Host "Platform: $Platform" -ForegroundColor Yellow
Write-Host ""

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning previous build..." -ForegroundColor Yellow
    $cleanPath = "Winyl\$Platform\$Configuration"
    if (Test-Path $cleanPath) {
        Remove-Item $cleanPath -Recurse -Force
        Write-Host "Previous build cleaned." -ForegroundColor Green
    }
    Write-Host ""
}

# Execute build
Write-Host "Building Winyl Player..." -ForegroundColor Cyan
Write-Host "Command: `"$msbuild`" Winyl\Winyl.vcxproj /p:Configuration=$Configuration /p:Platform=$Platform" -ForegroundColor Gray
Write-Host ""

& $msbuild "Winyl\Winyl.vcxproj" /p:Configuration=$Configuration /p:Platform=$Platform /v:minimal

# Check result
if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Green
    Write-Host " BUILD SUCCESS!" -ForegroundColor Green
    Write-Host "======================================================" -ForegroundColor Green
    Write-Host ""
    
    $exePath = "Winyl\$Platform\$Configuration\Winyl.exe"
    Write-Host "Executable: $exePath" -ForegroundColor Yellow
    
    if (Test-Path $exePath) {
        $exe = Get-Item $exePath
        Write-Host "Size: $($exe.Length) bytes" -ForegroundColor Green
        Write-Host "Modified: $($exe.LastWriteTime)" -ForegroundColor Green
        Write-Host ""
        Write-Host "Ready for testing!" -ForegroundColor Green
    } else {
        Write-Host "Warning: Executable not found at expected location" -ForegroundColor Yellow
    }
} else {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Red
    Write-Host " BUILD FAILED - Exit Code: $LASTEXITCODE" -ForegroundColor Red
    Write-Host "======================================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "Check the build output above for specific errors." -ForegroundColor Yellow
    Write-Host "Common issues:" -ForegroundColor Yellow
    Write-Host "- Missing dependencies (TagLib, BASS libraries)" -ForegroundColor Gray
    Write-Host "- Platform/Configuration mismatch" -ForegroundColor Gray
    Write-Host "- Linker errors" -ForegroundColor Gray
}

Write-Host ""
Write-Host "Press any key to continue..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")