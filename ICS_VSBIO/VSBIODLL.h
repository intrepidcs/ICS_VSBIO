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

/// <summary>
/// Opens a vsb file for read
/// </summary>
/// <param name="filename">Full pPath of the file to read</param>
/// <returns>The file handle</returns>
VSBIODLL_API ReadHandle ReadVSBW(const wchar_t * filename);

/// <summary>
/// Opens a vsb file for read
/// </summary>
/// <param name="filename">Full pPath of the file to read</param>
/// <returns>The file handle</returns>
VSBIODLL_API ReadHandle ReadVSB(const char * filename);

/// <summary>
/// Reads the next message from the file
/// </summary>
/// <param name="handle">The file handle to use</param>
/// <param name="message">Message to read</param>
/// <param name="size">Allocated message size</param>
/// <param name="lengthOfMessageReturned">Message size returned</param>
/// <returns>Whether the message was read</returns>
VSBIODLL_API int ReadNextMessage(ReadHandle handle, icsSpyMessageVSB * message, unsigned int size, unsigned int * lengthOfMessageReturned);

/// <summary>
/// Closes the file
/// </summary>
/// <param name="handle">Handle of the file to close</param>
VSBIODLL_API void ReadClose(ReadHandle handle);

/// <summary>
/// Return the progress value (0-100)
/// </summary>
/// <param name="handle">Handle to the open file</param>
/// <returns>Progress (0-100)</returns>
VSBIODLL_API int GetProgress(ReadHandle handle);

/// <summary>
/// Returns info about the current file processed
/// </summary>
/// <param name="handle">Handle to the open file</param>
/// <returns>Display message</returns>
VSBIODLL_API const char * GetDisplayMessage(ReadHandle handle);

/// <summary>
/// Returns the the UTC timestamp for the given message
/// </summary>
/// <param name="message">Message to use</param>
/// <returns>Number of seconds since midnight Jan 1, 2007</returns>
VSBIODLL_API double GetMsgTime(icsSpyMessageVSB * message);

/// <summary>
/// Returns the error message string for the file handle
/// </summary>
/// <param name="handle">File handle</param>
/// <returns>Error message</returns>
VSBIODLL_API const char * GetErrorMessage(ReadHandle handle);

/// <summary>
/// Opens a vsb file for write
/// </summary>
/// <param name="filename">Full pPath of the file to write</param>
/// <returns>The file handle</returns>
VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename);

/// <summary>
/// Opens a vsb file for write
/// </summary>
/// <param name="filename">Full pPath of the file to write</param>
/// <returns>The file handle</returns>
VSBIODLL_API WriteHandle WriteVSB(const char * filename);

/// <summary>
/// Writes a message to the file
/// </summary>
/// <param name="handle">File handle to write to</param>
/// <param name="message">Message to write</param>
/// <param name="size">Message size</param>
/// <returns>Whether the write was successful</returns>
VSBIODLL_API int WriteMessage(WriteHandle handle, icsSpyMessageVSB * message, unsigned int size);

/// <summary>
/// Closes the file
/// </summary>
/// <param name="handle">Handle of the file to close</param>
VSBIODLL_API void WriteClose(WriteHandle handle);

/// <summary>
/// Frees the memory allocated with VSBIOMalloc
/// </summary>
/// <param name="message">Buffer to free</param>
VSBIODLL_API void VSBIOFree(icsSpyMessageVSB * message);

/// <summary>
/// Allocates memory for the message
/// </summary>
/// <param name="nBytes">Number of bytes to allocate</param>
/// <returns>The allocated buffer</returns>
VSBIODLL_API icsSpyMessageVSB * VSBIOMalloc(int nBytes);

/// <summary>
/// Concatenates the vsb files found in the provided directory, sorted by name
/// </summary>
/// <param name="sInputFilePath">Directory where the input files reside</param>
/// <param name="outputFileName">Output path for the concatenated file</param>
/// <param name="prog">Progress return</param>
/// <returns>Whether the files were concatenated successfully</returns>
VSBIODLL_API bool ConcatenateW(const wchar_t * inputFileList, const wchar_t * outputFileName, ProgressFunc prog);

