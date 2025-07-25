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

#include "stdafx.h"
#include "resource.h"
#include "LibAudio.h"
#include "WinylWnd.h"
#include "FileSystem.h"
//#include <regex>

// Debug logging system - opt-in for development
// Uncomment next line to enable comprehensive audio system logging:
// #define WINYL_ENABLE_DEBUG_LOGGING

// Uncomment for specific system logging (requires WINYL_ENABLE_DEBUG_LOGGING):
// #define WINYL_WASAPI_DEBUG  
// #define WINYL_EQ_DEBUG

#include "DebugMacros.h"

// In short, this class is a mess. This class is a mix of BASS C api and C++
// every change should be as careful as possible or it can just break things.

// BASS uses api similar to WinAPI and sometimes it's a bit hard to notice
// if something is wrong, so just use this verify define everywhere and we are fine.
#ifdef _DEBUG
#define verify(f)    assert(f)
#else
#define verify(f)    ((void)(f))
#endif

int LibAudio::Buffer::DirectSound   = 1000; // Buffer size for DirectSound in ms
double LibAudio::Buffer::WasapiAsio = 1.0; // Reading buffer size for WASAPI/ASIO in sec
int LibAudio::Buffer::BassRead      = 1024 * 256; // BASS reading buffer in bytes, default is 65536

int LibAudio::FadeTime::Pause = 500;
int LibAudio::FadeTime::Stop  = 500;
int LibAudio::FadeTime::Pos   = 500;
int LibAudio::FadeTime::Mute  = 100;

// Original Winyl 10-band equalizer frequencies (Hz) - ISO standard octave-based spacing
// Technical approach: 31.5, 63, 125, 250, 500, 1k, 2k, 4k, 8k, 16k Hz
const float LibAudio::eqFrequencies[10] = {
	31.5f,   // Band 0: 31.5 Hz (Original octave-based spacing)
	63.0f,   // Band 1: 63 Hz
	125.0f,  // Band 2: 125 Hz  
	250.0f,  // Band 3: 250 Hz
	500.0f,  // Band 4: 500 Hz
	1000.0f, // Band 5: 1 kHz
	2000.0f, // Band 6: 2 kHz
	4000.0f, // Band 7: 4 kHz
	8000.0f, // Band 8: 8 kHz
	16000.0f // Band 9: 16 kHz
};

LibAudio::LibAudio()
{
	BASS_SetConfig(BASS_CONFIG_UNICODE, TRUE);
	BASS_ASIO_SetUnicode(TRUE);
	
	// Initialize equalizer effect handles to NULL
	InitializeEqHandles();
}

LibAudio::~LibAudio()
{
	SaveEqualizer();

	if (isMediaPlay && !isMediaPause)
	{ // Fade out the volume when close the program
		Stop();
	//	if (bassDriver > 0) // Hack
	//	{
	//		isMediaPlay = true;
	//		isMediaPause = false;
	//	}
		Sleep(500);
	}

	Free();
}


bool LibAudio::Init(WinylWnd* wnd, int driver, int device, bool isBit32, bool isSoftMix, bool isLoadEq)
{
	// ALWAYS output this message - regardless of Debug/Release
	::OutputDebugStringW(L"*** WINYL AUDIO SYSTEM INITIALIZING - DebugView should see this! ***\n");
	
#ifdef _DEBUG
	::OutputDebugStringW(L"Build mode: DEBUG\n");
#else
	::OutputDebugStringW(L"Build mode: RELEASE\n");
#endif

#ifdef WINYL_EQ_DEBUG
	::OutputDebugStringW(L"WINYL_EQ_DEBUG is defined\n");
#else
	::OutputDebugStringW(L"WINYL_EQ_DEBUG is NOT defined\n");
#endif

	DEBUG_LOG("========================================");
	DEBUG_LOG("LibAudio::Init - STARTING AUDIO SYSTEM");
	DEBUG_LOG("========================================");
	DEBUG_LOGF("Parameters: driver=%d, device=%d, 32bit=%d, softmix=%d, loadEQ=%d", 
		driver, device, isBit32, isSoftMix, isLoadEq);
	
	const wchar_t* driverNames[] = {L"DirectSound", L"WASAPI", L"ASIO", L"Unknown"};
	int driverIndex = (driver >= 0 && driver <= 2) ? driver : 3;
	DEBUG_LOGF("Audio Driver: %s (%d)", driverNames[driverIndex], driver);
	DEBUG_LOGF("Audio Device: %d", device);
	
	if (wnd) {
		wndWinyl = wnd;
		DEBUG_LOG("Window handle assigned for audio callbacks");
	} else {
		DEBUG_LOG("WARNING: No window handle provided");
	}

	bassDriver = driver;
	bassDevice = device;

	BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, TRUE);
	//BASS_SetConfig(29, 1); // BASS_CONFIG_NOTIMERES disable timeBeginPeriod(1)

	if (bassDriver == 0) // DirectSound
	{
		DEBUG_LOG("Initializing DirectSound driver...");
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 100);
		DEBUG_LOG("DirectSound update period set to 100ms");

		if (bassDevice == 0) // Audio device should not be "No Sound"
		{
			DEBUG_LOG("ERROR: DirectSound device 0 (No Sound) is not valid");
			return false;
		}

		// There was a strange bug:
		// If in Win7 pin Winyl to the taskbar then BASS_Init often returns BASS_ERROR_UNKNOWN
		// To fix this always set win parameter of BASS_Init because quote from BASS forum:
		// The only time BASS_Init can result in BASS_ERROR_UNKNOWN is when
		// it fails to set the DirectSound "cooperative level" or create the primary buffer.
		// Setting the cooperative level is what requires the window handle, hence the suspicion that's what's wrong
		HWND wndHandle = (wndWinyl && wndWinyl->IsWnd()) ? wndWinyl->Wnd() : NULL;
		DEBUG_LOGF("Attempting BASS_Init with device=%d, window=0x%p", bassDevice, wndHandle);
		if (!BASS_Init(bassDevice, 44100, BASS_DEVICE_DSOUND, wndHandle, NULL))
		{
			int error = BASS_ErrorGetCode();
			DEBUG_LOGF("ERROR: BASS_Init failed with error code %d", error);
			return false;
		}
		DEBUG_LOG("DirectSound BASS_Init successful");

		dwSampleEx = 0;

		if (isBit32) // 32 bit audio
		{
			DEBUG_LOG("Testing 32-bit float audio support...");
			// Check that the sound card supports 32 bit
			HSTREAM streamFloat = BASS_StreamCreate(44100, 2, BASS_SAMPLE_FLOAT, NULL, 0);
			if (streamFloat) // If yes use it
			{
				dwSampleEx |= BASS_SAMPLE_FLOAT;
				BASS_StreamFree(streamFloat);
				DEBUG_LOG("32-bit float audio support: ENABLED");
			}
			else
			{
				DEBUG_LOG("32-bit float audio support: NOT AVAILABLE");
			}
		}

		if (isSoftMix) // Software audio mixing
		{
			dwSampleEx |= BASS_SAMPLE_SOFTWARE;
			DEBUG_LOG("Software mixing: ENABLED");
		}
	}
	else if (bassDriver == 1) // WASAPI
	{
		DEBUG_LOG("Initializing WASAPI driver...");
		if (!futureWin->IsVistaOrLater())
		{
			DEBUG_LOG("ERROR: WASAPI requires Vista or later");
			return false;
		}

		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
		DEBUG_LOG("WASAPI update period set to 0 (event-driven)");
		
		DEBUG_LOG("Initializing BASS with dummy device for WASAPI");
		BASS_Init(0, 44100, 0, 0, NULL);
		DEBUG_LOG("BASS dummy initialization complete");
	}
	else if (bassDriver == 2) // ASIO
	{
		DEBUG_LOG("Initializing ASIO driver...");
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
		DEBUG_LOG("ASIO update period set to 0 (event-driven)");
		
		DEBUG_LOG("Initializing BASS with dummy device for ASIO");
		BASS_Init(0, 44100, 0, 0, NULL);
		DEBUG_LOG("BASS dummy initialization complete");

//		if (!BASS_ASIO_Init(bassDevice, 0))
//			return false;
	}

	if (bassDriver == 1 || bassDriver == 2)
	{
		DEBUG_LOG("Setting up high-performance audio buffer thread...");
		stopThreadBuffer = false;
		eventBuffer.reset(new Threading::Event());
		eventPause.reset(new Threading::Event(false));
		mutexBuffer.reset(new Threading::Mutex());
		threadBuffer.reset(new Threading::Thread());
		threadBuffer->Start(std::bind(&LibAudio::RunThreadBuffer, this), Threading::Thread::Priority::TimeCritical);
		DEBUG_LOG("High-performance audio buffer thread started");
	}

	
	DEBUG_LOG("Configuring BASS audio settings...");
	BASS_SetConfig(BASS_CONFIG_BUFFER, Buffer::DirectSound); // DirectSound buffer size (1000 ms should be enought to preload track)
	DEBUG_LOGF("DirectSound buffer: %d ms", Buffer::DirectSound);
	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
	BASS_SetConfig(BASS_CONFIG_ASYNCFILE_BUFFER, Buffer::BassRead); // default is 65536
	DEBUG_LOGF("Async file buffer: %d bytes", Buffer::BassRead);
	BASS_SetConfig(BASS_CONFIG_SRC, 2); // for WASAPI mixer if freq is not supported, default is 1
	DEBUG_LOG("Sample rate conversion: High quality (2)");
	//if (!isBassFX) BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);

	if (wnd) // if wnd == nullptr then ReInit and we don't need to load plugins again
	{
#ifndef NDEBUG
#ifndef _WIN64
		programPath.append(L"x86\\");
#else
		programPath.append(L"x64\\");
#endif // _WIN64
#endif // NDEBUG

		// Load BASS format plugins (WAV, MP3, OGG, AIFF included in BASS)
		// TODO: Re-enable FLAC plugin when verified working:
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bassflac.dll").c_str(), BASS_UNICODE));
		// TODO: Download and add remaining plugins:
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bass_ape.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\basswma.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bass_aac.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bassalac.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bass_spx.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bassopus.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\basswv.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bass_mpc.dll").c_str(), BASS_UNICODE));
		// verify(BASS_PluginLoad((const char*)(programPath + L"Bass\\bass_tta.dll").c_str(), BASS_UNICODE));
	}

	// Test BASS_FX library availability
	DEBUG_LOG("Checking BASS_FX availability...");

	if (isLoadEq)
	{
		DEBUG_LOG("Loading equalizer settings...");
		LoadEqualizer();
		DEBUG_LOG("Equalizer loading complete");
	}
	else
	{
		DEBUG_LOG("Equalizer loading skipped (isLoadEq=false)");
	}

	DEBUG_LOG("========================================");
	DEBUG_LOG("LibAudio::Init - AUDIO SYSTEM READY");
	DEBUG_LOG("========================================");
	return true;
}

