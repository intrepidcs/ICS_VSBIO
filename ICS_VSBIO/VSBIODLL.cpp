// VSBIODLL.cpp : Defines the exported functions for the DLL application.
//

#include "VSBIODLL.h"
#include "VSBIO/VSBIO.h"
#include <string>
#include <locale>

using namespace std;
#ifndef _WIN32
#include <dirent.h>

wstring widestring(const string &text)
{
	if (text.size() == 0)
		return L"";
  wstring result;
  result.resize(text.length());
  mbstowcs(&result[0], &text[0], text.length());
  return result;
}

string mbstring(const wstring &text)
{
	if (text.size() == 0)
		return "";
  string result;
  result.resize(text.length());
  wcstombs(&result[0], &text[0], text.length());
  return result;
}
#else
wstring widestring(const string &text)
{
	if (text.size() == 0)
		return L"";
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);
	std::wstring strTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &strTo[0], size_needed);
	return strTo;
}
string mbstring(const wstring &text)
{
	if (text.size() == 0)
		return "";
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &text[0], (int)text.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}
#endif

extern "C" VSBIODLL_API ReadHandle ReadVSBW(const wchar_t * filename)
{
	return new VSBIORead(filename);
}

extern "C" VSBIODLL_API ReadHandle ReadVSB(const char * filename)
{
	return new VSBIORead(widestring(filename).c_str());
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

std::string ws2s(const std::wstring& wstr)
{
    return std::string(wstr.begin(), wstr.end());
}

extern "C" VSBIODLL_API const char * GetDisplayMessage(ReadHandle handle)
{
	std::wstring const& wstr = ((VSBIORead *)handle)->GetDisplayMessage();
	static std::string str;
	str = ws2s(wstr);
	return str.c_str();
}
extern "C" VSBIODLL_API const char * GetErrorMessage(ReadHandle handle)
{
	std::wstring const& wstr = ((VSBIORead *)handle)->GetErrorMessage();
	static std::string str;
	str = ws2s(wstr);
	return str.c_str();
}

extern "C" VSBIODLL_API WriteHandle WriteVSB(const char * filename)
{
	return WriteVSBW(widestring(filename).c_str());
}

extern "C" VSBIODLL_API WriteHandle WriteVSBW(const wchar_t * filename)
{
	VSBIOWrite * write = new VSBIOWrite();

	if (write->Init(filename))
		return write;
	delete write;
	return NULL;
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



