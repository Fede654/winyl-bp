/*  Debug Macros for Winyl Player
    Centralized debug logging system for development builds
*/

#pragma once

#ifdef _DEBUG
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

#else
    // Release builds - no logging
    #define DEBUG_LOG(msg)
    #define DEBUG_LOGF(fmt, ...)
    #define DEBUG_FUNC_ENTRY(func)
    #define DEBUG_FUNC_SUCCESS(func)
    #define DEBUG_FUNC_FAILED(func)
    #define DEBUG_PTR_CHECK(ptr, name)
    #define WASAPI_DEBUG_LOG(msg)
    #define WASAPI_DEBUG_LOGF(fmt, ...)
#endif