# BASS Audio Library Architecture Reference

## Overview

This document provides a comprehensive technical reference for BASS audio library architecture, specifically focused on understanding stream types, effect application points, and driver-specific behaviors that affect equalizer implementation in Winyl Player.

## BASS Stream Types and Audio Flow

### 1. Stream Creation Modes

#### Playable Streams (Normal Mode)
```cpp
HSTREAM stream = BASS_StreamCreateFile(FALSE, filename, 0, 0, 0);
// Flags: 0x0 (default - playable stream)
```

**Characteristics:**
- BASS manages the entire audio pipeline internally
- Stream can be played directly with `BASS_ChannelPlay()`
- Full support for all effect types (built-in and BASS_FX)
- Audio flows: `File → BASS Decoder → BASS Mixer → Audio Driver`

#### Decoded Streams (BASS_STREAM_DECODE)
```cpp
HSTREAM stream = BASS_StreamCreateFile(FALSE, filename, 0, 0, BASS_STREAM_DECODE);
// Flags: 0x200000 (BASS_STREAM_DECODE)
```

**Characteristics:**
- BASS only decodes audio data, doesn't play it
- Stream cannot be played directly with `BASS_ChannelPlay()`
- Audio data must be manually retrieved and fed to external output
- **Limited effect support** - some effects don't work
- Audio flows: `File → BASS Decoder → Manual Buffer Management → External Driver`

### 2. Driver-Specific Stream Usage

#### DirectSound Driver (bassDriver = 0)
```
Audio File → BASS Stream (Playable) → BASS Mixer → DirectSound API
           ↑ All effects work here ✅
```

**Implementation:**
- Uses playable streams
- BASS handles audio output internally
- Effects applied to source stream (`streamPlay`)
- All BASS_FX and built-in effects supported

#### WASAPI Driver (bassDriver = 1)
```
Audio File → BASS Stream (DECODED) → BASS Mixer → Manual Buffer → WASAPI API
           ↑ Some effects fail here ❌              ↑ Effects should work here ✅
```

**Implementation:**
- Uses decoded streams due to WASAPI requirements
- Manual buffer management required
- Effects on decoded stream may not work
- Effects on mixer stream should work

#### ASIO Driver (bassDriver = 2)
```
Audio File → BASS Stream (DECODED) → BASS Mixer → Manual Buffer → ASIO Driver
           ↑ Some effects fail here ❌              ↑ Effects should work here ✅
```

**Implementation:**
- Uses decoded streams due to ASIO requirements
- Manual buffer management required
- Similar limitations to WASAPI

## Effect Application Points

### Stream Hierarchy in Winyl

```
streamFile    → Raw audio file stream (source)
streamPlay    → File stream or decoded stream
streamMixer   → BASS mixer stream (combines multiple sources)
```

### Effect Target Selection

#### Rule 1: Driver-Based Selection
```cpp
if (bassDriver == 0) {
    // DirectSound: Apply to source stream
    target = streamPlay;
} else if (bassDriver == 1 || bassDriver == 2) {
    // WASAPI/ASIO: Apply to mixer stream
    target = streamMixer;
}
```

#### Rule 2: Stream Type Validation
```cpp
BASS_CHANNELINFO ci;
if (BASS_ChannelGetInfo(target, &ci)) {
    if (ci.flags & BASS_STREAM_DECODE) {
        // Decoded stream - limited effect support
        // Use mixer stream instead
        target = streamMixer;
    }
}
```

## Effect Compatibility Matrix

### Built-in BASS Effects

| Effect Type | Playable Streams | Decoded Streams | Mixer Streams | Notes |
|-------------|------------------|-----------------|---------------|-------|
| BASS_FX_VOLUME | ✅ | ⚠️ | ✅ | Universal volume control |
| BASS_FX_DX8_PARAMEQ | ✅ | ⚠️ | ✅ | Parametric equalizer |
| BASS_FX_DX8_CHORUS | ✅ | ⚠️ | ✅ | Chorus effect |
| BASS_FX_DX8_REVERB | ✅ | ⚠️ | ✅ | Reverb effect |
| DSP Callbacks | ✅ | ✅ | ✅ | Manual processing |

### BASS_FX Library Effects

| Effect Type | Playable Streams | Decoded Streams | Mixer Streams | Notes |
|-------------|------------------|-----------------|---------------|-------|
| BASS_FX_BFX_VOLUME | ✅ | ❌ | ⚠️ | BASS_FX library limitation |
| BASS_FX_BFX_PEAKEQ | ✅ | ❌ | ⚠️ | Multi-band EQ from BASS_FX |
| BASS_FX_BFX_REVERB | ✅ | ❌ | ⚠️ | BASS_FX reverb |

**Legend:**
- ✅ Fully supported
- ⚠️ May work depending on implementation
- ❌ Not supported

## Technical Constraints

### BASS_FX Library Limitations

1. **Decoded Stream Incompatibility**
   - BASS_FX effects require BASS to manage playback
   - Decoded streams bypass BASS playback mechanism
   - Error: `BASS_ERROR_FORMAT (19)` when applied to decoded streams

