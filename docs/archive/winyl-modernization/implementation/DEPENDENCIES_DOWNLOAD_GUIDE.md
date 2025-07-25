# Dependencies Download Guide

## IMMEDIATE ACTIONS NEEDED

### 1. SQLite3 Amalgamation (REQUIRED - 5 minutes)
**URL**: https://www.sqlite.org/download.html  
**File**: `sqlite-amalgamation-3500300.zip` (or latest version)  
**Size**: ~2.7 MB  

**Steps:**
1. Download the amalgamation ZIP file
2. Extract to temporary folder
3. Copy these files to `C:\Users\Fede\REPOS\winyl-bp\Winyl\src\sqlite3\sqlite3\src\`:
   - `sqlite3.c`
   - `sqlite3.h`
   - `sqlite3ext.h`

### 2. pugixml (REQUIRED - 5 minutes)
**URL**: https://pugixml.org/  
**File**: `pugixml-1.13.tar.gz` (or latest)  
**Size**: ~400 KB  

**Steps:**
1. Download latest release
2. Extract to temporary folder
3. Copy these files to `C:\Users\Fede\REPOS\winyl-bp\Winyl\src\pugixml\`:
   - `pugixml.hpp`
   - `pugixml.cpp`
   - `pugiconfig.hpp`

**IMPORTANT**: After copying, edit `pugiconfig.hpp` and uncomment these lines:
```cpp
#define PUGIXML_NO_XPATH
#define PUGIXML_NO_STL
#define PUGIXML_NO_EXCEPTIONS
#define PUGIXML_HEADER_ONLY
#define PUGIXML_HAS_LONG_LONG
```

### 3. Try First Build (IMMEDIATE)
After getting SQLite3 and pugixml:

1. Open `C:\Users\Fede\REPOS\winyl-bp\Winyl.sln` in Visual Studio
2. Try to build (even though it will fail)
3. Document what errors you get

This will tell us exactly what other dependencies are blocking us.

## COMPLEX DEPENDENCIES (Next Phase)

### 4. TagLib (COMPLEX - 2-3 hours)
**URL**: https://github.com/taglib/taglib  
**Requirements**: CMake, manual patching  

**Build Process** (detailed in BUILD.md):
1. Clone repository: `git clone https://github.com/taglib/taglib.git`
2. Install CMake if not available
3. Configure with CMake GUI: Enable `ENABLE_STATIC_RUNTIME` only
4. Apply manual patch to `taglib/mpeg/id3v2/id3v2tag.cpp`
5. Change Platform Toolset to `v141_xp` in Visual Studio
6. Build only the `tag` project
7. Copy headers and libraries to correct locations

### 5. BASS Audio Library (CRITICAL DECISION NEEDED)
**URL**: http://www.un4seen.com/  
**Issue**: Commercial license, unclear if compatible with open source  

**Required Downloads** (if licensing allows):
- bass24.zip
- bassmix24.zip  
- bass_fx24.zip
- basswasapi24.zip
- bassasio13.zip
- bass_aac24.zip
- bass_ape24.zip
- bass_mpc24.zip
- bass_spx24.zip
- bass_tta24.zip
- bassalac24.zip
- bassflac24.zip
- bassopus24.zip
- basswm24.zip
- basswv24.zip

**Licensing Research Needed:**
- Check if BASS allows open source usage
- Evaluate cost of commercial license
- Research alternative audio libraries

### 6. zlib (OPTIONAL - for PackSkin utility)
**URL**: https://zlib.net/  
**File**: Latest source with Visual Studio projects  
**Note**: Only needed if you want to build PackSkin utility  

## BUILD VALIDATION STEPS

After each dependency:
1. Try to build the project
2. Note which errors disappear
3. Note which new errors appear
4. Document the progress

## EXPECTED PROGRESSION

**With SQLite3 + pugixml**: ~50% of errors should resolve  
**With TagLib added**: ~80% of errors should resolve  
**With BASS added**: Project should build successfully  
**With zlib added**: PackSkin utility should build  

## IMMEDIATE PRIORITY ORDER

1. **SQLite3** - 5 minutes, required
2. **pugixml** - 5 minutes, required  
3. **First build attempt** - See what's really missing
4. **TagLib** - 2-3 hours, complex but doable
5. **BASS licensing research** - Could block entire project

## FALLBACK PLAN

If BASS licensing is prohibitive:
- Document current audio engine thoroughly
- Research alternative audio libraries
- Consider rewriting audio engine with open source components
- This would be a major architectural change

---

**NEXT ACTION**: Download SQLite3 and pugixml, then try building!