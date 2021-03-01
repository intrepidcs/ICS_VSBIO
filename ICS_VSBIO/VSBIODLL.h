#pragma once

#if SWIG
#	define VSBIODLL_API extern
#elif defined(_WINDLL)
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
VSBIODLL_API void ReadClose(ReadHandle handle);

VSBIODLL_API int GetProgress(ReadHandle handle);
VSBIODLL_API const char * GetDisplayMessage(ReadHandle handle);
VSBIODLL_API const char * GetErrorMessage(ReadHandle handle);

VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename);
VSBIODLL_API WriteHandle WriteVSB(const char * filename);
VSBIODLL_API int WriteMessage(WriteHandle handle, icsSpyMessageVSB * message, unsigned int size);
VSBIODLL_API void WriteClose(WriteHandle handle);

VSBIODLL_API void VSBIOFree(icsSpyMessageVSB * message);
VSBIODLL_API icsSpyMessageVSB * VSBIOMalloc(int nBytes);

VSBIODLL_API bool ConcatenateW(const wchar_t * inputFileList, const wchar_t * outputFileName, ProgressFunc prog);
VSBIODLL_API bool Concatenate(const char * inputFileList, const char * outputFileName, ProgressFunc prog);
VSBIODLL_API bool SplitW(const wchar_t *sFileName, unsigned int nMessagesPerFile, const wchar_t *OutputLocation, ProgressFunc prog);
VSBIODLL_API bool Split(const char *sFileName, unsigned int nMessagesPerFile, const char *OutputLocation, ProgressFunc prog);

#ifdef __cplusplus
}
#endif