void LibAudio::SetUserAgent(const std::string& agent)
{
	if (!agent.empty())
		BASS_SetConfigPtr(BASS_CONFIG_NET_AGENT, agent.c_str());
}

void LibAudio::SetNoVolumeEffect(bool volume, bool effect, bool reset)
{
	if (bassDriver > 0)
	{
		bassNoVolume = volume;
		bassNoEffect = effect;
		if (bassNoVolume)
			realVolume = 1.0f;
	}
	else if (reset)
	{
		bassNoVolume = false;
		bassNoEffect = false;
		realVolume = soundVolume;
	}
}

void LibAudio::SetProxy(int proxy, const std::wstring& host, const std::wstring& port, const std::wstring& login, const std::wstring& pass)
{
	if (proxy == 0)
	{
		BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, (void*)NULL);
	}
	else if (proxy == 1)
	{
		proxyString = UTF::UTF8S(host);
		proxyString.push_back(':');
		proxyString += UTF::UTF8S(port);

		BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, &proxyString[0]);
	}
	else if (proxy == 2)
	{
		proxyString = UTF::UTF8S(login);
		proxyString.push_back(':');
		proxyString += UTF::UTF8S(pass);
		proxyString.push_back('@');
		proxyString += UTF::UTF8S(host);
		proxyString.push_back(':');
		proxyString += UTF::UTF8S(port);

		BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, &proxyString[0]);
	}
}

void LibAudio::Free()
{
	if (threadBuffer)
	{
		stopThreadBuffer = true;
		eventBuffer->Set();
		eventPause->Set();
		if (threadBuffer->IsJoinable())
			threadBuffer->Join();
		threadBuffer.reset();

		for (std::size_t i = 0; i < bufferStreams.size(); ++i)
		{
			// Free buffer stream
			verify(BASS_StreamFree(bufferStreams[i]->streamFile));
			delete[] bufferStreams[i]->buffer;
			delete bufferStreams[i];
		}
		bufferStreams.clear();
	}

	eventBuffer.reset();
	mutexBuffer.reset();

	////////

	if (bassDriver == 1)
		BASS_WASAPI_Free();
	else if (bassDriver == 2)
		BASS_ASIO_Free();

	BASS_Free();

	if (threadRadio.IsJoinable())
	{
		threadRadio.TryJoin(30000);
	}
}

bool LibAudio::GetRadioTags(std::wstring& title, std::wstring& artist, std::wstring& album)
{
	if (streamPlay == NULL)
		return false;

	const char* tags = BASS_ChannelGetTags(streamPlay, BASS_TAG_META);

	if (tags == nullptr)
		return false;

	std::wstring text = UTF::IsUTF8(tags) ? UTF::UTF16(tags) : UTF::UTF16A(tags);

	if (text.empty())
		return false;

	// Example: "StreamTitle='Morning Show: BEFORE - Quixote (Album)';StreamUrl='';"

	std::size_t find1 = text.find(L"='");
	std::size_t find2 = text.find(L"';");
	if (find1 != std::string::npos && find2 != std::string::npos)
	{
		text = text.substr(find1 + 2, find2 - find1 - 2);
		if (!text.empty())
		{
			title = text;
			std::size_t find = title.rfind('-');
			if (find != std::string::npos)
			{
				artist = title.substr(0, find);
				title = title.substr(find + 1);
			}
		}
	}
/*
	std::wsmatch result;
	if (std::regex_match(text, result, std::wregex(L"^.*StreamTitle='(.*?)';.*$")))
	{
		text = result[1];
		if (std::regex_match(text, result, std::wregex(L"^(.*)-(.*)$")))
		{
			artist = result[1];
			title = result[2];

//			text = title;
//			if (std::regex_match(text, result, std::wregex(L"^(.*)\\((.*?)\\)$")))
//			{
//				title = result[1];
//				album = result[2];
//			}
		}
	}
*/
	StringEx::Trim(title);
	StringEx::Trim(artist);
	StringEx::Trim(album);

	return true;
}

bool LibAudio::GetRadioInfo(int& type, int& kbps, int& chans, int& freq, std::wstring& meta)
{
	if (!isMediaPlay || !isMediaRadio || streamPlay == NULL)
		return false;

	kbps = radioBitrate;

	BASS_CHANNELINFO info = {};
	BASS_ChannelGetInfo(streamPlay, &info);

	chans = info.chans;
	freq = info.freq;

	switch(info.ctype)
	{
		case BASS_CTYPE_STREAM_MP3: type = 1; break;
		case BASS_CTYPE_STREAM_OGG: type = 2; break;
		case 0x10300: type = 3; break; // BASS_CTYPE_STREAM_WMA
		case 0x10301: type = 1; break; // BASS_CTYPE_STREAM_WMA_MP3
		case 0x10b00: type = 4; break; // BASS_CTYPE_STREAM_AAC
		case 0x10b01: type = 5; break; // BASS_CTYPE_STREAM_MP4
		default: type = 0;
	}

	// WMA Bitrate (From BASS forum)
	if (kbps == 0 && (info.ctype == 0x10300 || info.ctype == 0x10301))
	{
		const char* wma = BASS_ChannelGetTags(streamPlay, 8);
		if (wma)
		{
			while (*wma)
			{
				if (!strncmp(wma, "Bitrate=", 8)) 
				{
					kbps = atoi(wma + 8) / 1000; // should be 1024 but 1000 more accurate, don't know why
					break;
				}
				wma += strlen(wma) + 1;
			}
		}
	}

	///////////////////////
	const char* tags = BASS_ChannelGetTags(streamPlay, BASS_TAG_META);
	std::wstring text = UTF::IsUTF8(tags) ? UTF::UTF16(tags) : UTF::UTF16A(tags);

	if (text.empty())
		return true;

	// Example: "StreamTitle='Morning Show: BEFORE - Quixote (Album)';StreamUrl='';"

	std::size_t find1 = text.find(L"='");
	std::size_t find2 = text.find(L"';");
	if (find1 != std::string::npos && find2 != std::string::npos)
	{
		text = text.substr(find1 + 2, find2 - find1 - 2);
		if (!text.empty())
		{
			meta = text;
		}
	}
/*
	std::wsmatch result;
	if (std::regex_match(text, result, std::wregex(L"^.*StreamTitle='(.*?)';.*$")))
	{
		meta = result[1];
	}
*/
	return true;
}

void LibAudio::SyncProcEnd(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (!libAudio->isMediaPlay)
		return;

	assert(libAudio->bassDriver == 0);

	// Previous or current file is part of cue need to reset syncs
	if ((libAudio->cuePreload || libAudio->cueThis) && libAudio->isPreloadRate)
		libAudio->ResetCueSync(libAudio->cuePreload, libAudio->timeSecondPreload);

	//libAudio->SyncProcEndImpl();
	::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSNEXT, 0, 0);
}

void LibAudio::SyncProcGaplessWA(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (!libAudio->isMediaPlay)
		return;

	if (libAudio->streamPreload && libAudio->isPreloadRate)
	{
		BASS_ChannelSetAttribute(libAudio->streamPreload, BASS_ATTRIB_VOL, libAudio->realVolume);

		BASS_Mixer_StreamAddChannel(channel, libAudio->streamPreload, BASS_MIXER_NORAMPIN|BASS_MIXER_BUFFER);
		BASS_ChannelSetPosition(channel, 0, BASS_POS_BYTE);
	}

	// Previous or current file is part of cue need to reset syncs
	if ((libAudio->cuePreload || libAudio->cueThis) && libAudio->isPreloadRate)
		libAudio->ResetCueSync(libAudio->cuePreload, libAudio->timeSecondPreload);

	if (libAudio->isMediaPause) // It fixes bug: pause at the end of the track with big fading
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSWASTOP, 0, 0);
	::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSNEXT, 0, 0);
}

void LibAudio::SyncProcGaplessDS(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (!libAudio->isMediaPlay)
		return;

	//::PostMessage(libAudio->hParent, UWM_BASSPRELOAD, 0, 0); // Don't do this
	libAudio->SyncProcPreloadImpl();

	if (libAudio->streamPreload && libAudio->isPreloadRate)
	{
		// Need to stop channel when changes from cue to file or end of cue mixed with new file
		// WASAPI and ASIO seems to work fine without this, use WASAPI buffer size to test
		if (libAudio->cueThis && CueFile::IsLenght(libAudio->cueThis))
		{
			BASS_ChannelStop(libAudio->streamPlay);
			// Without this the func calls twice when changes from cue to file (to reproduce move position to the end)
			verify(BASS_ChannelRemoveSync(libAudio->streamMixer, libAudio->syncEndMixCue));
			libAudio->syncEndMixCue = NULL;
		}

		BASS_Mixer_StreamAddChannel(channel, libAudio->streamPreload, BASS_MIXER_NORAMPIN|BASS_MIXER_BUFFER);
		BASS_ChannelSetPosition(channel, 0, BASS_POS_BYTE);
	}
}

void LibAudio::SyncProcGaplessCueWA(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	libAudio->nextCueTrack = true;
	libAudio->eventBuffer->Set();
}

