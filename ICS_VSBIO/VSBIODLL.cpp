// VSBIODLL.cpp : Defines the exported functions for the DLL application.
//

#include "VSBIODLL.h"
#include "VSBIO/VSBIO.h"
#include <string>
#include <locale>

using namespace std;

extern "C" VSBIODLL_API ReadHandle ReadVSBW(const wchar_t * filename)
{
	return new VSBIORead(mbstring(filename).c_str());
}

extern "C" VSBIODLL_API ReadHandle ReadVSB(const char * filename)
{
	return new VSBIORead(filename);
}

extern "C" VSBIODLL_API int ReadNextMessage(ReadHandle handle, icsSpyMessageVSB * message, unsigned int size, unsigned int * lengthOfMessageReturned)
{
	return ((VSBIORead *)handle)->ReadNextMessage((unsigned char *)message, (size_t)size, (size_t *)lengthOfMessageReturned);
}

extern "C" VSBIODLL_API void ReadClose(ReadHandle handle)
{
	delete ((VSBIORead *)handle);
}

extern "C" VSBIODLL_API int GetProgress(ReadHandle handle)
{
	return ((VSBIORead *)handle)->GetProgress();
}

extern "C" VSBIODLL_API const char * GetDisplayMessage(ReadHandle handle)
{
	std::string const& str = ((VSBIORead *)handle)->GetDisplayMessage();
	return str.c_str();
}
extern "C" VSBIODLL_API const char * GetErrorMessage(ReadHandle handle)
{
	std::string const& str = ((VSBIORead *)handle)->GetErrorMessage();
	return str.c_str();
}

extern "C" VSBIODLL_API WriteHandle WriteVSB(const char * filename)
{
	VSBIOWrite * write = new VSBIOWrite();

	if (write->Init(filename))
		return write;
	delete write;
	return NULL;
}

extern "C" VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename)
{
	return WriteVSB(mbstring(filename).c_str());
}

extern "C" VSBIODLL_API int WriteMessage(WriteHandle handle, icsSpyMessageVSB * message, unsigned int size)
{
	return ((VSBIOWrite *)handle)->WriteMessage((unsigned char *)message, (size_t)size);
}

extern "C" VSBIODLL_API void WriteClose(WriteHandle handle)
{
	delete ((VSBIOWrite *)handle);
}

extern "C" VSBIODLL_API void VSBIOFree(icsSpyMessageVSB * message)
{
	delete (unsigned char * )message;
}

extern "C" VSBIODLL_API icsSpyMessageVSB * VSBIOMalloc(int nBytes)
{
	return (icsSpyMessageVSB *)new unsigned char[nBytes];
}

extern "C" VSBIODLL_API bool ConcatenateW(const wchar_t * inputFilePath, const wchar_t * outputFileName, ProgressFunc prog)
{
    return Concatenate(mbstring(inputFilePath).c_str(), mbstring(outputFileName).c_str(), prog);
}

extern "C" VSBIODLL_API bool Concatenate(const char * inputFilePath, const char * outputFileName, ProgressFunc prog)
{
    VSBIOWrite write;
    write.Init(outputFileName);
    return write.ConcatenateFromDirectory(inputFilePath, prog);
}

extern "C" VSBIODLL_API bool SplitW(const wchar_t *sFileName, unsigned int nMessagesPerFile, const wchar_t *OutputLocation, ProgressFunc prog)
{
    return Split(mbstring(sFileName).c_str(), nMessagesPerFile, mbstring(OutputLocation).c_str(), prog);
}

extern "C" VSBIODLL_API bool Split(const char *sFileName, unsigned int nMessagesPerFile, const char *OutputLocation, ProgressFunc prog)
{
    VSBIORead read(sFileName);
    return read.Split(nMessagesPerFile, OutputLocation, prog);
}



