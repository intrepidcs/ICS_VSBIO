// VSBIODLL.cpp : Defines the exported functions for the DLL application.
//

#include "VSBIODLL.h"
#include "VSBIO/VSBIO.h"
#include "VSBIO/VSBDB.h"
#include <string>
#include <locale>

using namespace std;

/// <summary>
/// Opens a vsb file for read
/// </summary>
/// <param name="filename">Full pPath of the file to read</param>
/// <returns>The file handle</returns>
extern "C" VSBIODLL_API ReadHandle ReadVSBW(const wchar_t * filename)
{
	return new VSBIORead(mbstring(filename).c_str());
}

/// <summary>
/// Opens a vsb file for read
/// </summary>
/// <param name="filename">Full pPath of the file to read</param>
/// <returns>The file handle</returns>
extern "C" VSBIODLL_API ReadHandle ReadVSB(const char * filename)
{
	return new VSBIORead(filename);
}

/// <summary>
/// Reads the next message from the file
/// </summary>
/// <param name="handle">The file handle to use</param>
/// <param name="message">Message to read</param>
/// <param name="size">Allocated message size</param>
/// <param name="lengthOfMessageReturned">Message size returned</param>
/// <returns>Whether the message was read</returns>
extern "C" VSBIODLL_API int ReadNextMessage(ReadHandle handle, icsSpyMessageVSB * message, unsigned int size, unsigned int * lengthOfMessageReturned)
{
	return ((VSBIORead *)handle)->ReadNextMessage((unsigned char *)message, (size_t)size, (size_t *)lengthOfMessageReturned);
}

/// <summary>
/// Closes the file
/// </summary>
/// <param name="handle">Handle of the file to close</param>
extern "C" VSBIODLL_API void ReadClose(ReadHandle handle)
{
	delete ((VSBIORead *)handle);
}

/// <summary>
/// Return the progress value (0-100)
/// </summary>
/// <param name="handle">Handle to the open file</param>
/// <returns>Progress (0-100)</returns>
extern "C" VSBIODLL_API int GetProgress(ReadHandle handle)
{
	return ((VSBIORead *)handle)->GetProgress();
}

/// <summary>
/// Returns info about the current file processed
/// </summary>
/// <param name="handle">Handle to the open file</param>
/// <returns>Display message</returns>
extern "C" VSBIODLL_API const char * GetDisplayMessage(ReadHandle handle)
{
	std::string const& str = ((VSBIORead *)handle)->GetDisplayMessage();
	return str.c_str();
}

/// <summary>
/// Returns the the UTC timestamp for the given message
/// </summary>
/// <param name="message">Message to use</param>
/// <returns>Number of seconds since midnight Jan 1, 2007</returns>
extern "C" VSBIODLL_API double GetMsgTime(icsSpyMessageVSB * message)
{
	return CMessageTimeDecoderVSB::CalcTimeStamp(*message);
}

/// <summary>
/// Returns the error message string for the file handle
/// </summary>
/// <param name="handle">File handle</param>
/// <returns>Error message</returns>
extern "C" VSBIODLL_API const char * GetErrorMessage(ReadHandle handle)
{
	std::string const& str = ((VSBIORead *)handle)->GetErrorMessage();
	return str.c_str();
}

/// <summary>
/// Opens a vsb file for write
/// </summary>
/// <param name="filename">Full pPath of the file to write</param>
/// <returns>The file handle</returns>
extern "C" VSBIODLL_API WriteHandle WriteVSB(const char * filename)
{
	VSBIOWrite * write = new VSBIOWrite();

	if (write->Init(filename))
		return write;
	delete write;
	return NULL;
}

/// <summary>
/// Opens a vsb file for write
/// </summary>
/// <param name="filename">Full pPath of the file to write</param>
/// <returns>The file handle</returns>
extern "C" VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename)
{
	return WriteVSB(mbstring(filename).c_str());
}

/// <summary>
/// Writes a message to the file
/// </summary>
/// <param name="handle">File handle to write to</param>
/// <param name="message">Message to write</param>
/// <param name="size">Message size</param>
/// <returns>Whether the write was successful</returns>
extern "C" VSBIODLL_API int WriteMessage(WriteHandle handle, icsSpyMessageVSB * message, unsigned int size)
{
	return ((VSBIOWrite *)handle)->WriteMessage((unsigned char *)message, (size_t)size);
}

/// <summary>
/// Closes the file
/// </summary>
/// <param name="handle">Handle of the file to close</param>
extern "C" VSBIODLL_API void WriteClose(WriteHandle handle)
{
	delete ((VSBIOWrite *)handle);
}

/// <summary>
/// Frees the memory allocated with VSBIOMalloc
/// </summary>
/// <param name="message">Buffer to free</param>
extern "C" VSBIODLL_API void VSBIOFree(icsSpyMessageVSB * message)
{
	delete (unsigned char * )message;
}

/// <summary>
/// Allocates memory for the message
/// </summary>
/// <param name="nBytes">Number of bytes to allocate</param>
/// <returns>The allocated buffer</returns>
extern "C" VSBIODLL_API icsSpyMessageVSB * VSBIOMalloc(int nBytes)
{
	return (icsSpyMessageVSB *)new unsigned char[nBytes];
}