void LibAudio::SyncProcPreloadImpl()
{
	assert(filePreload.empty() && streamPreload == NULL && cuePreload == 0);

	if (!isMediaPlay)
		return;
	if (!filePreload.empty() || streamPreload || cuePreload) // Already preloaded
		return;

	std::wstring file;

	//Sleep(3000);

	// Get the next track
	//::SendMessage(hParent, UWM_NEXTFILE, (LPARAM)&csFile, 0);
	file = wndWinyl->ChangeFile(cuePreload);

	if (!file.empty())
	{
		//Sleep(2000); // Test slow file load

		if (!cuePreload)
			streamPreload = OpenMediaFile(file, &byteLengthPreload, &timeSecondPreload);
		else // CUE
		{
			double offset = CueFile::GetOffset(cuePreload);

			// Next track from same CUE
			if (cueThis && file == fileThis && CueFile::IsNextCue(cueThis, cuePreload))
				cueOffsetPreload = BASS_ChannelSeconds2Bytes(streamPlay, offset);
			else // New CUE file
				streamPreload = OpenMediaFile(file, &byteLengthCue, &timeSecondCue, &offset, &cueOffsetPreload);

			if (CueFile::IsLenght(cuePreload))
			{
				timeSecondPreload = CueFile::GetLenght(cuePreload);
				byteLengthPreload = BASS_ChannelSeconds2Bytes(
					streamPreload ? streamPreload : streamPlay, timeSecondPreload);
			}
			else
			{
				timeSecondPreload = timeSecondCue - offset;
				byteLengthPreload = byteLengthCue - cueOffsetPreload;
			}

			// If next track from same CUE exit here
			if (!streamPreload)
			{
				isPreloadRate = true;
				return;
			}
		}

		filePreload = file;

		// If streamPreload == NULL then most probably it is URL, we will check it in SyncProcEndImpl
		if (streamPreload == NULL)
		{
			isPreloadRate = false;
			return;
		}

		BASS_CHANNELINFO ci;
		BASS_CHANNELINFO pi;
		BASS_ChannelGetInfo(streamPlay, &ci);
		BASS_ChannelGetInfo(streamPreload, &pi);

		if (pi.freq == ci.freq && pi.chans == ci.chans)
			isPreloadRate = true;
		else
			isPreloadRate = false;
	}
}

void LibAudio::SyncProcEndImpl()
{
	if (!isMediaPlay)
		return;

	if (!isMediaRadio) 	// If it wasn't radio (just stop if it was radio)
	{
		if (!filePreload.empty()) // If there is a next file
		{
			if (streamPreload) // File is preloaded
			{
				BASS_StreamFree(streamPlay);

				streamPlay = streamPreload;
				streamPreload = NULL;

				fileThis = filePreload;
				filePreload.clear();

				byteLength = byteLengthPreload;//BASS_ChannelGetLength(streamPlay, BASS_POS_BYTE);
				timeSecond = timeSecondPreload;//BASS_ChannelBytes2Seconds(streamPlay, byteLength);

				cueThis = cuePreload;
				cuePreload = 0;

				cueOffset = cueOffsetPreload;
				cueOffsetPreload = 0;

				posPlus = cueOffset;

				// Sample rates are different, need to restart the device with new sample rate
				if (!isPreloadRate)
				{
					if (bassDriver == 0)
						StartPlayDS(true, false, true);
					else if (bassDriver == 1)
						StartPlayWASAPI(true, false, true);
					else if (bassDriver == 2)
						StartPlayASIO(true, false, true);
				}
				else
					isPreloadRate = false;

				//::SendMessage(hParent, UWM_PLAYDRAW, 0, 0);
				wndWinyl->ChangeNode(false, false);

				return;
			}
			else // File is not preloaded
			{
				// Maybe it is URL
				if (PathEx::IsURL(filePreload))
				{
					PlayURL(filePreload);
					filePreload.clear();
					//::SendMessage(hParent, UWM_PLAYDRAW, 0, 1);
					wndWinyl->ChangeNode(false, true);
					return;
				}

				// Or maybe the file is not found then stop
				filePreload.clear();
			}
		}
		else if (cuePreload) // Next track from same CUE
		{
			cueThis = cuePreload;
			cuePreload = 0;

			cueOffset = cueOffsetPreload;
			cueOffsetPreload = 0;

			byteLength = byteLengthPreload;
			timeSecond = timeSecondPreload;

			wndWinyl->ChangeNode(false, false);

			return;
		}
	}
	
	// Stop

	if (bassDriver == 1)
	{
		BASS_WASAPI_Stop(TRUE);
		BASS_WASAPI_Free();
	}
	else if (bassDriver == 2)
	{
		BASS_ASIO_Stop();
		BASS_ASIO_Free();
	}

	ResetCueSync();

	if (streamMixer)
	{
		BASS_StreamFree(streamMixer);
		streamMixer = NULL;
	}
	if (streamPlay)
	{
		BASS_StreamFree(streamPlay);
		streamPlay = NULL;
	}

	isMediaPlay = false;
	isMediaPause = false;
	isMediaRadio = false;

	//::SendMessage(hParent, UWM_PLAYDRAW, 1, 0); // PostMessage
	wndWinyl->ChangeNode(true, false);
}

void LibAudio::PrepareOpen(bool needFade)
{
	ResetCueSync();

	if (bassDriver == 0) // DirectSound
	{	
		if (needFade && isMediaPlay && !isMediaPause && !bassNoEffect) // Fade out mixer
		{
			// Need to remove syncs here or the mixer that fade out can call them.
			// Below in WASAPI & ASIO it's not needed because a new mixer isn't created.
			verify(BASS_ChannelRemoveSync(streamMixer, syncEnd) == TRUE);
			verify(BASS_ChannelRemoveSync(streamMixer, syncEndMix) == TRUE);
			// Use BASS_STREAM_AUTOFREE without sync.
			assert(!(BASS_ChannelFlags(streamMixer, 0, 0) & BASS_STREAM_DECODE));
			verify(BASS_Mixer_ChannelFlags(streamPlay, BASS_STREAM_AUTOFREE, BASS_STREAM_AUTOFREE) != -1); // Important!
			verify(BASS_ChannelFlags(streamMixer, BASS_STREAM_AUTOFREE, BASS_STREAM_AUTOFREE) != -1); // Important!
			//BASS_ChannelSetSync(streamMixer, BASS_SYNC_MIXTIME|BASS_SYNC_SLIDE, 0, SyncFreeMixer, this);
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, -1, FadeTime::Pos);
			streamPlay = NULL; streamMixer = NULL;
		}

		if (streamMixer)
		{
			verify(BASS_StreamFree(streamMixer) == TRUE);
			streamMixer = NULL;
		}
		if (streamPlay)
		{
			verify(BASS_StreamFree(streamPlay) == TRUE);
			streamPlay = NULL;
		}
	}
	else // WASAPI & ASIO
	{
		if (streamPlay)
			BASS_ChannelGetInfo(streamPlay, &oldci);

		if (streamMixer)
			verify(BASS_ChannelRemoveSync(streamMixer, syncEndMix) == TRUE);

		if (needFade && isMediaPlay && !isMediaPause && !bassNoEffect) // Fade out stream
		{
			// We cannot use BASS_STREAM_AUTOFREE because streamPlay is BASS_STREAM_DECODE in this case, so use sync.
			//verify(BASS_ChannelFlags(streamPlay, BASS_STREAM_AUTOFREE, BASS_STREAM_AUTOFREE) != -1);
			BASS_ChannelSetSync(streamPlay, BASS_SYNC_MIXTIME|BASS_SYNC_SLIDE, 0, SyncFreePlay, this);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Pos);
			streamPlay = NULL;
		}

		if (streamPlay)
		{
			verify(BASS_StreamFree(streamPlay) == TRUE);
			streamPlay = NULL;
		}
	}
}

void LibAudio::RunThreadBuffer()
{
	while (!stopThreadBuffer)
	{
		mutexBuffer->Lock();
		for (std::size_t i = 0; i < bufferStreams.size();)
		{
			STRUCTBUFFER* buf = bufferStreams[i];
			mutexBuffer->Unlock();

			// Check how much data is currently buffered
			int c = (int)BASS_StreamPutData(buf->streamBuffer, 0, 0);
			if (c == -1) // 
			{
				// Free buffer stream
				verify(BASS_StreamFree(buf->streamFile));

				mutexBuffer->Lock();
				delete[] buf->buffer;
				delete buf;
				bufferStreams.erase(bufferStreams.begin() + i);
				//printf("FREE1\n");
				continue;
			}
			else
			{
				//AllocConsole();
				//freopen ("CONOUT$", "w", stdout);
				//printf("LEN: %u,        %i,%i\n", buf->streamBuffer, c, buf->bytes);

				if (buf->bytes > c) // Less data than wanted
				{
					// Get more data from the file
					c = BASS_ChannelGetData(buf->streamFile, buf->buffer, buf->bytes - c);

					if (c == -1) // File end
					{
						verify(BASS_StreamFree(buf->streamFile));

						// Preload next file
						//::PostMessage(libAudio->hParent, UWM_BASSPRELOAD, 0, 0);
						SyncProcPreloadImpl();

						// Signal to end stream
						BASS_StreamPutData(buf->streamBuffer, buf->buffer, BASS_STREAMPROC_END);

						// Free buffer stream
						mutexBuffer->Lock();
						delete[] buf->buffer;
						delete buf;
						bufferStreams.erase(bufferStreams.begin() + i);
						//printf("FREE2\n");
						continue;
					}
					else // Pass data to the buffer stream
						BASS_StreamPutData(buf->streamBuffer, buf->buffer, c);
				}
			}

			++i;
			mutexBuffer->Lock();
		}

		bool isEmptyBufferStreams = bufferStreams.empty();
		mutexBuffer->Unlock();

		if (isEmptyBufferStreams)
			eventBuffer->Wait();
		else
			eventBuffer->TryWait(100);

		eventPause->Wait();

		if (nextCueTrack)
		{
			SyncProcPreloadImpl();
			nextCueTrack = false;
		}
	}
}

