/*  This file is part of Winyl Player source code.
    Copyright (C) 2008-2018, Alex Kras. <winylplayer@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// stdafx.cpp : source file that includes just the standard includes
// Winyl.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// Winyl project settings. Changes from Win32 (not empty) project default settings:
// Debug:
// C/C++->Code Generation->Runtime Library->/MTd
// Linker->Input->Ignore Specific Default Libraries->LIBCMT
// Release:
// C/C++->Code Generation->Runtime Library->/MT
// Linker->Debugging->Generate Debug Info->No
//
// Reminder: How to set Precompiled Header in VS
// C/C++->Precompiled Headers->Precompiled Header->Use (/Yu)
// C/C++->Precompiled Headers->Precompiled Header File->stdafx.h
// And for stdafx.cpp file properties set C/C++->Precompiled Header->Create (/Yc)

// Just link all libs here, I hate to edit project settings in VS

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Winhttp.lib")

#ifndef _WIN64
#	pragma comment(lib, "src/bass/bass.lib")
#	pragma comment(lib, "src/bass/bass_fx.lib")
#	pragma comment(lib, "src/bass/bassmix.lib")
#	pragma comment(lib, "src/bass/basswasapi.lib")
#	pragma comment(lib, "src/bass/bassasio.lib")
#else // _WIN64
#	pragma comment(lib, "src/bass/x64/bass.lib")
#	pragma comment(lib, "src/bass/x64/bass_fx.lib")
#	pragma comment(lib, "src/bass/x64/bassmix.lib")
#	pragma comment(lib, "src/bass/x64/basswasapi.lib")
#	pragma comment(lib, "src/bass/x64/bassasio.lib")
#endif

#ifndef _WIN64
#	ifndef NDEBUG
#		pragma comment(lib, "src/sqlite3/sqlite3/Debug/sqlite3.lib")
#	else
#		pragma comment(lib, "src/sqlite3/sqlite3/Release/sqlite3.lib")
#	endif
#else // _WIN64
#	ifndef NDEBUG
#		pragma comment(lib, "src/sqlite3/sqlite3/x64/Debug/sqlite3.lib")
#	else
#		pragma comment(lib, "src/sqlite3/sqlite3/x64/Release/sqlite3.lib")
#	endif
#endif

// TagLib is compiled as source files, not as a separate library
//#ifndef _WIN64
//#	ifndef NDEBUG
//#		pragma comment(lib, "src/taglib/Debug/tag.lib")
//#	else
//#		pragma comment(lib, "src/taglib/Release/tag.lib")
//#	endif
//#else // _WIN64
//#	ifndef NDEBUG
//#		pragma comment(lib, "src/taglib/x64/Debug/tag.lib")
//#	else
//#		pragma comment(lib, "src/taglib/x64/Release/tag.lib")
//#	endif
//#endif




