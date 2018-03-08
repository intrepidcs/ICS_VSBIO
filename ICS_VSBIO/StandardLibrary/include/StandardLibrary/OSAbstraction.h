//StandardLibrary/vstd/OSAbstraction.h

#ifndef _OSABSTRACTION_H
#define _OSABSTRACTION_H

#ifndef _WIN32
#include <stddef.h>
#define ZeroMemory(a, b) memset(a, 0, b)
#endif

#define ICSUNI(string) L##string

#if defined(PLASMA_BUILD)
#include <jni.h>
#include <android/log.h>
/**
         * Android port of our favorite debug print function.
         */
void OutputDebugStringA(const char* lpOutputString);
#endif

#include <stdio.h>
#include <Core/Defines.h>

//a couple of handy functions to convert between wchar_t and char and back
void char_to_wchar(wchar_t* Dest, const char* Src);
void wchar_to_char(char* Dest, const wchar_t* Src);

#if defined(_WIN32) || defined(__BORLANDC__)
void OSleep(unsigned long lTime);
char* Oultoa(unsigned long value, char* str, int radix);

#define OGetSystemTime timeGetTime
#define Owcslen wcslen
#define Owtoi _wtoi
#define Owcsnicmp _wcsnicmp
#define Owcsicmp _wcsicmp
#define Owcstod wcstod
#define Owcschr wcschr
#define Owcsrchr wcsrchr
#define Owcsncmp wcsncmp
#define Owcsncpy wcsncpy
#define Owcscat wcscat
#define OGetThreadID GetCurrentThreadId

#ifndef ANDROID
inline int fixup_OsprintfW(wchar_t* buffer, size_t count, int swprintfRet)
{
	if (swprintfRet < 0 || swprintfRet >= (int)count)
		buffer[count] = L'\0';
	return swprintfRet;
}

#ifdef __BORLANDC__
#define OsprintfW(B,C,F,...) fixup_OsprintfW(B, C - 1, swprintf(B, C - 1, F, __VA_ARGS__))
#else
#define OsprintfW(B,C,F,...) fixup_OsprintfW(B, C - 1, swprintf(B, C, F, __VA_ARGS__))
#endif
#endif

#if defined(__BORLANDC__)
#define Oitoa itoa
#else
#define Oitoa _itoa
#endif

#define Owcstoul wcstoul
#define Owcstol wcstol
#define Owcstombs wcstombs
#define Ombstowcs mbstowcs
#define Owchdir _wchdir
#define Owmkdir _wmkdir
#define Owfopen _wfopen
#define Owgetcwd _wgetcwd
#define Owunlink _wunlink
#define Owcscpy wcscpy
#else
wchar_t* Owcschr(const wchar_t* str, wchar_t c);
wchar_t* Owcsrchr(const wchar_t* str, wchar_t c);
int Owcsncmp(const wchar_t* string1, const wchar_t* string2, size_t count);
wchar_t* Owcsncpy(wchar_t* strDest, const wchar_t* strSource, size_t count);
unsigned long OGetSystemTime();
unsigned long long OGetSystemTime64();
void OSleep(unsigned long);
char* Oitoa(int value, char* str, int radix);
char* Oultoa(unsigned long value, char* str, int radix);
int Owtoi(const wchar_t* str);
int Owcslen(const wchar_t* str);
int OsprintfW(wchar_t* Buffer, int MaxChars, const wchar_t* format, ...);
double Owcstod(const wchar_t* from, wchar_t** endptr);
wchar_t* Owcscpy(wchar_t* strDestination, const wchar_t* strSource);
int Owcsnicmp(const wchar_t* string1, const wchar_t* string2, size_t count);
int Owcsicmp(const wchar_t* string1, const wchar_t* string2);
int Owcstombs(char* mbstr, const wchar_t* wcstr, size_t max);
size_t Ombstowcs(wchar_t* dest, const char* src, size_t max);
unsigned long Owcstoul(const wchar_t* nptr, wchar_t** endptr, int base);
long Owcstol(const wchar_t* nptr, wchar_t** endptr, int base);
int Owchdir(const wchar_t* dirname);
int Owmkdir(const wchar_t* dirname);
FILE* Owfopen(const wchar_t* file, const wchar_t* mode);
wchar_t* Owgetcwd(wchar_t* buffer, int maxlen);
int Owunlink(const wchar_t* filename);
unsigned long OGetThreadID();
#endif
int64_t Owcstoi64(const wchar_t* nptr, wchar_t** endptr, int base);
uint64_t Owcstoui64(const wchar_t* nptr, wchar_t** endptr, int base);
/**
 * @author	Ozrien
 * @date	10/17/2011
 * @return	nonzero if debugger is present on this application's process, 0 if not.
 * 			This is used by ics_assert to only breakpoint when debugging.
 */
int OIsDebuggerPresent(void);

#endif