HSTREAM LibAudio::OpenMediaFile(const std::wstring& file, QWORD* outByteLength, double* outTimeSecond, double* position, QWORD* outBytePosition, int* outError)
{
	HSTREAM streamFile = NULL;
	if (bassDriver == 0)
		streamFile = BASS_StreamCreateFile(FALSE, file.c_str(), 0, 0, BASS_ASYNCFILE|BASS_UNICODE|BASS_STREAM_DECODE|dwSampleEx);
	else
		streamFile = BASS_StreamCreateFile(FALSE, file.c_str(), 0, 0, BASS_ASYNCFILE|BASS_UNICODE|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT);

	if (streamFile == NULL)
	{
		if (outError)
			*outError = BASS_ErrorGetCode();

		return NULL;
	}

	// Get length of the stream in bytes and seconds
	*outByteLength = BASS_ChannelGetLength(streamFile, BASS_POS_BYTE);
	*outTimeSecond = BASS_ChannelBytes2Seconds(streamFile, *outByteLength);

	// Set start pos and return the pos in bytes
	if (position)
	{
		QWORD pos = BASS_ChannelSeconds2Bytes(streamFile, *position);
		BASS_ChannelSetPosition(streamFile, pos, BASS_POS_BYTE);

		if (outBytePosition)
			*outBytePosition = pos;
	}
	else if (outBytePosition)
		*outBytePosition = 0;

	// That is all for DirectSound

	// Reminder: If need to enable double reading buffer for DirectSound (like WASAPI/ASIO):
	// 1. comment next line 2. set DirectSound buffer = 500 3. In Init enable thread  4. in SyncGaplessDS uncomment preload line
	// 5. in GetPosition and GetTimePosition add posPlus 6. Below in hBuffer create stream with dwSampleEx
	// !7. In StartPlayDS add ::SetEvent(hBufferEvent);
	// ^ This comment is old btw some things are changed
	if (bassDriver == 0)
		return streamFile;

	// For WASAPI/ASIO create buffer stream that above file stream
	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(streamFile, &ci);

	HSTREAM streamBuffer = BASS_StreamCreate(ci.freq, ci.chans, BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT, STREAMPROC_PUSH, NULL);

	STRUCTBUFFER* buf = new STRUCTBUFFER;

	buf->streamFile = streamFile;
	buf->streamBuffer = streamBuffer;

	buf->bytes = (int)BASS_ChannelSeconds2Bytes(streamFile, Buffer::WasapiAsio);
	buf->buffer = new char[buf->bytes];

	// Add buffer stream to buffer streams queue (RunThreadBuffer does everything else)
	mutexBuffer->Lock();
	bufferStreams.push_back(buf);
	mutexBuffer->Unlock();

	// Add first portion of the data to buffer here (to fill buffer faster)
	DWORD c = BASS_ChannelGetData(buf->streamFile, buf->buffer, buf->bytes);
	BASS_StreamPutData(buf->streamBuffer, buf->buffer, c);

	return streamBuffer;
}

HSTREAM LibAudio::OpenMediaURL(const std::wstring& url, int* outError)
{
	HSTREAM streamFile = NULL;
	if (bassDriver == 0)
		streamFile = BASS_StreamCreateURL((char*)url.c_str(), 0, BASS_UNICODE|BASS_STREAM_BLOCK|BASS_STREAM_DECODE|dwSampleEx, NULL, 0);
	else
		streamFile = BASS_StreamCreateURL((char*)url.c_str(), 0, BASS_UNICODE|BASS_STREAM_BLOCK|BASS_STREAM_DECODE|BASS_SAMPLE_FLOAT, NULL, 0);

	if (streamFile == NULL && outError)
		*outError = BASS_ErrorGetCode();

	return streamFile;
}

void LibAudio::FreePreload()
{
	if (streamPreload)
	{
		BASS_StreamFree(streamPreload);
		streamPreload = NULL;
		filePreload.clear();
		isPreloadRate = false;
	}
	if (cuePreload)
	{
		cuePreload = 0;
		cueOffsetPreload = 0;
	}
}

LibAudio::Error LibAudio::PlayFile(const std::wstring& file, long long cue)
{
	// When Gapless Playback if a new file is already preloaded then free it
	FreePreload();

	bool needFade = isMediaPlay;
	PrepareOpen(true);

	fileThis = file;
	urlThis.clear();
	posPlus = 0;
	cueThis = cue;
	cueOffset = 0;

	int error = 0;

	if (!cue)
		streamPlay = OpenMediaFile(file, &byteLength, &timeSecond, nullptr, nullptr, &error);
	else
	{
		double offset = CueFile::GetOffset(cue);
		streamPlay = OpenMediaFile(file, &byteLengthCue, &timeSecondCue, &offset, &cueOffset, &error);

		posPlus = cueOffset;

		if (CueFile::IsLenght(cue))
		{
			timeSecond = CueFile::GetLenght(cue);
			byteLength = BASS_ChannelSeconds2Bytes(streamPlay, timeSecond);
		}
		else
		{
			timeSecond = timeSecondCue - offset;
			byteLength = byteLengthCue - cueOffset;
		}
	}

	if (streamPlay == NULL)
	{
		if (bassDriver == 1)
		{
			BASS_WASAPI_Stop(TRUE);
			BASS_WASAPI_Free();
		}
		else if (bassDriver == 2)
		{
			BASS_ASIO_Stop();
			BASS_ASIO_Free();
		}

		if (streamMixer)
		{
			BASS_StreamFree(streamMixer);
			streamMixer = NULL;
		}

		if (error == BASS_ERROR_INIT)
			return Error::Device;

		return Error::File;
	}

	isMediaPlay = true;
	isMediaPause = false;
	isMediaRadio = false;
	
	EQ_DEBUG_LOG("Media playback starting - equalizer will now be applied if enabled");
	if (isEqEnable) {
		EQ_DEBUG_LOG("Equalizer is enabled - will apply settings to new audio stream");
	}

	bool isStarted = false;

	if (bassDriver == 0)
		isStarted = StartPlayDS(true, needFade);
	else if (bassDriver == 1)
		isStarted = StartPlayWASAPI(true, needFade);
	else if (bassDriver == 2)
		isStarted = StartPlayASIO(true, needFade);

	if (!isStarted)
	{
		isMediaPlay = false;

		if (streamMixer)
		{
			BASS_StreamFree(streamMixer);
			streamMixer = NULL;
		}
		if (streamPlay)
		{
			BASS_StreamFree(streamPlay);
			streamPlay = NULL;
		}
		// Start buffer thread to remove all buffers
		if (eventBuffer)
			eventBuffer->Set();
		if (eventPause)
			eventPause->Set();

		return Error::Device;
	}

	return Error::None;
}

bool LibAudio::StartPlayDS(bool isFile, bool needFade, bool gaplessResume)
{
		if (gaplessResume)
		{
			verify(BASS_StreamFree(streamMixer) == TRUE);
			streamMixer = NULL;
		}

		BASS_CHANNELINFO ci;
		BASS_ChannelGetInfo(streamPlay, &ci);

		streamMixer = BASS_Mixer_StreamCreate(ci.freq, ci.chans, dwSampleEx|BASS_MIXER_END);
		BASS_Mixer_StreamAddChannel(streamMixer, streamPlay, BASS_MIXER_BUFFER);

		ApplyEqualizer();

		syncEnd = BASS_ChannelSetSync(streamMixer, BASS_SYNC_END, 0, SyncProcEnd, this);
		syncEndMix = BASS_ChannelSetSync(streamMixer, BASS_SYNC_END|BASS_SYNC_MIXTIME, 0, SyncProcGaplessDS, this);

		ResetCueSync(cueThis, timeSecond, posPlus, cueOffset);

		if (!gaplessResume)
		{
			if (!needFade || bassNoEffect)
				BASS_ChannelSetAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume);
			else
			{
				// Fade in volume
				BASS_ChannelSetAttribute(streamMixer, BASS_ATTRIB_VOL, 0.0f);
				BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume, FadeTime::Pos);
			}
		}
		else
		{
			BASS_ChannelSetAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume);
		}

		if (!isMediaPause)
			return BASS_ChannelPlay(streamMixer, FALSE) ? true : false;

		return true;
}