/// <summary>
/// Concatenates the vsb files found in the provided directory, sorted by name
/// </summary>
/// <param name="sInputFilePath">Directory where the input files reside</param>
/// <param name="outputFileName">Output path for the concatenated file</param>
/// <param name="prog">Progress return</param>
/// <returns>Whether the files were concatenated successfully</returns>
VSBIODLL_API bool Concatenate(const char * inputFileList, const char * outputFileName, ProgressFunc prog);

/// <summary>
/// Splits this file using the messages per file parameter and a _00000... suffix for the output files.
/// </summary>
/// <param name="sFileName">Field to split</param>
/// <param name="nMessagesPerFile">Number of messages to split on</param>
/// <param name="sOutputLocation">Output path</param>
/// <param name="prog">Progress callback</param>
/// <returns>Whether the output files were all written</returns>
VSBIODLL_API bool SplitW(const wchar_t *sFileName, unsigned int nMessagesPerFile, const wchar_t *OutputLocation, ProgressFunc prog);

/// <summary>
/// Splits this file using the messages per file parameter and a _00000... suffix for the output files.
/// </summary>
/// <param name="sFileName">Field to split</param>
/// <param name="nMessagesPerFile">Number of messages to split on</param>
/// <param name="sOutputLocation">Output path</param>
/// <param name="prog">Progress callback</param>
/// <returns>Whether the output files were all written</returns>
VSBIODLL_API bool Split(const char *sFileName, unsigned int nMessagesPerFile, const char *OutputLocation, ProgressFunc prog);

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.  Old data is removed.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
VSBIODLL_API bool CreateDatabaseW(const wchar_t* inputFilePath, const wchar_t* outputFileName, ProgressFunc prog);

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.  Old data is removed.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
VSBIODLL_API bool CreateDatabase(const char* inputFilePath, const char* outputFileName, ProgressFunc prog);

/// <summary>
/// Appends to or creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
VSBIODLL_API bool AddToDatabaseW(const wchar_t* inputFilePath, const wchar_t* outputFileName, ProgressFunc prog);

/// <summary>
/// Appends to or creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
VSBIODLL_API bool AddToDatabase(const char* inputFilePath, const char* outputFileName, ProgressFunc prog);

/// <summary>
/// Creates a vsb file from the given Sqlite message database.
/// The filter is basically the WHERE clause, which can restrict messages by columns such as MessageTime, NetworkId, Id, etc
/// Please consult the database schema for details.
/// </summary>
/// <param name="inputFilePath">Database from which the messages will be read</param>
/// <param name="outputFileName">Output file containing filtered messages</param>
/// <param name="pFilter">The WHERE clause, which can be used to filter a subset of the messages</param>
/// <param name="prog">Progess callback</param>
/// <returns>The number of records written or -1 if an error occurred</returns>
VSBIODLL_API int WriteFilteredVsbW(const wchar_t* inputFilePath, const wchar_t* outputFileName, const wchar_t* filter, ProgressFunc prog);

/// <summary>
/// Creates a vsb file from the given Sqlite message database.
/// The filter is basically the WHERE clause, which can restrict messages by columns such as MessageTime, NetworkId, Id, etc
/// Please consult the database schema for details.
/// </summary>
/// <param name="inputFilePath">Database from which the messages will be read</param>
/// <param name="outputFileName">Output file containing filtered messages</param>
/// <param name="pFilter">The WHERE clause, which can be used to filter a subset of the messages</param>
/// <param name="prog">Progess callback</param>
/// <returns>The number of records written or -1 if an error occurred</returns>
VSBIODLL_API int WriteFilteredVsb(const char* inputFilePath, const char* outputFileName, const char* filter, ProgressFunc prog);


#ifdef __cplusplus
}
#endif

