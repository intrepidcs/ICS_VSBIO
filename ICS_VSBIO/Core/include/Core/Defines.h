//File: defines.h
#ifndef _DEFINES_H
#define _DEFINES_H

#include <stdint.h>

#define ICSUNI(string) L##string
#define ICSCHAR wchar_t
#if defined(linux) || defined(QNX_OS)

typedef struct
{
	long left;
	long top;
	long right;
	long bottom;
} RECT, *LPRECT;

#endif

#ifdef _WIN32
#define NO_USING_NAMESPACE_PNGLANG
#define NO_USING_NAMESPACE_PNGIMAGE
#endif


#if defined(linux) || defined(QNX_OS)


#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define __int64 int64_t
#define DWORD unsigned int
#define WORD unsigned short
#define LPSTR char*
#define PVOID void*
#define PUCHAR unsigned char*
#define HANDLE FILE*
#define LARGE_INTEGER long int
#define ULARGE_INTEGER unsigned long int
#define BOOL bool
#define TCHAR wchar_t
#define UINT32 unsigned int
#define WINAPI
#define CALLBACK
#define _stdcall
#define __stdcall
#define BYTE unsigned char
//These match up with the fseek() parameters commented out below
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2

#define MAX_PATH 260

//      our code      what linux calls it (i.e. precompiler replaces with these in linux)
#define _wcsnicmp wcsncasecmp
#define _wcsicmp wcscasecmp
#define stricmp strcasecmp
#define strnicmp strncasecmp
#define strncmpi strncasecmp
#define strcmpi strcasecmp

#define NO_ERROR 0L
//#define SEEK_SET    0 //Beginning of the file
//#define SEEK_CUR    1
//#define SEEK_END    2


#else

#ifndef uint64
typedef unsigned __int64 uint64;
#endif

#ifndef __BORLANDC__//Windows, but not Borland
#define strncmpi _strnicmp
#endif

#endif

#if (_MSC_VER >= 1600)
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

#endif
