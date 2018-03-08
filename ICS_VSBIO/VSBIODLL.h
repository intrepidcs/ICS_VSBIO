#pragma once

#if defined(_WINDLL)
#	ifdef VSBIODLL_EXPORTS
#		ifdef PYD
#			define VSBIODLL_API __declspec(dllexport) 
#		else
#			define VSBIODLL_API __declspec(dllexport) 
#		endif
#	else
#		define VSBIODLL_API __declspec(dllimport) 
#	endif
#elif defined(LINUXSO)
#	define VSBIODLL_API 
#elif SWIG
#	define VSBIODLL_API extern
#else
#	define VSBIODLL_API 
#endif

typedef void * ReadHandle;
typedef void * WriteHandle;

#ifndef SWIG
#include "VSBIO/VSBStruct.h"
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

VSBIODLL_API ReadHandle ReadVSBW(const wchar_t * filename);
VSBIODLL_API ReadHandle ReadVSB(const char * filename);
VSBIODLL_API int ReadNextMessage(ReadHandle handle, icsSpyMessageVSB * message, unsigned int size, unsigned int * lengthOfMessageReturned);
VSBIODLL_API void ReadClose(ReadHandle handle);

VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename);
VSBIODLL_API WriteHandle WriteVSB(const char * filename);
VSBIODLL_API int WriteMessage(WriteHandle handle, icsSpyMessageVSB * message, unsigned int size);
VSBIODLL_API void WriteClose(WriteHandle handle);

VSBIODLL_API void VSBIOFree(icsSpyMessageVSB * message);
VSBIODLL_API icsSpyMessageVSB * VSBIOMalloc(int nBytes);

#ifdef __cplusplus
}
#endif