2. **Dependency Requirements**
   - Requires `bass_fx.lib` linking
   - Requires `bass_fx.dll` deployment
   - Version compatibility with BASS library

### Built-in BASS Effects Advantages

1. **Universal Compatibility**
   - Work with all stream types
   - No external dependencies
   - Part of core BASS library

2. **Performance Benefits**
   - Lower overhead
   - Direct integration
   - No additional DLL loading

## Recommended Architecture

### Equalizer Implementation Strategy

#### Option 1: Built-in BASS Effects (Recommended)
```cpp
// Apply to appropriate stream based on driver
HSTREAM target = (bassDriver == 0) ? streamPlay : streamMixer;

// Create 10 separate parametric EQ bands
for (int i = 0; i < 10; i++) {
    fxEqualizer[i] = BASS_ChannelSetFX(target, BASS_FX_DX8_PARAMEQ, i + 2);
    
    BASS_DX8_PARAMEQ eq;
    eq.fCenter = frequencies[i];     // Band frequency
    eq.fGain = gains[i];            // Band gain
    eq.fBandwidth = 1.0f;           // Standard bandwidth
    
    BASS_FXSetParameters(fxEqualizer[i], &eq);
}
```

#### Option 2: DSP Callbacks (Maximum Compatibility)
```cpp
// Manual DSP processing - works with all streams
HDSP dsp = BASS_ChannelSetDSP(target, EqualizerDSP, this, 0);

DWORD CALLBACK EqualizerDSP(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user) {
    // Manual equalizer processing
    // Apply frequency filtering to audio buffer
    return length;
}
```

### Stream Target Selection Logic

```cpp
HSTREAM GetEffectTarget() {
    if (bassDriver == 0) {
        // DirectSound: Use source stream (playable)
        return streamPlay;
    } else {
        // WASAPI/ASIO: Use mixer stream to avoid decoded stream issues
        if (streamMixer != NULL) {
            return streamMixer;
        } else {
            // Fallback to source stream
            return streamPlay;
        }
    }
}
```

## Debug and Validation

### Stream Information Extraction
```cpp
void AnalyzeStream(HSTREAM stream, const char* name) {
    BASS_CHANNELINFO ci;
    if (BASS_ChannelGetInfo(stream, &ci)) {
        printf("%s: freq=%d, chans=%d, flags=0x%x, ctype=0x%x\n", 
               name, ci.freq, ci.chans, ci.flags, ci.ctype);
        
        if (ci.flags & BASS_STREAM_DECODE) {
            printf("  -> DECODED stream (limited effect support)\n");
        } else {
            printf("  -> PLAYABLE stream (full effect support)\n");
        }
    }
}
```

### Effect Validation
```cpp
bool ValidateEffect(HFX effect, const char* name) {
    if (effect == 0) {
        int error = BASS_ErrorGetCode();
        printf("ERROR: %s failed with BASS error %d\n", name, error);
        return false;
    } else {
        printf("SUCCESS: %s created with handle %d\n", name, effect);
        return true;
    }
}
```

## Frequency Band Design

### Standard 10-Band Equalizer
```cpp
const float frequencies[10] = {
    80.0f,    // Sub-bass (adjusted for BASS_DX8_PARAMEQ limits)
    100.0f,   // Bass (adjusted for compatibility)
    125.0f,   // Bass
    250.0f,   // Low-mid
    500.0f,   // Mid
    1000.0f,  // Mid
    2000.0f,  // High-mid
    4000.0f,  // Presence
    8000.0f,  // Brilliance
    16000.0f  // Air
};
```

### Frequency Constraints
- **BASS_DX8_PARAMEQ minimum**: ~80 Hz (varies by sample rate)
- **BASS_DX8_PARAMEQ maximum**: ~Nyquist frequency (samplerate/2)
- **Bandwidth range**: 0.1 to 2.0 octaves
- **Gain range**: Typically -15 to +15 dB

## Troubleshooting Guide

### Common Issues

1. **Effects not audible**
   - Check stream target (use mixer for WASAPI/ASIO)
   - Verify effect creation success
   - Confirm parameter application

2. **BASS_ERROR_FORMAT (19)**
   - Effect incompatible with stream type
   - Switch to built-in effects
   - Use mixer stream instead of decoded stream

3. **BASS_ERROR_ILLPARAM (20)**
   - Invalid frequency or bandwidth values
   - Check frequency bounds for effect type
   - Adjust bandwidth for low frequencies

### Debug Checklist

1. ✅ Verify stream type and flags
2. ✅ Confirm appropriate target stream selection
3. ✅ Validate effect creation return values
4. ✅ Check BASS error codes on failures
5. ✅ Test parameter application success
6. ✅ Monitor real-time parameter changes

---

**Conclusion**: The key to successful equalizer implementation in BASS is understanding the fundamental differences between playable and decoded streams, and selecting the appropriate target stream based on the audio driver being used. Built-in BASS effects provide the best compatibility across all driver types.