bool LibAudio::StartPlayWASAPI(bool isFile, bool needFade, bool gaplessResume)
{
	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(streamPlay, &ci);

	if (!gaplessResume)
	{
		if (!needFade || bassNoEffect)
			BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
		else
		{
			// Fade in volume
			BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume, FadeTime::Pos);
		}

		// If the mixer and the new stream sample rate are equal then just add the new stream to the mixer
		if (streamMixer)
		{
			//BASS_CHANNELINFO mi;
			//BASS_ChannelGetInfo(streamMixer, &mi);

			if (oldci.freq == ci.freq && oldci.chans == ci.chans)
			{
				syncEndMix = BASS_ChannelSetSync(streamMixer, BASS_SYNC_MIXTIME|BASS_SYNC_END, 0, SyncProcGaplessWA, this);
				BASS_Mixer_StreamAddChannel(streamMixer, streamPlay, BASS_MIXER_BUFFER|BASS_MIXER_NORAMPIN);
				BASS_ChannelSetPosition(streamMixer, 0, BASS_POS_BYTE);
				ResetCueSync(cueThis, timeSecond, posPlus, cueOffset);
				if (!BASS_WASAPI_IsStarted()) // If paused
					BASS_WASAPI_Start();
				eventBuffer->Set();
				eventPause->Set();
				return true;
			}
			else
			{
				verify(BASS_StreamFree(streamMixer) == TRUE);
				streamMixer = NULL;
			}
		}
	}
	else
	{
		verify(BASS_StreamFree(streamMixer) == TRUE);
		streamMixer = NULL;

		BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
	}

	// Restart WASAPI with new parameters

	BASS_WASAPI_Free();

	// We need streamMixerCopyWASAPI because we will get the mixer only after BASS_WASAPI_Init
	// but we need to send independent mixer not a pointer to WasapiProc (so it will be the same as AsioProc) (?)
	// otherwise when stop for example the mixer resets immediately but fading is still going it will cause a bug.
	streamMixerCopyWASAPI = NULL;

	// Convert bassDevice=-1 to actual device ID for WASAPI exclusive mode
	int actualDevice = bassDevice;
	if (bassDevice == -1) {
		WASAPI_DEBUG_LOG("Converting default device (-1) to actual device ID");
		// Find the default WASAPI device
		BASS_WASAPI_DEVICEINFO deviceInfo;
		for (int i = 0; i < 20; i++) { // Check first 20 devices
			if (BASS_WASAPI_GetDeviceInfo(i, &deviceInfo)) {
				if (deviceInfo.flags & BASS_DEVICE_DEFAULT) {
					actualDevice = i;
					WASAPI_DEBUG_LOGF("Found default device at index %d", i);
					break;
				}
			}
		}
		
		if (actualDevice == -1) {
			actualDevice = 0; // Fallback to device 0
			WASAPI_DEBUG_LOG("No default device found, using device 0");
		}
	}

	// https://msdn.microsoft.com/en-us/library/windows/desktop/dd370875%28v=vs.85%29.aspx
	WASAPI_DEBUG_LOGF("Initializing WASAPI with device %d (original: %d)", actualDevice, bassDevice);
	
	if (!wasapiEvent)
	{
		if (!BASS_WASAPI_Init(actualDevice, ci.freq, ci.chans, BASS_WASAPI_AUTOFORMAT|BASS_WASAPI_EXCLUSIVE,
			0.05f, 0, WasapiProc, (void*)&streamMixerCopyWASAPI)) {
			DWORD error = BASS_ErrorGetCode();
			WASAPI_DEBUG_LOGF("BASS_WASAPI_Init FAILED with error %d", error);
			return false;
		}
		WASAPI_DEBUG_LOG("BASS_WASAPI_Init SUCCESS");
	}
	else
	{
		if (!BASS_WASAPI_Init(actualDevice, ci.freq, ci.chans, BASS_WASAPI_AUTOFORMAT|BASS_WASAPI_EXCLUSIVE|BASS_WASAPI_EVENT,
			0.05f, 0, WasapiProc, (void*)&streamMixerCopyWASAPI)) {
			DWORD error = BASS_ErrorGetCode();
			WASAPI_DEBUG_LOGF("BASS_WASAPI_Init (with EVENT) FAILED with error %d", error);
			return false;
		}
		WASAPI_DEBUG_LOG("BASS_WASAPI_Init (with EVENT) SUCCESS");
	}

	BASS_WASAPI_INFO wi;
	BASS_WASAPI_GetInfo(&wi);

	streamMixer = BASS_Mixer_StreamCreate(wi.freq, wi.chans, BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE|BASS_MIXER_POSEX|BASS_MIXER_END);
	syncEndMix = BASS_ChannelSetSync(streamMixer, BASS_SYNC_MIXTIME|BASS_SYNC_END, 0, SyncProcGaplessWA, this);

	ResetCueSync(cueThis, timeSecond, posPlus, cueOffset);

	ApplyEqualizer();

	BASS_Mixer_StreamAddChannel(streamMixer, streamPlay, BASS_MIXER_DOWNMIX|BASS_MIXER_BUFFER);
	streamMixerCopyWASAPI = streamMixer;

	if (!isMediaPause)
	{
		BASS_WASAPI_Start();
		eventBuffer->Set();
		eventPause->Set();
	}

	return true;
}

bool LibAudio::StartPlayASIO(bool isFile, bool needFade, bool gaplessResume)
{
	BASS_CHANNELINFO ci;
	BASS_ChannelGetInfo(streamPlay, &ci);

	if (!gaplessResume)
	{
		if (!needFade || bassNoEffect)
			BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
		else
		{
			// Fade in volume
			BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume, FadeTime::Pos);
		}

		// If the mixer and the new stream sample rate are equal then just add the new stream to the mixer
		if (streamMixer)
		{
			//BASS_CHANNELINFO mi;
			//BASS_ChannelGetInfo(streamMixer, &mi);

			if (oldci.freq == ci.freq && oldci.chans == ci.chans)
			{
				syncEndMix = BASS_ChannelSetSync(streamMixer, BASS_SYNC_MIXTIME|BASS_SYNC_END, 0, SyncProcGaplessWA, this);
				BASS_Mixer_StreamAddChannel(streamMixer, streamPlay, BASS_MIXER_BUFFER|BASS_MIXER_NORAMPIN);
				BASS_ChannelSetPosition(streamMixer, 0, BASS_POS_BYTE);
				ResetCueSync(cueThis, timeSecond, posPlus, cueOffset);
				if (!BASS_ASIO_IsStarted()) // If paused
					BASS_ASIO_Start(0, 0);
				eventBuffer->Set();
				eventPause->Set();
				return true;
			}
			else
			{
				verify(BASS_StreamFree(streamMixer) == TRUE);
				streamMixer = NULL;
			}
		}
	}
	else
	{
		verify(BASS_StreamFree(streamMixer) == TRUE);
		streamMixer = NULL;

		BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
	}

	//////////////////////////////////////////////////////////////

//		AllocConsole();
//		freopen ("CONOUT$", "w", stdout); 
//		printf("%i,%i\n", ci.freq, ci.chans);

	streamMixer = BASS_Mixer_StreamCreate(ci.freq, ci.chans, BASS_SAMPLE_FLOAT|BASS_STREAM_DECODE|BASS_MIXER_POSEX|BASS_MIXER_END);
	syncEndMix = BASS_ChannelSetSync(streamMixer, BASS_SYNC_MIXTIME|BASS_SYNC_END, 0, SyncProcGaplessWA, this);

	ResetCueSync(cueThis, timeSecond, posPlus, cueOffset);

	ApplyEqualizer();

	BASS_Mixer_StreamAddChannel(streamMixer, streamPlay, BASS_MIXER_BUFFER);

	// Reset all channels
	if (!BASS_ASIO_Stop())
	{
		if (BASS_ASIO_ErrorGetCode() == BASS_ERROR_INIT &&
			!BASS_ASIO_Init(bassDevice, 0)) // The audio output device encountered an error.
			return false;
	}
	BASS_ASIO_ChannelReset(FALSE, -1, BASS_ASIO_RESET_ENABLE|BASS_ASIO_RESET_JOIN);

	BASS_ASIO_ChannelEnable(FALSE, asioChannel, &AsioProc, (void*)(DWORD_PTR)streamMixer); // enable 1st output channel...
	if (ci.chans == 1) // Enable mirror for mono to make it stereo (I'm not sure that it's right way to do this)
		BASS_ASIO_ChannelEnableMirror(asioChannel + 1, FALSE, 0);
	else
	{
		for (int i = asioChannel + 1; i < (int)ci.chans + asioChannel; ++i)
			BASS_ASIO_ChannelJoin(FALSE, i, asioChannel); // and join the next channels to it
	}
	BASS_ASIO_ChannelSetFormat(FALSE, asioChannel, BASS_ASIO_FORMAT_FLOAT); // set the source format (float)
	BASS_ASIO_ChannelSetRate(FALSE, asioChannel, ci.freq); // set the source rate
	if (!BASS_ASIO_SetRate(ci.freq)) // try to set the device rate too (saves resampling)
	{
		if (!BASS_ASIO_SetRate(48000))
			BASS_ASIO_SetRate(44100);
	}
	
	if (!isMediaPause)
	{
		BASS_ASIO_Start(0, 0);
		eventBuffer->Set();
		eventPause->Set();
	}

	return true;
}

void LibAudio::ResetCueSync(long long cue, double lenght, QWORD position, QWORD offset)
{
	if (syncEndCue)
	{
		verify(BASS_ChannelRemoveSync(streamMixer, syncEndCue));
		syncEndCue = NULL;
	}
	if (syncEndMixCue)
	{
		verify(BASS_ChannelRemoveSync(streamMixer, syncEndMixCue));
		syncEndMixCue = NULL;
	}

	// If part of cue and not a latest track in cue
	if (cue && CueFile::IsLenght(cue))
	{
		QWORD sync = BASS_ChannelGetPosition(streamMixer, BASS_POS_BYTE);
		if (position == offset)
			sync += BASS_ChannelSeconds2Bytes(streamMixer, lenght);
		else
		{
			double pos = BASS_ChannelBytes2Seconds(streamPlay, position - offset);
			sync += BASS_ChannelSeconds2Bytes(streamMixer, lenght - pos);
		}

		if (bassDriver == 0)
		{
			syncEndCue = BASS_ChannelSetSync(streamMixer, BASS_SYNC_POS, sync, SyncProcEnd, this);
			syncEndMixCue = BASS_ChannelSetSync(streamMixer, BASS_SYNC_POS|BASS_SYNC_MIXTIME, sync, SyncProcGaplessDS, this);
		}
		else if (bassDriver == 1 || bassDriver == 2)
		{
			syncEndCue = BASS_ChannelSetSync(streamMixer, BASS_SYNC_POS|BASS_SYNC_MIXTIME, sync, SyncProcGaplessWA, this);

			QWORD preload = BASS_ChannelSeconds2Bytes(streamMixer, Buffer::WasapiAsio);
			if (preload > sync) preload = sync;

			syncEndMixCue = BASS_ChannelSetSync(streamMixer, BASS_SYNC_POS|BASS_SYNC_MIXTIME, sync - preload, SyncProcGaplessCueWA, this);
		}
	}
}

void LibAudio::PlayURL(const std::wstring& url, bool isReconnect)
{
	// When Gapless Playback if a new file is already preloaded then free it
	FreePreload();

	isRadioReconnect = isReconnect;

	urlThis = url;
	fileThis.clear();
	posPlus = 0;
	cueThis = 0;
	cueOffset = 0;

	byteLength = 0;
	timeSecond = 0;

	if (!threadRadio.IsRunning())
	{
		PrepareOpen(true);

		if (threadRadio.IsJoinable())
			threadRadio.Join();

		threadRadio.Start(std::bind(&LibAudio::RunThreadRadio, this));
	}
}

void LibAudio::RunThreadRadio()
{
	while (true)
	{
		std::wstring url = urlThis;
		
		int error = 0;
		HSTREAM streamRadio = OpenMediaURL(url, &error);

		byteLength = 0;
		timeSecond = 0;

		if (urlThis.empty()) // Stop pressed while loading, exit
		{
			BASS_StreamFree(streamRadio);
			break;
		}
		else if (url != urlThis) // URL changed while loading, reload
		{
			BASS_StreamFree(streamRadio);
			continue;
		}
		else
		{
			streamPlay = streamRadio;

			// Return to GUI thread
			::PostMessage(wndWinyl->Wnd(), UWM_RADIOSTART, error, isRadioReconnect);

			break;
		}
	}
}

