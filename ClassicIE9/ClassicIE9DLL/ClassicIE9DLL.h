// Classic Shell (c) 2009-2013, Ivo Beltchev
// The sources for Classic Shell are distributed under the MIT open source license

#pragma once

#ifdef CLASSICIE9DLL_EXPORTS
#define CSIE9API __declspec(dllexport)
#else
#define CSIE9API __declspec(dllimport)
#endif

void InitClassicIE9( HMODULE hModule );
CSIE9API void ShowIE9Settings( void );
CSIE9API DWORD GetIE9Settings( void );
CSIE9API void CheckForNewVersionIE9( void );
CSIE9API void LogMessage( const char *text, ... );

enum
{
	IE9_SETTING_CAPTION=1,
	IE9_SETTING_PROGRESS=2,
	IE9_SETTING_ZONE=4,
	IE9_SETTING_PROTECTED=8,
};
