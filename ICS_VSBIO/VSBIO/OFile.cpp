//////////////////////////////////////////////////////////////////////
//
// OFile.cpp:
//
//////////////////////////////////////////////////////////////////////
#include <stdint.h>

#ifdef _WIN32

#include <Windows.h>
#include <stdio.h>

#include "OFile.h"

std::wstring widestring(const std::string &text)
{
	if (text.size() == 0)
		return L"";
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0);
	std::wstring strTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &text[0], (int)text.size(), &strTo[0], size_needed);
	return strTo;
}
std::string mbstring(const std::wstring &text)
{
	if (text.size() == 0)
		return "";
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &text[0], (int)text.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &text[0], (int)text.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

bool OFile::OpenFile(const char* sUtf8FileName, bool bCreateNew, bool bWritable, bool bOverlapped)
{
	m_bOverlapped = bOverlapped;
    std::wstring sFileName(widestring(sUtf8FileName));
	if (bCreateNew)
	{
    	m_bReadOnly = false;
		hFile = CreateFileW(sFileName.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, CREATE_ALWAYS,
			m_bOverlapped ? FILE_FLAG_OVERLAPPED : FILE_ATTRIBUTE_NORMAL, 0);
	}
    else
    {
	    if (!bWritable)
		    m_bReadOnly = true;
	    else
		    // if the file is read only open it for read access only and give a warning on writes
		    m_bReadOnly = (GetFileAttributesW(sFileName.c_str()) & FILE_ATTRIBUTE_READONLY) ? true : false;

	    hFile = CreateFileW(sFileName.c_str(), m_bReadOnly ? GENERIC_READ : GENERIC_READ + GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 
                FILE_ATTRIBUTE_NORMAL, 0);
	}

	return hFile == INVALID_HANDLE_VALUE ? false : true;
}

#else

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <stdio.h>
#include <inttypes.h>
#include <time.h>

#include "OFile.h"

#include <dirent.h>
#include <unistd.h>
#include <cstdlib>

#define FILE_CURRENT 1

std::wstring widestring(const std::string &text)
{
	if (text.size() == 0)
		return L"";
  std::wstring result;
  result.resize(text.length());
  std::mbstowcs(&result[0], &text[0], text.length());
  return result;
}

std::string mbstring(const std::wstring &text)
{
	if (text.size() == 0)
		return "";
  std::string result;
  result.resize(text.length());
  std::wcstombs(&result[0], &text[0], text.length());
  return result;
}

#if defined(__linux__) || defined(QNX_OS)
#if defined(ANDROID) || defined(__APPLE__)

#define _fseeki64 fseek
#define _ftelli64 ftell

#else

#define _fseeki64 fseeko64
#define _ftelli64 ftello64//ltello64

#endif
#endif

bool OFile::OpenFile(const char* sFileName, bool bCreateNew, bool bWriteable, bool bOverlapped)
{
	m_bReadOnly = bWriteable ? false : true;
	m_bOverlapped = false;// overlapped IO is a Windows feature

	if (m_fp != NULL)
		return false;//already open!
	if (bCreateNew)//if bCreateNew is true, we always create a new file and open it for reading and writing
	{
		m_fp = fopen(sFileName, "w+b");

		return m_fp == NULL ? false : true;
	}
	if (!m_bReadOnly)//opening an existing file for read and write access
	{
		//if we attempting to open an existing file (i.e. no create) and we want to open it for writing,
		//check to see if someone has it open already
		//if it is open already and is set for read-only, then set m_bReadOnly to true and only open it for reading
		m_fp = fopen(sFileName, "a+");//

		if (m_fp != NULL)
		{
			fseek(m_fp, 0, SEEK_SET);//VSpy Core always opens existing files for writing at the BEGINNING of the file
			return true;
		}

		m_bReadOnly = true;

		//now it will go below and try to open the file for read only. If it does, attempting to write will generate an error
	}

	m_fp = fopen(sFileName, "r");
	if (!m_fp)
		return false;
	fseek(m_fp, 0, SEEK_SET);
	return m_fp == NULL ? false : true;
}

#endif

bool OFile::CloseFile(void)
{
#ifdef _WIN32
	if (hFile != NULL)
    {
		CloseHandle(hFile);
		hFile = NULL;
    }
#else
	if (m_fp != NULL)
    {
		fclose(m_fp);
		m_fp = NULL;
    }
#endif
	return true;
}

bool OFile::Read(void* buffer, unsigned long bytesToRead, unsigned long& bytesRead)
{
#ifdef _WIN32
	return ReadFile(hFile, buffer, bytesToRead, &bytesRead, NULL) ? true : false;
#else
	if (m_fp)
    {
	    bytesRead = (unsigned long)fread(buffer, 1, bytesToRead, m_fp);
	    if (bytesRead == bytesToRead || bytesRead > 0)
		    return true;
    }
	return false;
#endif
}


#ifdef _WIN32
bool OFile::Write(const void* buffer, unsigned long bytesToWrite, unsigned long& bytesWritten, LPOVERLAPPED overlap)
{
	return WriteFile(hFile, buffer, bytesToWrite, &bytesWritten, overlap) ? true : false;
#else
bool OFile::Write(const void* buffer, unsigned long bytesToWrite, unsigned long& bytesWritten)
{
	if (m_fp)
    {
	    bytesWritten = (unsigned long)fwrite(buffer, 1, bytesToWrite, m_fp);
	    if (bytesWritten == bytesToWrite)
		    return true;
    }
	return false;
#endif
}

long long OFile::GetFilePtrLong()
{
	unsigned long dwCurrentFilePosition;
	long dwCurrentFilePositionHigh = 0;
	long long iFileSize;

	dwCurrentFilePosition = SetFilePtr(0, &dwCurrentFilePositionHigh, FILE_CURRENT);
	iFileSize = ((int64_t)dwCurrentFilePositionHigh << (int64_t)32);
	iFileSize += dwCurrentFilePosition;
	return iFileSize;
}


unsigned long long OFile::SetFilePtr(unsigned long long offset, unsigned long moveMethod)
{
#ifdef _WIN32
	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer(hFile, (long)li.LowPart, &li.HighPart, moveMethod);

	if ((li.LowPart == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
	{
		return 0;
	}

	return li.QuadPart;
#else
	unsigned long long iRetVal = _fseeki64(m_fp, offset, moveMethod);

	if (iRetVal == 0)
	{
		//this may be a problem from the original code. This function returns the file position after
		//moving the pointer, but if we are moving a 64 bit distance the function only returns a 32 bit position.
		//maybe nobody uses the return value and only calls the CurrPositionLarge() function separately.
		//Might be a good idea to identify the callers that need 64 bit values and write a completely different
		//function for that
		return _ftelli64(m_fp);
	}

	return 0;
#endif
}

unsigned long OFile::SetFilePtr(long distance, long* disttomovehi, unsigned long moveMethod)
{
#ifdef _WIN32
	return SetFilePointer(hFile, distance, disttomovehi, moveMethod);
#else
	int iRetVal;

	if (disttomovehi == NULL)
	{
		iRetVal = fseek(m_fp, distance, (int)moveMethod);

		if (iRetVal == 0)
			return (unsigned long)ftell(m_fp);

		return 0;
	}

	//moving a 64 bit distance
	uint64_t offset;

	offset = ((uint64_t)(*disttomovehi) << 32) + distance;
	iRetVal = _fseeki64(m_fp, offset, moveMethod);

	if (iRetVal == 0)
	{
		//this may be a problem from the original code. This function returns the file position after
		//moving the pointer, but if we are moving a 64 bit distance the function only returns a 32 bit position.
		//maybe nobody uses the return value and only calls the CurrPositionLarge() function separately.
		//Might be a good idea to identify the callers that need 64 bit values and write a completely different
		//function for that
		return (unsigned long)ftell(m_fp);
	}

	return 0;
#endif
}

unsigned long long OFile::FileSizeLarge()
{
#ifdef _WIN32
	LARGE_INTEGER stLong;
	GetFileSizeEx(hFile, &stLong);
	return stLong.QuadPart;
#else
	uint64_t CurPos = _ftelli64(m_fp);
	_fseeki64(m_fp, 0, SEEK_END);
	uint64_t Size = _ftelli64(m_fp);
	_fseeki64(m_fp, CurPos, SEEK_SET);//restore the file position
	return Size;
#endif
}

bool FileExists(const std::string& sPath)
{
#if defined(WIN32)
	DWORD dwAttrib = GetFileAttributesW(widestring(sPath).c_str());
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
#else
#ifdef __linux__
	struct stat buf;
	if (stat(sPath.c_str(), &buf) == 0)
#else
	struct stat64 buf;
	if (stat64(sPath.c_str(), &buf) == 0)
#endif
		return S_ISREG(buf.st_mode);
	return false;
#endif
}

bool IsDirectory(const std::string& sPath)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesW(widestring(sPath).c_str());
	return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY));
#else
#ifdef __linux__
	struct stat buf;
	if (stat(sPath.c_str(), &buf) == 0)
#else
	struct stat64 buf;
	if (stat64(sPath.c_str(), &buf) == 0)
#endif
		return S_ISDIR(buf.st_mode);
    return false;
#endif
}

bool RemoveFile(const std::string& sPath)
{
#ifndef linux
	return DeleteFileW(widestring(sPath).c_str()) ? true : false;
#else
	return unlink(sPath.c_str()) == 0;
#endif
}

std::string CombinePath(const std::string& sPath, const std::string& sFileName)
{
	std::string sFullPath(sPath);
	if (sFullPath.size() && sFileName.size())
    {
        if ((sFullPath[sFullPath.size() - 1] == '\\') || (sFullPath[sFullPath.size() - 1] == '/'))
        {
            if ((sFileName[0] == '\\') || (sFileName[0] == '/'))
                return sFullPath + sFileName.substr(1);
        }
        else if ((sFileName[0] != '\\') && (sFileName[0] != '/'))
            sFullPath += OS_Path_Sep;
    }
	return sFullPath + sFileName;
}

size_t GetLastSlash(const std::string& sFilePath)
{
    size_t pos = sFilePath.rfind('\\');
    if (pos == std::string::npos)
        return sFilePath.rfind('/');
    else
    {
        size_t pos2 = sFilePath.rfind('/');
        if ((pos2 == std::string::npos) || (pos > pos2))
            return pos;
        else
            return pos2;
    }
}

void SplitPath(const std::string& sFullPath, std::string& sDirectory, std::string& sFileName, std::string& sExtension)
{
    size_t pos = GetLastSlash(sFullPath);
    if (pos == std::string::npos)
    {
        sFileName = sFullPath;
        sDirectory.clear();
    }
    else
    {
        sDirectory = sFullPath.substr(0, pos);
        sFileName = sFullPath.substr(pos + 1);
    }
    size_t posDot = sFileName.find('.');
    if (posDot != std::string::npos)
    {
        sExtension = sFileName.substr(posDot);
        sFileName = sFileName.substr(0, posDot);
    }
    else
        sExtension.clear();
}

std::vector<std::string> GetFilesInDirectory(const std::string& sPath, const std::string& sExtension)
{
    std::vector<std::string> fileNames;
#if defined(WIN32)
    WIN32_FIND_DATAW fd; 
    HANDLE hFind = ::FindFirstFileW(widestring(CombinePath(sPath, "*" + sExtension)).c_str(), &fd); 
    if (hFind != INVALID_HANDLE_VALUE)
    { 
        do
        { 
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                fileNames.push_back(CombinePath(sPath, mbstring(fd.cFileName).c_str()));
        }
        while(::FindNextFileW(hFind, &fd)); 
        ::FindClose(hFind); 
    } 
#else
    std::string dirName, sName;
    size_t pos = GetLastSlash(sPath);
    if (sPath.size() && (pos == sPath.size() - 1))
        dirName = sPath.substr(0, pos);
    else
        dirName = sPath;
	DIR* dir = opendir(dirName.c_str());
	if (dir)
    {
	    struct dirent* ent = readdir(dir);
	    while (ent != NULL)
	    {
		    sName = ent->d_name;
		    if ((sName.size() > sExtension.size()) && (sName.substr(sName.size() - sExtension.size()) == sExtension))
                fileNames.push_back(CombinePath(sPath, sName));
		    ent = readdir(dir);
	    }
	    closedir(dir);
    }
#endif
    return fileNames;
}

std::string GetJson(const std::map<std::string, std::string>& strings)
{
	if (strings.size() == 0)
		return "";
	std::string json = "{\"attr\":[";
	bool bFirst = true;
	for (std::map<std::string, std::string>::const_iterator it = strings.begin(); it != strings.end(); ++it)
	{
		if (bFirst)
			bFirst = !bFirst;
		else
			json += ",";
		json += "{\"" + it->first + "\":\"" + it->second + "\"}";
	}
	json += "]}";
	return json;
}

std::list<std::string> split(const char* str, char c)
{
	std::list<std::string> result;

	do
	{
		const char* begin = str;

		while ((*str != c) && *str)
			str++;

		result.push_back(std::string(begin, str));
	} while (0 != *str++);

	return result;
}