LibAudio::Error LibAudio::StartRadio(int error)
{
	if (streamPlay == NULL)
	{
		isMediaPlay = false;
		isMediaPause = false;
		isMediaRadio = false;
		urlThis.clear();

		if (bassDriver == 1)
		{
			BASS_WASAPI_Stop(TRUE);
			BASS_WASAPI_Free();
		}
		else if (bassDriver == 2)
		{
			BASS_ASIO_Stop();
			BASS_ASIO_Free();
		}

		if (streamMixer)
		{
			BASS_StreamFree(streamMixer);
			streamMixer = NULL;
		}

		switch (error)
		{
		case BASS_ERROR_INIT:
			return Error::Device;
		case BASS_ERROR_NONET:
		case BASS_ERROR_TIMEOUT:
		case BASS_ERROR_ILLPARAM:
		case BASS_ERROR_UNKNOWN:
			return Error::Inet;
		}
		return Error::File;
	}

	// http://www.un4seen.com/forum/?topic=12314.0;hl=get+bitrate
	QWORD bufLen = BASS_StreamGetFilePosition(streamPlay, BASS_FILEPOS_END);
	if (bufLen != -1)
		radioBitrate = (int)(bufLen * 8 / BASS_GetConfig(BASS_CONFIG_NET_BUFFER));
	else
		radioBitrate = 0;

	BASS_ChannelSetSync(streamPlay, BASS_SYNC_META, 0, SyncRadioMeta, this);

	QWORD radioLength = BASS_StreamGetFilePosition(streamPlay, BASS_FILEPOS_SIZE);
	if (radioLength == -1)
		isRadioStream = true;
	else
		isRadioStream = false;

	// Test code to open url with offset
	//radioLength = BASS_StreamGetFilePosition(streamPlay, BASS_FILEPOS_SIZE);
	//if (radioOffset == 0 && radioLength != -1)
	//{
	//	radioLength -= BASS_StreamGetFilePosition(streamPlay, BASS_FILEPOS_START);
	//	byteLength = BASS_ChannelGetLength(streamPlay, BASS_POS_BYTE);
	//	timeSecond = BASS_ChannelBytes2Seconds(streamPlay, byteLength);
	//}

	///////////////////////////////////////

	isMediaPlay = true;
	isMediaPause = false;
	isMediaRadio = true;

	bool isStarted = false;

	if (bassDriver == 0)
		isStarted = StartPlayDS(false, true);
	else if (bassDriver == 1)
		isStarted = StartPlayWASAPI(false, true);
	else if (bassDriver == 2)
		isStarted = StartPlayASIO(false, true);

	if (!isStarted)
	{
		isMediaPlay = false;
		isMediaRadio = false;
		urlThis.clear();

		if (streamMixer)
		{
			BASS_StreamFree(streamMixer);
			streamMixer = NULL;
		}
		if (streamPlay)
		{
			BASS_StreamFree(streamPlay);
			streamPlay = NULL;
		}

		return Error::Device;
	}

	return Error::None;
}

void LibAudio::SyncRadioMeta(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	::PostMessage(libAudio->wndWinyl->Wnd(), UWM_RADIOMETA, 0, 0);
}

int LibAudio::GetPosition()
{
	if (isMediaRadio || !streamPlay)
		return 0;

	//QWORD qSize = BASS_ChannelGetLength(mainChan, BASS_POS_BYTE);
	QWORD pos = 0;
	
	if (bassDriver == 0)
		pos = BASS_Mixer_ChannelGetPosition(streamPlay, BASS_POS_BYTE);// + posPlus;
		//pos = BASS_ChannelGetPosition(streamMixer, BASS_POS_BYTE);
	else if (bassDriver == 1 || bassDriver == 2)
		pos = BASS_Mixer_ChannelGetPosition(streamPlay, BASS_POS_BYTE) + posPlus;

	if (byteLength == 0)
		return 0;

	return (int)((pos - cueOffset) * 100000 / byteLength);
}

int LibAudio::GetTimePosition()
{
	if (isMediaRadio || !streamPlay)
		return 0;

	QWORD pos = 0;
	
	if (bassDriver == 0)
		pos = BASS_Mixer_ChannelGetPosition(streamPlay, BASS_POS_BYTE);// + posPlus;
		//qPos = BASS_ChannelGetPosition(streamMixer, BASS_POS_BYTE);
	else if (bassDriver == 1 || bassDriver == 2)
		pos = BASS_Mixer_ChannelGetPosition(streamPlay, BASS_POS_BYTE) + posPlus;

	return (int)std::round(BASS_ChannelBytes2Seconds(streamPlay, pos - cueOffset));
}

void LibAudio::SetPosition(int position)
{
	if (isMediaRadio || !streamPlay)
		return;

	// mm:ss:ff (minute-second-frame) format - http://www.un4seen.com/forum/?topic=15777.0
	//bytepos=BASS_ChannelSeconds2Bytes(stream, minutes*60.0+seconds+frames/75.0);
	//BASS_ChannelSetPosition(stream, bytepos, BASS_POS_BYTE);

	// Preloaded stream can be already added into the mixer, this is more likely for DirectSound because
	// the stream added to the mixer a second before a new track starts but it possible for WASAPI/ASIO too
	// so remove the stream from the mixer or it will cause a problem when a new stream will be added to the mixer.
	// Reminder: How to reproduce the bug: move a track slider back when the stream is already added to the mixer.
	if (streamPreload && BASS_Mixer_ChannelGetMixer(streamPreload))
		BASS_Mixer_ChannelRemove(streamPreload);

	FreePreload();

	isMediaPause = false; // Important!

	PrepareOpen(true);

	double pos = std::min((double)position * timeSecond / 100000.0, timeSecond - 0.1);
	
	if (!cueThis)
		streamPlay = OpenMediaFile(fileThis, &byteLength, &timeSecond, &pos, &posPlus);
	else
	{
		double offset = CueFile::GetOffset(cueThis);
		double newpos = offset + pos;
		streamPlay = OpenMediaFile(fileThis, &byteLengthCue, &timeSecondCue, &newpos, &posPlus);

		cueOffset = BASS_ChannelSeconds2Bytes(streamPlay, offset);

		if (CueFile::IsLenght(cueThis))
		{
			timeSecond = CueFile::GetLenght(cueThis);
			byteLength = BASS_ChannelSeconds2Bytes(streamPlay, timeSecond);
		}
		else
		{
			timeSecond = timeSecondCue - offset;
			byteLength = byteLengthCue - cueOffset;
		}
	}

	if (bassDriver == 0)
		StartPlayDS(true, true);
	else if (bassDriver == 1)
		StartPlayWASAPI(true, true);
	else if (bassDriver == 2)
		StartPlayASIO(true, true);

	// Reminder: How to directly set mixer pos
//	BASS_ChannelStop(streamMixer);
//	BASS_Mixer_ChannelSetPosition(streamPlay, (QWORD)iPosition * byteLength / 100000, BASS_POS_BYTE);
//	BASS_ChannelPlay(streamMixer, TRUE);
}

void LibAudio::SetVolume(int volume)
{
	soundVolume = (float)volume / 100000;

	if (bassNoVolume)
		return;

	if (!isSoundMute)
	{
		realVolume = soundVolume;
		if (bassDriver == 0)
			BASS_ChannelSetAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume);
		else
			BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
	}
}

int LibAudio::GetVolume()
{
	return int(soundVolume * 100000);
}

void LibAudio::Stop()
{
	// When Gapless Playback if a new file is already preloaded then free it
	FreePreload();
	ResetCueSync();

	if (!isMediaPlay)
		return;

	isMediaPlay = false;
	isMediaPause = false;

	if (bassDriver == 0)
	{
		if (bassNoEffect)
		{
			BASS_ChannelStop(streamMixer);
			BASS_StreamFree(streamMixer); streamMixer = NULL;
			BASS_StreamFree(streamPlay); streamPlay = NULL;
		}
		else
		{
			// Use BASS_STREAM_AUTOFREE without sync.
			assert(!(BASS_ChannelFlags(streamMixer, 0, 0) & BASS_STREAM_DECODE));
			verify(BASS_Mixer_ChannelFlags(streamPlay, BASS_STREAM_AUTOFREE, BASS_STREAM_AUTOFREE) != -1); // Important!
			verify(BASS_ChannelFlags(streamMixer, BASS_STREAM_AUTOFREE, BASS_STREAM_AUTOFREE) != -1); // Important!
			//BASS_ChannelSetSync(streamMixer, BASS_SYNC_SLIDE|BASS_SYNC_ONETIME, 0, SyncFreeMixer, this);
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, -1, FadeTime::Stop);
			streamPlay = NULL; streamMixer = NULL;
		}
	}
	else if (bassDriver == 1)
	{
		if (bassNoEffect)
		{
			BASS_WASAPI_Stop(TRUE);
			BASS_WASAPI_Free();
			BASS_StreamFree(streamMixer); streamMixer = NULL;
			BASS_StreamFree(streamPlay); streamPlay = NULL;
		}
		else
		{
			BASS_ChannelSetSync(streamPlay, BASS_SYNC_SLIDE|BASS_SYNC_ONETIME, 0, SyncWASAPIStop, this);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Stop);
			streamPlay = NULL; streamMixer = NULL;
		}
	}
	else if (bassDriver == 2)
	{
		if (bassNoEffect)
		{
			BASS_ASIO_Stop();
			BASS_ASIO_Free();
			BASS_StreamFree(streamMixer); streamMixer = NULL;
			BASS_StreamFree(streamPlay); streamPlay = NULL;
		}
		else
		{
			BASS_ChannelSetSync(streamPlay, BASS_SYNC_SLIDE|BASS_SYNC_ONETIME, 0, SyncASIOStop, this);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Stop);
			streamPlay = NULL; streamMixer = NULL;
		}
	}
}

