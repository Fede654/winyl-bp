# Winyl Build Setup Log

## Environment Setup Progress

### Current Status: DEPENDENCIES MISSING
**Date**: 2024-01-20  
**Phase**: Initial Setup

### What We Have:
✅ Complete source code structure  
✅ Visual Studio solution file (Winyl.sln)  
✅ Project files and resource files  
✅ Skin system and UI assets  
✅ Documentation structure  

### What's Missing (CRITICAL):
❌ **BASS Audio Library** - Commercial library from Un4seen Developments  
❌ **TagLib** - Audio metadata library (needs custom compilation)  
❌ **SQLite3** - Database engine (amalgamation source)  
❌ **pugixml** - XML parsing library  
❌ **zlib** - Compression library (for PackSkin utility)  

## Dependencies Download Plan

### 1. SQLite3 (FREE - Start Here)
**Priority**: HIGH - Required for compilation  
**Source**: https://www.sqlite.org/download.html  
**File**: sqlite-amalgamation-3440200.zip (or latest)  
**Target**: `Winyl/src/sqlite3/sqlite3/src/`  

### 2. pugixml (FREE - Easy win)
**Priority**: HIGH - Required for XML parsing  
**Source**: https://pugixml.org/  
**File**: pugixml-1.13.tar.gz (or latest)  
**Target**: `Winyl/src/pugixml/`  
**Config**: Need to uncomment specific defines in pugiconfig.hpp  

### 3. TagLib (FREE - Complex build)
**Priority**: HIGH - Required for audio metadata  
**Source**: https://github.com/taglib/taglib  
**Build**: Requires CMake configuration  
**Patch**: Manual code changes needed (String::Latin1 → String::UTF16)  
**Target**: `Winyl/src/taglib/`  

### 4. zlib (FREE - Optional)
**Priority**: MEDIUM - Only needed for PackSkin utility  
**Source**: https://zlib.net/  
**File**: zlib source with Visual Studio projects  
**Target**: `Winyl/src/zlib/`  

### 5. BASS Audio Library (COMMERCIAL - Core dependency)
**Priority**: CRITICAL - This is the heart of Winyl  
**Source**: http://www.un4seen.com/  
**License**: Need to evaluate licensing for open source project  
**Files**: 15+ different DLL files and headers  
**Target**: `Winyl/src/bass/`  

**Required BASS packages:**
- bass24.zip (core)
- bassmix24.zip (mixing)
- bass_fx24.zip (effects)
- basswasapi24.zip (WASAPI)
- bassasio13.zip (ASIO)
- bass_aac24.zip (AAC support)
- bass_ape24.zip (APE support)
- bass_mpc24.zip (Musepack)
- bass_spx24.zip (Speex)
- bass_tta24.zip (TTA)
- bassalac24.zip (ALAC)
- bassflac24.zip (FLAC)
- bassopus24.zip (Opus)
- basswm24.zip (WMA)
- basswv24.zip (WavPack)

## Immediate Action Plan

### Phase 1: Get Free Dependencies (Day 1)
1. Download and setup SQLite3 amalgamation
2. Download and setup pugixml
3. Attempt initial compilation to see what breaks

### Phase 2: TagLib Complex Build (Days 2-3)
1. Clone TagLib repository
2. Configure CMake for Visual Studio
3. Apply manual patches as per BUILD.md
4. Build for both x86 and x64
5. Copy headers and libraries to correct locations

### Phase 3: BASS Evaluation (Days 4-5)
1. Research BASS licensing options for open source
2. Consider alternatives if licensing is prohibitive
3. Download BASS if possible, or plan alternative approach

### Phase 4: First Build Attempt (Day 6)
1. Try compilation with available dependencies
2. Document all errors and missing pieces
3. Create fallback plan if BASS is unavailable

## Risk Assessment

### High Risk: BASS Library Licensing
- **Problem**: BASS is commercial, may not be compatible with open source
- **Impact**: Without BASS, the core audio engine doesn't work
- **Mitigation**: Research licensing options, consider alternatives
- **Alternatives**: PortAudio + libsndfile + custom DSP (major rewrite)

### Medium Risk: TagLib Build Complexity
- **Problem**: Requires CMake, patches, complex configuration
- **Impact**: No audio metadata without TagLib
- **Mitigation**: Follow BUILD.md instructions exactly
- **Alternatives**: Direct file format parsing (limited functionality)

### Low Risk: Other Dependencies
- **Problem**: Standard libraries, should be straightforward
- **Impact**: Compilation errors but easy to resolve
- **Mitigation**: Standard download and setup procedures

## Next Steps

1. **IMMEDIATE**: Download SQLite3 and pugixml
2. **TODAY**: Setup basic dependencies and try compilation
3. **THIS WEEK**: Resolve TagLib build and BASS licensing
4. **DECISION POINT**: If BASS unavailable, pivot to alternative architecture

## Tools Needed

### Development Environment:
- Visual Studio 2017 with C++ tools ✅
- Windows 10 SDK ✅
- CMake (for TagLib) ❌
- Git (for dependency management) ✅

### Additional Requirements:
- BASS license evaluation
- Audio testing hardware
- Professional audio measurement tools

---

**Status**: Ready to download first dependencies  
**Next Action**: Download SQLite3 amalgamation  
**Blocker**: BASS library licensing decision