# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Winyl is an open-source music player for Windows written in C++. It features a custom UI framework with skinning support, audio playback via BASS library, and SQLite for data persistence.

## Build Commands

### Building the Project
```bash
# Open Visual Studio 2017 solution
devenv Winyl.sln

# Build from command line (requires VS 2017 with Windows XP support)
msbuild Winyl.sln /p:Configuration=Release /p:Platform=x86
msbuild Winyl.sln /p:Configuration=Release /p:Platform=x64
msbuild Winyl.sln /p:Configuration=Debug /p:Platform=x86
msbuild Winyl.sln /p:Configuration=Debug /p:Platform=x64
```

### Prerequisites
- Visual Studio 2017 with Windows XP support for C++ (v141_xp toolset)
- Pre-built third-party libraries (see BUILD.md for compilation instructions)
- Dependencies must be manually compiled and placed in correct directories

## Architecture

### Core Components
1. **WinylApp** (Winyl.cpp): Application entry point and lifecycle management
2. **WinylWnd** (WinylWnd.cpp): Main window implementation
3. **LibAudio**: Audio playback engine using BASS library
4. **DBase**: SQLite database wrapper for library and playlist management
5. **Skin System**: Custom UI framework with XML-based skins

### Key Design Patterns
- Single instance application using mutex
- Message-driven Windows architecture
- Custom window procedure chain: WindowProc → WindowProcEx → WindowProcMy
- No standard Windows controls - all UI elements are custom drawn

### Database Structure
Main library table with 50+ columns for music metadata:
- Core fields: path, file, title, artist, album, year, genre
- Technical: bitrate, samplerate, duration, channels
- Extended: rating, playcount, lastplayed, replaygain values

### Dependencies
- **SQLite**: Database engine (src/sqlite3/)
- **TagLib**: Audio metadata library (src/taglib/)
- **BASS**: Commercial audio library (src/bass/)
- **pugixml**: XML parsing (src/pugixml/)
- **zlib**: Compression for skin files (src/zlib/)

### File Organization
```
Winyl/
├── src/              # All source code
│   ├── *.cpp/h      # Main application files
│   ├── bass/        # BASS audio library headers
│   ├── taglib/      # TagLib headers and libs
│   ├── sqlite3/     # SQLite source
│   └── pugixml/     # XML parser
├── data/            # Runtime resources
│   ├── Language/    # Localization files (28 languages)
│   ├── Skin/        # UI themes
│   └── x86/x64/     # Platform-specific DLLs
└── Winyl.vcxproj    # Visual Studio project file
```

## Development Notes

### Character Encoding
- Uses Unicode throughout (UNICODE and _UNICODE defined)
- All string handling uses wide characters (wchar_t)

### Platform Support
- Windows XP and later (using v141_xp toolset)
- Both x86 and x64 architectures
- Portable mode support (settings in program directory)

### UI Development
- All UI elements derive from SkinElement base class
- Layouts defined in XML files in data/Skin/
- Custom drawing using GDI+ and Direct2D
- No resource files - all UI defined in code or XML

### Audio Features
- Gapless playback and crossfading
- Multiple output methods: DirectSound, WASAPI, ASIO
- Format support via BASS plugins: MP3, FLAC, OGG, WMA, AAC, etc.
- CUE sheet support with separate database

### Testing
No formal testing framework is present. Manual testing required for all changes.

### Common Tasks
- To add a new UI element: Derive from SkinElement and implement drawing
- To add audio format support: Add corresponding BASS plugin
- To modify database schema: Update table definitions in database creation code
- Language files are in data/Language/ as UTF-16 LE text files

## Memories

- Remember this last successful build line for future