void LibAudio::Pause()
{
	if (!isMediaPlay)
		return;
	if (isMediaPause)
		return;

	isMediaPause = true;

	if (bassDriver == 0)
	{
		if (bassNoEffect)
			BASS_ChannelPause(streamMixer);
		else
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, -1, FadeTime::Pause);
	}
	else if (bassDriver == 1)
	{
		if (bassNoEffect)
		{
			BASS_WASAPI_Stop(TRUE);
			eventPause->Reset();
		}
		else
		{
			BASS_ChannelSetSync(streamPlay, BASS_SYNC_SLIDE, 0, SyncWASAPIPause, this);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Pause);
		}
	}
	else if (bassDriver == 2)
	{
		if (bassNoEffect)
		{
			BASS_ASIO_Stop();
			eventPause->Reset();
		}
		else
		{
			BASS_ChannelSetSync(streamPlay, BASS_SYNC_SLIDE, 0, SyncASIOPause, this);
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Pause);
		}
	}
}

void LibAudio::Play()
{
	if (!isMediaPlay)
		return;
	if (!isMediaPause)
		return;

	isMediaPause = false;

	if (bassDriver == 0)
	{
		if (bassNoEffect)
			BASS_ChannelPlay(streamMixer, FALSE);
		else
		{
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume, FadeTime::Pause);
			BASS_ChannelPlay(streamMixer, FALSE);
		}
	}
	else if (bassDriver == 1)
	{
		eventPause->Set();
		if (bassNoEffect)
			BASS_WASAPI_Start();
		else
		{
			// If press pause at the end of a track with big fading then will be a bug (won't play next track).
			// It fixes this bug (I do not know how it works).
			// Now it fixed other way, see SyncProcGaplessWA function.
			//if (BASS_ChannelIsActive(streamMixer) == 0)
			//	BASS_ChannelSetPosition(streamMixer, 0, BASS_POS_BYTE);

			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume, FadeTime::Pause);
			BASS_WASAPI_Start();
		}
	}
	else if (bassDriver == 2)
	{
		eventPause->Set();
		if (bassNoEffect)
			BASS_ASIO_Start(0, 0);
		else
		{
			// If press pause at the end of a track with big fading then will be a bug (won't play next track).
			// It fixes this bug (I do not know how it works).
			// Now it fixed other way, see SyncProcGaplessWA function.
			//if (BASS_ChannelIsActive(streamMixer) == 0)
			//	BASS_ChannelSetPosition(streamMixer, 0, BASS_POS_BYTE);

			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume, FadeTime::Pause);
			BASS_ASIO_Start(0, 0);
		}
	}
}

void LibAudio::SyncFreePlay(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	//assert(user == NULL);
	//verify(BASS_StreamFree(channel) == TRUE); // streamPlay
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (libAudio->wndWinyl->IsWnd())
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSCHFREE, 0, (LPARAM)channel);
}

//void LibAudio::SyncFreeMixer(HSYNC handle, DWORD channel, DWORD data, void* user)
//{
//	//assert(user != NULL);
//	//verify(BASS_StreamFree(channel) == TRUE); // streamMixer
//	//verify(BASS_StreamFree((HSTREAM)(DWORD_PTR)user) == TRUE); // streamPlay
//	LibAudio* libAudio = static_cast<LibAudio*>(user);
//
//	if (libAudio->wndWinyl->IsWnd())
//		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSCHFREE, 0, (LPARAM)channel);
//}

void LibAudio::SyncWASAPIPause(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (libAudio->wndWinyl->IsWnd())
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSWASTOP, 0, (LPARAM)channel);
}

void LibAudio::SyncASIOPause(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (libAudio->wndWinyl->IsWnd())
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSWASTOP, 0, (LPARAM)channel);
}

void LibAudio::SyncWASAPIStop(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (libAudio->wndWinyl->IsWnd())
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSWASTOP, 1, (LPARAM)channel);
}

void LibAudio::SyncASIOStop(HSYNC handle, DWORD channel, DWORD data, void* user)
{
	LibAudio* libAudio = static_cast<LibAudio*>(user);

	if (libAudio->wndWinyl->IsWnd())
		::PostMessage(libAudio->wndWinyl->Wnd(), UWM_BASSWASTOP, 1, (LPARAM)channel);
}

void LibAudio::SyncFreeChannelImpl(HSTREAM channel)
{
	verify(BASS_StreamFree(channel) == TRUE);
}

void LibAudio::SyncWAStopPauseImpl(bool isStop, HSTREAM channel)
{
	if (!isStop) // Pause
	{
		if (isMediaPause)
		{
			if (bassDriver == 1)
			{
				BASS_WASAPI_Stop(TRUE);
				eventPause->Reset();
			}
			else if (bassDriver == 2)
			{
				BASS_ASIO_Stop();
				eventPause->Reset();
			}
		}
	}
	else // Stop
	{
		if (!isMediaPlay)
		{
			if (bassDriver == 1)
			{
				BASS_WASAPI_Stop(TRUE);
				BASS_WASAPI_Free();
			}
			else if (bassDriver == 2)
			{
				BASS_ASIO_Stop();
				BASS_ASIO_Free();
			}
		}

		verify(BASS_StreamFree(BASS_Mixer_ChannelGetMixer(channel)) == TRUE);
		verify(BASS_StreamFree(channel) == TRUE);
	}
}

void LibAudio::SetMute(bool isMute)
{
	isSoundMute = isMute;

	if (bassNoVolume)
		return;

	if (isSoundMute)
	{
		realVolume = 0.0f;
		//BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, 0);
		if (bassDriver == 0)
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, 0.0f, FadeTime::Mute);
		else
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, 0.0f, FadeTime::Mute);
	}
	else
	{
		realVolume = soundVolume;
		//BASS_ChannelSetAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume);
		if (bassDriver == 0)
			BASS_ChannelSlideAttribute(streamMixer, BASS_ATTRIB_VOL, realVolume, FadeTime::Mute);
		else
			BASS_ChannelSlideAttribute(streamPlay, BASS_ATTRIB_VOL, realVolume, FadeTime::Mute);
	}
}

void LibAudio::GetFFT(float* fft)
{
	BASS_Mixer_ChannelGetData(streamPlay, fft, BASS_DATA_FFT2048);
}

void LibAudio::GetFFT2(float* fft, int size)
{
	BASS_Mixer_ChannelGetData(streamPlay, fft, size|BASS_DATA_FLOAT);
}

void LibAudio::SetEq(int band, float gain)
{
	EQ_DEBUG_LOGF("SetEq called: band=%d, gain=%.2f", band, gain);
	
	// Validate band range
	if (band < 0 || band >= 10) {
		EQ_DEBUG_LOGF("ERROR: Invalid band %d (must be 0-9)", band);
		return;
	}
	
	eqValues[band] = gain;

	if (!isMediaPlay) {
		EQ_DEBUG_LOG("No media playing, EQ settings stored but not applied");
		return;
	}

	// Check if the effect handle for this band is valid
	if (fxEqualizer[band] == 0) {
		EQ_DEBUG_LOGF("ERROR: fxEqualizer[%d] is 0 - EQ effect not initialized!", band);
		return;
	}

	// Use BASS_DX8_PARAMEQ structure for built-in BASS effect
	BASS_DX8_PARAMEQ eq;
	eq.fCenter = eqFrequencies[band];  // Use frequency from our array
	eq.fGain = gain;                  // Gain in dB 
	
	// CRITICAL FIX: Use valid DirectX 8 bandwidth value (1.0-36.0 semitones)
	// Bandwidth options: 6.0f (narrow/surgical), 12.0f (default/balanced), 18.0f (wide/musical)
	eq.fBandwidth = 18.0f;  // Wide bandwidth for musical EQ character (original Winyl approach)
	
	// Apply the parameters to the specific band effect
	if (!BASS_FXSetParameters(fxEqualizer[band], (void*)&eq)) {
		int error = BASS_ErrorGetCode();
		EQ_DEBUG_LOGF("ERROR: EQ band %d (%.0f Hz, %.1f dB) failed with BASS error %d", band, eq.fCenter, eq.fGain, error);
	}
	// Note: Success case logging removed to reduce noise
}

void LibAudio::SetPreamp(float preamp)
{
	EQ_DEBUG_LOGF("SetPreamp called: preamp=%.2f", preamp);
	eqPreamp = preamp;

	if (!isMediaPlay) {
		EQ_DEBUG_LOG("No media playing, preamp setting stored but not applied");
		return;
	}

	BASS_FX_VOLUME_PARAM fv;
	fv.fTarget = (float)pow(10, (preamp / 20));
	fv.fCurrent = fv.fTarget;  // Set current to target (no fade)
	fv.fTime = 0;              // No fade time
	fv.lCurve = 0;             // Linear curve

	EQ_DEBUG_LOGF("Calling BASS_FXSetParameters with fxPreamp=%d, volume=%.3f", fxPreamp, fv.fTarget);
	if (fxPreamp == 0) {
		EQ_DEBUG_LOG("ERROR: fxPreamp is 0 - Preamp effect not initialized!");
	}
	BASS_FXSetParameters(fxPreamp, (void*)&fv);
}

