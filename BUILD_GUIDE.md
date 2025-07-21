# Winyl Build Guide

## Quick Start

### Option 1: Use the Fixed Build Script (Recommended)
```bash
# From MINGW64/Git Bash:
./build.bat

# Or specify configuration:
./build.bat Release x64
./build.bat Debug x64 clean
```

### Option 2: Use PowerShell Script
```bash
# From MINGW64/Git Bash:
powershell -ExecutionPolicy Bypass -File build.ps1

# With parameters:
powershell -ExecutionPolicy Bypass -File build.ps1 -Configuration Release -Platform x64 -Clean
```

### Option 3: Direct MSBuild (if you have VS tools in PATH)
```bash
# From regular command prompt:
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" Winyl\Winyl.vcxproj /p:Configuration=Debug /p:Platform=x64
```

## Build Output
- **Executable Location**: `Winyl\x64\Debug\Winyl.exe` (for Debug builds)
- **Libraries**: Automatically linked from `Winyl\src\bass\x64\`

## Prerequisites
- Visual Studio 2019 Build Tools or Community Edition
- Windows SDK 10.0 (should be included with VS)

## Troubleshooting

### "MSBuild not found"
1. Install Visual Studio 2019 Community Edition from: https://visualstudio.microsoft.com/vs/older-downloads/
2. Or install just the Build Tools: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019

### "BASS library errors"
- The x64 BASS libraries are included in `Winyl\src\bass\x64\`
- Make sure you're building for x64 platform

### "TagLib errors"
- All TagLib source files are integrated (51 implementation files)
- Should build without external TagLib dependencies

## Project Status
- ✅ **Milestone 1**: Build system modernized (VS2017→VS2019)
- ✅ **Milestone 2**: Complete TagLib integration (51 files, all audio formats)  
- ✅ **Milestone 3**: BASS audio libraries integrated (x64)

## Current Capabilities
- **Audio Metadata**: Full TagLib support for MP3, FLAC, OGG, MP4, WMA, etc.
- **Audio Playback**: BASS, BASS_FX, BASS_Mix, BASS_WASAPI, BASS_ASIO
- **Platform**: Windows x64, Visual Studio 2019+
- **Libraries**: Static TagLib, Dynamic BASS

Ready for modern Windows audio player development!