# Winyl - Professional Audio Player

Remember when music collections were crafted with devotion? When cue logs were shared like sacred texts, and every pristine copy was too precious to let slip into anonymous digital oblivion. We used to listen to complete albums, meticulously organised, metadata corrected down to every accent mark - obsessive, certainly, but in those days afternoons had room for everything: every record, all of it could fit into a single afternoon. Hard to believe now. A gift for solitary fanatics. Unfold those FLAC collections and find again the lost taste of detail in music.

Thanks to the original author of this meticulous piece of art - a player with the rare distinction of delivering bit-perfect FLAC reproduction on native Windows, one of the few free applications to implement full WASAPI compatibility and bring it to end users. A strange gem, one we loved for a very long time.

## 📊 Modernization Status

### ✅ **Milestone 1: Build System Modernization** - COMPLETED
- **Toolchain Migration**: VS2017 XP toolset → VS2019 v142 + Windows SDK 10.0
- **Architecture**: Full x64 native compilation
- **Build Status**: 0 errors, 0 warnings
- **Output**: Working 9.3MB Winyl.exe executable

### ✅ **Milestone 2: Complete TagLib Integration** - COMPLETED  
- **Implementation**: 54 TagLib source files integrated with static linking
- **Format Support**: MP3, FLAC, OGG, MP4, APE, MPC, WavPack, TrueAudio, ASF, AIFF
- **Metadata**: Full ID3v1/v2, Vorbis Comment, APE tags, MP4 atoms
- **Build Integration**: Custom include paths, precompiled header management

### ✅ **Milestone 3: Professional Audio Libraries** - COMPLETED
- **BASS Audio Engine**: Real x64 libraries (bass.lib, bassmix.lib, basswasapi.lib, bassasio.lib)
- **Audio Output**: WASAPI, ASIO support for professional audio interfaces  
- **Bit-perfect Playback**: Direct audio path without Windows audio processing
- **Library Source**: Official un4seen.com distributions

### ✅ **Milestone 4: Modern Development Environment** - COMPLETED
- **Automated Build System**: Smart MSBuild detection across VS2019 installations
- **Build Options**: Force rebuild (--force, clean, rebuild), incremental builds
- **Build Intelligence**: File timestamp tracking, rebuild detection, comprehensive logging
- **Cross-platform**: CMD, PowerShell, MINGW64 compatibility
- **Developer Experience**: One-command build with detailed status reporting

## 🔧 Technical Architecture

### Audio Engine Stack
```
Application Layer
    ↓
BASS Audio Library (x64)
    ↓  
WASAPI/ASIO Drivers
    ↓
Audio Hardware
```

### Metadata Processing
```
Audio Files → TagLib (Static) → Unified Tag Interface → Database (SQLite3)
```

### Build System
- **MSBuild 16.x** with v142 toolset
- **Static Linking**: TagLib, SQLite3, pugixml 
- **Dynamic Linking**: BASS audio libraries
- **Platform**: x64 native (Windows 10+)

## 🚀 Quick Start

```bash
# Clone and build
git clone https://github.com/Fede654/winyl-bp.git
cd winyl-bp
build.bat

# Force clean rebuild
build.bat --force

# Release build
build.bat Release
```

**Generated executable**: `Winyl\x64\Debug\Winyl.exe` (9.3MB)

## 📈 Development Roadmap

### Phase 1: Core Functionality Testing
- [ ] Audio playback verification across formats
- [ ] Metadata reading/writing validation  
- [ ] WASAPI/ASIO output testing
- [ ] Library management functionality

### Phase 2: Audio Enhancement
- [ ] BASS_FX integration for DSP effects
- [ ] Advanced equalizer restoration
- [ ] Audio analysis and monitoring tools
- [ ] High-resolution audio format optimization

### Phase 3: Modern User Experience  
- [ ] UI responsiveness improvements
- [ ] Modern Windows 10/11 integration
- [ ] Performance profiling and optimization
- [ ] Memory usage analysis

## 🛠️ Build Requirements

- **Visual Studio 2019** Build Tools or Community/Professional/Enterprise
- **Windows SDK 10.0** (any recent version)
- **Git** for source control
- **MSBuild 16.x** (included with VS2019)

## 🎯 Project Philosophy

**Professional Audio First**: Every change prioritizes audio fidelity and professional audio workflow compatibility. Modern development practices are adopted only when they preserve or enhance audio quality.

## 📄 License

GPL v3+ (maintains original Winyl licensing)