void LibAudio::EnableEq(bool enable)
{
	EQ_DEBUG_LOGF("EnableEq called with enable=%d", enable);
	isEqEnable = enable;

	if (!isMediaPlay) {
		EQ_DEBUG_LOG("No media playing, equalizer not applied");
		return;
	}

	// For WASAPI/ASIO, effects must be applied to mixer stream, not decoded source stream
	HSTREAM hChannel;
	if (bassDriver == 1 || bassDriver == 2) {
		// WASAPI/ASIO: Apply effects to mixer stream (where audio is actually processed)
		hChannel = streamMixer;
		EQ_DEBUG_LOGF("WASAPI/ASIO: Applying equalizer to MIXER stream %d (not source %d)", hChannel, streamPlay);
		
		// CRITICAL CHECK: Verify mixer stream is valid
		if (hChannel == 0 || hChannel == NULL) {
			EQ_DEBUG_LOG("CRITICAL ERROR: streamMixer is NULL/0 - effects will fail!");
			EQ_DEBUG_LOG("Fallback: Attempting to apply effects to source stream instead");
			hChannel = streamPlay;
		}
	} else {
		// DirectSound: Apply effects to source stream (traditional approach)
		hChannel = streamPlay;
		EQ_DEBUG_LOGF("DirectSound: Applying equalizer to SOURCE stream %d (mixer %d)", hChannel, streamMixer);
	}
	
	// Verify final target stream is valid
	EQ_DEBUG_LOGF("Final target stream: %d (bassDriver=%d)", hChannel, bassDriver);

	if (enable)
	{
		EQ_DEBUG_LOG("Attempting to create BASS_FX equalizer effects...");
		
		// Check if channel is valid first
		BASS_CHANNELINFO ci;
		if (BASS_ChannelGetInfo(hChannel, &ci)) {
			EQ_DEBUG_LOGF("Channel info: freq=%d, chans=%d, flags=0x%x, ctype=0x%x", 
				ci.freq, ci.chans, ci.flags, ci.ctype);
		} else {
			int error = BASS_ErrorGetCode();
			EQ_DEBUG_LOGF("ERROR: BASS_ChannelGetInfo failed with error %d", error);
		}
		
		// Try built-in BASS effects instead of BASS_FX library
		EQ_DEBUG_LOG("Creating built-in BASS effects...");
		
		fxPreamp = BASS_ChannelSetFX(hChannel, BASS_FX_VOLUME, 0);
		if (fxPreamp == 0) {
			int error = BASS_ErrorGetCode();
			EQ_DEBUG_LOGF("ERROR: BASS_FX_VOLUME creation failed with error %d", error);
		}
		
		// Create 10 separate BASS_FX_DX8_PARAMEQ effects for 10-band equalizer
		int failedBands = 0;
		for (int i = 0; i < 10; i++) {
			fxEqualizer[i] = BASS_ChannelSetFX(hChannel, BASS_FX_DX8_PARAMEQ, i + 2); // Priority 2-11
			if (fxEqualizer[i] == 0) {
				int error = BASS_ErrorGetCode();
				EQ_DEBUG_LOGF("ERROR: EQ band %d (%.0f Hz) creation failed with error %d", i, eqFrequencies[i], error);
				failedBands++;
			}
		}
		
		if (failedBands == 0) {
			EQ_DEBUG_LOG("SUCCESS: All EQ effects created successfully");
		} else {
			EQ_DEBUG_LOGF("WARNING: %d EQ bands failed to create", failedBands);
		}
		
		// Note: Settings will be applied when SetEq/SetPreamp are called
	}
	else
	{
		EQ_DEBUG_LOG("Disabling equalizer effects");
		BASS_ChannelRemoveFX(hChannel, fxPreamp);
		for (int i = 0; i < 10; i++) {
			if (fxEqualizer[i] != 0) {
				BASS_ChannelRemoveFX(hChannel, fxEqualizer[i]);
				fxEqualizer[i] = 0;
			}
		}
		fxPreamp = 0;
	}
}

void LibAudio::ApplyEqualizer()
{
	EQ_DEBUG_LOGF("ApplyEqualizer called - isEqEnable=%d", isEqEnable);
	if (isEqEnable)
	{
		EQ_DEBUG_LOG("Equalizer is enabled - applying all settings to active audio stream");
		EnableEq(true);

		EQ_DEBUG_LOGF("Applying preamp: %.2f", eqPreamp);
		SetPreamp(eqPreamp);

		EQ_DEBUG_LOG("Applying equalizer bands:");
		for (int i = 0; i < 10; ++i) {
			EQ_DEBUG_LOGF("  Band %d: %.2f", i, eqValues[i]);
			SetEq(i, eqValues[i]);
		}
		EQ_DEBUG_LOG("All equalizer settings applied successfully");
	}
	else
	{
		EQ_DEBUG_LOG("Equalizer is disabled - no settings applied");
	}
}

bool LibAudio::SaveEqualizer()
{
	XmlFile xmlFile;
	XmlNode xmlMain = xmlFile.RootNode().AddChild("Equalizer");
	
	if (xmlMain)
	{
		xmlMain.AddAttribute("Enable", (int)isEqEnable);

		xmlMain.AddAttribute16("Preset", eqPreset);

		SaveEqBand(xmlMain, "Preamp", eqPreamp);

		SaveEqBand(xmlMain, "Band01", eqValues[0]);
		SaveEqBand(xmlMain, "Band02", eqValues[1]);
		SaveEqBand(xmlMain, "Band03", eqValues[2]);
		SaveEqBand(xmlMain, "Band04", eqValues[3]);
		SaveEqBand(xmlMain, "Band05", eqValues[4]);
		SaveEqBand(xmlMain, "Band06", eqValues[5]);
		SaveEqBand(xmlMain, "Band07", eqValues[6]);
		SaveEqBand(xmlMain, "Band08", eqValues[7]);
		SaveEqBand(xmlMain, "Band09", eqValues[8]);
		SaveEqBand(xmlMain, "Band10", eqValues[9]);
	}

	std::wstring file = profilePath + L"Equalizer";

	FileSystem::CreateDir(file);
	
	file.push_back('\\');
	file += L"Equalizer.xml";

	if (xmlFile.SaveFile(file))
		return true;

	return false;
}

void LibAudio::SaveEqBand(XmlNode& xmlNode, char* name, float f)
{
	XmlNode xmlGain = xmlNode.AddChild(name);

	if (xmlGain)
	{
		char gain[10] = {};
		sprintf_s(gain, "%.1f", f); // 1 decimal place // swprintf 1024

		xmlGain.AddAttributeRaw("Gain", gain);
	}
}

bool LibAudio::LoadEqualizer()
{
	EQ_DEBUG_LOG("LoadEqualizer() starting...");
	std::wstring file = profilePath;
	file += L"Equalizer";
	file.push_back('\\');
	file += L"Equalizer.xml";

	EQ_DEBUG_LOGF("Loading equalizer config from: %s", file.c_str());
	XmlFile xmlFile;

	if (xmlFile.LoadFile(file))
	{
		XmlNode xmlMain = xmlFile.RootNode().FirstChild("Equalizer");
		
		if (xmlMain)
		{
			eqPreset = xmlMain.Attribute16("Preset");
			EQ_DEBUG_LOGF("Loaded preset: %s", eqPreset.c_str());

			xmlMain.Attribute("Enable", &isEqEnable);
			EQ_DEBUG_LOGF("Equalizer enabled in config: %d", isEqEnable);

			LoadEqBand(xmlMain, "Preamp", &eqPreamp);
			EQ_DEBUG_LOGF("Preamp value: %.2f", eqPreamp);

			LoadEqBand(xmlMain, "Band01", &eqValues[0]);
			LoadEqBand(xmlMain, "Band02", &eqValues[1]);
			LoadEqBand(xmlMain, "Band03", &eqValues[2]);
			LoadEqBand(xmlMain, "Band04", &eqValues[3]);
			LoadEqBand(xmlMain, "Band05", &eqValues[4]);
			LoadEqBand(xmlMain, "Band06", &eqValues[5]);
			LoadEqBand(xmlMain, "Band07", &eqValues[6]);
			LoadEqBand(xmlMain, "Band08", &eqValues[7]);
			LoadEqBand(xmlMain, "Band09", &eqValues[8]);
			LoadEqBand(xmlMain, "Band10", &eqValues[9]);
			
			EQ_DEBUG_LOG("All equalizer bands loaded successfully");
		}
		else
		{
			EQ_DEBUG_LOG("ERROR: No Equalizer node found in XML");
		}
	}
	else
	{
		EQ_DEBUG_LOG("ERROR: Failed to load equalizer XML file");
		return false;
	}

	EQ_DEBUG_LOG("LoadEqualizer() completed successfully");
	return true;
}

void LibAudio::LoadEqBand(XmlNode& xmlNode, char* name, float* f)
{
	XmlNode xmlGain = xmlNode.FirstChild(name);
	if (xmlGain)
	{
		const char* gain = xmlGain.AttributeRaw("Gain");
		if (gain)
		{
			(*f) = (float)atof(gain);
			return;
		}
	}
	
	(*f) = 0.0f;
}

bool LibAudio::GetDeviceName(int device, std::wstring& deviceName)
{
	deviceName.clear();

	BASS_DEVICEINFO di;
	if (!BASS_GetDeviceInfo((DWORD)device, &di))
		return false;

	if (di.name == NULL || !(di.flags & BASS_DEVICE_ENABLED) || (di.flags & BASS_DEVICE_DEFAULT))
		return true;

	if (di.name)
		deviceName = UTF::UTF16(di.name);

	return true;
}

bool LibAudio::GetDeviceNameWASAPI(int device, std::wstring& deviceName)
{
	deviceName.clear();

	BASS_WASAPI_DEVICEINFO di;
	if (!BASS_WASAPI_GetDeviceInfo((DWORD)device, &di))
		return false;

	if (di.name == NULL || !(di.flags & BASS_DEVICE_ENABLED) || (di.flags & BASS_DEVICE_INPUT))
		return true;

	if (di.name)
		deviceName = UTF::UTF16(di.name);

	return true;
}

bool LibAudio::GetDeviceNameASIO(int device, std::wstring& deviceName)
{
	deviceName.clear();

	BASS_ASIO_DEVICEINFO di;
	if (!BASS_ASIO_GetDeviceInfo((DWORD)device, &di))
		return false;

	if (di.name == NULL)
		return true;

	if (di.name)
		deviceName = (const wchar_t*)di.name;

	return true;
}

bool LibAudio::GetChannelNameASIO(int channel, std::wstring& channelName)
{
	channelName.clear();

	BASS_ASIO_CHANNELINFO ci;
	if (!BASS_ASIO_ChannelGetInfo(FALSE, channel, &ci))
		return false;

	channelName = UTF::UTF16A(ci.name);
	return true;
}

DWORD LibAudio::AsioProc(BOOL input, DWORD channel, void* buffer, DWORD length, void* user)
{
	DWORD c = BASS_ChannelGetData((DWORD)(DWORD_PTR)user, buffer, length);
//	if (c == -1) c = 0; // an error, no data
	if (c == -1) // an error, produce silence
	{
		c = length;
		memset(buffer, 0, c);
	}
	return c;
}

DWORD LibAudio::WasapiProc(void* buffer, DWORD length, void* user)
{
	DWORD c = BASS_ChannelGetData(*(DWORD*)(user), buffer, length);
//	if (c == -1) c = 0; // an error, no data
	if (c == -1) // an error, produce silence
	{
		c = length;
		memset(buffer, 0, c);
	}
	return c;
}