/// <summary>
/// Concatenates the vsb files found in the provided directory, sorted by name
/// </summary>
/// <param name="sInputFilePath">Directory where the input files reside</param>
/// <param name="outputFileName">Output path for the concatenated file</param>
/// <param name="prog">Progress return</param>
/// <returns>Whether the files were concatenated successfully</returns>
extern "C" VSBIODLL_API bool ConcatenateW(const wchar_t * inputFilePath, const wchar_t * outputFileName, ProgressFunc prog)
{
    return Concatenate(mbstring(inputFilePath).c_str(), mbstring(outputFileName).c_str(), prog);
}

/// <summary>
/// Concatenates the vsb files found in the provided directory, sorted by name
/// </summary>
/// <param name="sInputFilePath">Directory where the input files reside</param>
/// <param name="outputFileName">Output path for the concatenated file</param>
/// <param name="prog">Progress return</param>
/// <returns>Whether the files were concatenated successfully</returns>
extern "C" VSBIODLL_API bool Concatenate(const char * inputFilePath, const char * outputFileName, ProgressFunc prog)
{
    VSBIOWrite write;
    write.Init(outputFileName);
    return write.ConcatenateFromDirectory(inputFilePath, prog);
}

/// <summary>
/// Splits this file using the messages per file parameter and a _00000... suffix for the output files.
/// </summary>
/// <param name="sFileName">Field to split</param>
/// <param name="nMessagesPerFile">Number of messages to split on</param>
/// <param name="sOutputLocation">Output path</param>
/// <param name="prog">Progress callback</param>
/// <returns>Whether the output files were all written</returns>
extern "C" VSBIODLL_API bool SplitW(const wchar_t *sFileName, unsigned int nMessagesPerFile, const wchar_t *sOutputLocation, ProgressFunc prog)
{
    return Split(mbstring(sFileName).c_str(), nMessagesPerFile, mbstring(sOutputLocation).c_str(), prog);
}

/// <summary>
/// Splits this file using the messages per file parameter and a _00000... suffix for the output files.
/// </summary>
/// <param name="sFileName">Field to split</param>
/// <param name="nMessagesPerFile">Number of messages to split on</param>
/// <param name="sOutputLocation">Output path</param>
/// <param name="prog">Progress callback</param>
/// <returns>Whether the output files were all written</returns>
extern "C" VSBIODLL_API bool Split(const char *sFileName, unsigned int nMessagesPerFile, const char *sOutputLocation, ProgressFunc prog)
{
    VSBIORead read(sFileName);
    return read.Split(nMessagesPerFile, sOutputLocation, prog);
}

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
extern "C" VSBIODLL_API bool CreateDatabaseW(const wchar_t* inputFilePath, const wchar_t* outputFileName, ProgressFunc prog)
{
	return CreateDatabase(mbstring(inputFilePath).c_str(), mbstring(outputFileName).c_str(), prog);
}

/// <summary>
/// Creates a Sqlite database containing all the messages in the vsb file.
/// </summary>
/// <param name="inputFilePath">File for which to generate a database</param>
/// <param name="outputFileName">File to generate</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the database was created</returns>
extern "C" VSBIODLL_API bool CreateDatabase(const char* inputFilePath, const char* outputFileName, ProgressFunc prog)
{
	return CreateDb(inputFilePath, outputFileName, prog);
}

/// <summary>
/// Creates a vsb file from the given Sqlite message database.
/// The filter is basically the WHERE clause, which can restrict messages by columns such as MessageTime, NetworkId, Id, etc
/// Please consult the database schema for details.
/// </summary>
/// <param name="inputFilePath">Database from which the messages will be read</param>
/// <param name="outputFileName">Output file containing filtered messages</param>
/// <param name="pFilter">The WHERE clause, which can be used to filter a subset of the messages</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the VSB file was created</returns>
extern "C" VSBIODLL_API bool WriteFilteredVsbW(const wchar_t* inputFilePath, const wchar_t* outputFileName, const wchar_t* filter,
	ProgressFunc prog)
{
	return WriteFilteredVsb(mbstring(inputFilePath).c_str(), mbstring(outputFileName).c_str(), mbstring(filter).c_str(), prog);
}

/// <summary>
/// Creates a vsb file from the given Sqlite message database.
/// The filter is basically the WHERE clause, which can restrict messages by columns such as MessageTime, NetworkId, Id, etc
/// Please consult the database schema for details.
/// </summary>
/// <param name="inputFilePath">Database from which the messages will be read</param>
/// <param name="outputFileName">Output file containing filtered messages</param>
/// <param name="pFilter">The WHERE clause, which can be used to filter a subset of the messages</param>
/// <param name="prog">Progess callback</param>
/// <returns>Whether the VSB file was created</returns>
extern "C" VSBIODLL_API bool WriteFilteredVsb(const char* inputFilePath, const char* outputFileName, const char *filter,
	ProgressFunc prog)
{
	return WriteVsb(inputFilePath, outputFileName, filter, prog);
}


