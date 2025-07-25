/*  Debug Macros for Winyl Player
    Centralized debug logging system for development builds
    
    Usage:
    - Automatic in _DEBUG builds with WINYL_ENABLE_DEBUG_LOGGING defined
    - Zero overhead in Release builds (all macros compile to nothing)
    - Opt-in even for Debug builds (must explicitly enable)
    
    To enable debug logging:
    1. Debug builds: #define WINYL_ENABLE_DEBUG_LOGGING before including this file
    2. Or add WINYL_ENABLE_DEBUG_LOGGING to preprocessor definitions
    3. For specific systems: #define WINYL_WASAPI_DEBUG or WINYL_EQ_DEBUG
*/

#pragma once

// Only enable debug logging if explicitly requested in debug builds
#if defined(_DEBUG) && defined(WINYL_ENABLE_DEBUG_LOGGING)
    #include <windows.h>
    #include <stdio.h>

    // Basic debug logging
    #define DEBUG_LOG(msg) ::OutputDebugStringW(L##msg L"\n")
    
    // Formatted debug logging
    #define DEBUG_LOGF(fmt, ...) do { \
        wchar_t debugBuf[512]; \
        swprintf_s(debugBuf, L##fmt L"\n", __VA_ARGS__); \
        ::OutputDebugStringW(debugBuf); \
    } while(0)

    // Function entry/exit logging
    #define DEBUG_FUNC_ENTRY(func) DEBUG_LOG(#func " - ENTRY")
    #define DEBUG_FUNC_SUCCESS(func) DEBUG_LOG(#func " - SUCCESS")
    #define DEBUG_FUNC_FAILED(func) DEBUG_LOG(#func " - FAILED")

    // Pointer validation logging
    #define DEBUG_PTR_CHECK(ptr, name) do { \
        if (ptr == nullptr) { \
            DEBUG_LOGF("%s - WARNING: %s is nullptr", __FUNCTION__, name); \
        } \
    } while(0)

    // WASAPI specific logging (detailed)
    #ifdef WINYL_WASAPI_DEBUG
        #define WASAPI_DEBUG_LOG(msg) DEBUG_LOG("WASAPI: " msg)
        #define WASAPI_DEBUG_LOGF(fmt, ...) DEBUG_LOGF("WASAPI: " fmt, __VA_ARGS__)
    #else
        #define WASAPI_DEBUG_LOG(msg)
        #define WASAPI_DEBUG_LOGF(fmt, ...)
    #endif

    // Equalizer specific logging (detailed)
    #ifdef WINYL_EQ_DEBUG
        #define EQ_DEBUG_LOG(msg) DEBUG_LOG("EQ: " msg)
        #define EQ_DEBUG_LOGF(fmt, ...) DEBUG_LOGF("EQ: " fmt, __VA_ARGS__)
    #else
        #define EQ_DEBUG_LOG(msg)
        #define EQ_DEBUG_LOGF(fmt, ...)
    #endif

#else
    // Debug logging disabled (Release builds or Debug builds without WINYL_ENABLE_DEBUG_LOGGING)
    // All macros compile to nothing - zero performance overhead
    #define DEBUG_LOG(msg)
    #define DEBUG_LOGF(fmt, ...)
    #define DEBUG_FUNC_ENTRY(func)
    #define DEBUG_FUNC_SUCCESS(func)
    #define DEBUG_FUNC_FAILED(func)
    #define DEBUG_PTR_CHECK(ptr, name)
    #define WASAPI_DEBUG_LOG(msg)
    #define WASAPI_DEBUG_LOGF(fmt, ...)
    #define EQ_DEBUG_LOG(msg)
    #define EQ_DEBUG_LOGF(fmt, ...)
#endif

/*
    Example usage in code:
    
    // Basic logging
    DEBUG_LOG("Audio system initialized");
    DEBUG_LOGF("Sample rate: %d Hz", sampleRate);
    
    // Function tracing
    DEBUG_FUNC_ENTRY(InitializeAudio);
    if (success) DEBUG_FUNC_SUCCESS(InitializeAudio);
    else DEBUG_FUNC_FAILED(InitializeAudio);
    
    // Pointer validation
    DEBUG_PTR_CHECK(audioDevice, "audioDevice");
    
    // System-specific logging (requires WINYL_WASAPI_DEBUG defined)
    WASAPI_DEBUG_LOG("Exclusive mode activated");
    WASAPI_DEBUG_LOGF("Buffer size: %d frames", bufferSize);
    
    // Equalizer logging (requires WINYL_EQ_DEBUG defined)
    EQ_DEBUG_LOG("10-band EQ parameters updated");
    EQ_DEBUG_LOGF("Band %d: %.1f dB", band, gain);
*/