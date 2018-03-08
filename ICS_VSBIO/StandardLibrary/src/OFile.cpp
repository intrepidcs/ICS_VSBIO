//////////////////////////////////////////////////////////////////////
//
// OFile.cpp:
//
//////////////////////////////////////////////////////////////////////
#include <stdint.h>

#ifdef _WIN32

#include <Windows.h>
#include <stdio.h>

#include "StandardLibrary/OFile.h"


OFile::OFile()
{
	m_bReadOnly = false;
	hFile = NULL;
}

OFile::~OFile()
{
	if (hFile != NULL)
		CloseHandle(hFile);
}

bool OFile::OpenFile(const wchar_t* sFileName, bool bCreateNew, bool bWritable, bool bOverlapped)
{
	unsigned long DesiredAccess;
	m_bReadOnly = false;
	m_bOverlapped = bOverlapped;

	if (bCreateNew)
	{
		hFile = CreateFileW(sFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, 0, CREATE_ALWAYS,
			m_bOverlapped ? FILE_FLAG_OVERLAPPED : FILE_ATTRIBUTE_NORMAL, 0);

		return hFile == INVALID_HANDLE_VALUE ? false : true;
	}


	if (!bWritable)
		m_bReadOnly = true;
	else
	{
		// if the file is read only open it for read access only and give a warning on writes
		m_bReadOnly = (GetFileAttributesW(sFileName) & FILE_ATTRIBUTE_READONLY) ? true : false;
	}

	DesiredAccess = m_bReadOnly ? GENERIC_READ : GENERIC_READ + GENERIC_WRITE;

	hFile = CreateFileW(sFileName, DesiredAccess, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	return hFile == INVALID_HANDLE_VALUE ? false : true;
}

bool OFile::CloseFile(void)
{
	if (hFile)
	{
		CloseHandle(hFile);//WIN_CALL
		hFile = NULL;
	}

	return true;
}

bool OFile::Read(void* buffer, unsigned long bytesToRead, unsigned long& bytesRead)
{
	return ReadFile(hFile, buffer, bytesToRead, &bytesRead, NULL) ? true : false;
}


bool OFile::Write(const void* buffer, unsigned long bytesToWrite, unsigned long& bytesWritten, LPOVERLAPPED overlap)
{
	return WriteFile(hFile, buffer, bytesToWrite, &bytesWritten, overlap);
}

unsigned long long OFile::SetFilePtr(unsigned long long offset, unsigned long moveMethod)
{
	LARGE_INTEGER li;
	li.QuadPart = offset;
	li.LowPart = SetFilePointer(hFile, (long)li.LowPart, &li.HighPart, moveMethod);

	if ((li.LowPart == INVALID_SET_FILE_POINTER) && (GetLastError() != NO_ERROR))
	{
		return 0;
	}

	return li.QuadPart;
}

unsigned long OFile::SetFilePtr(long distance, long* disttomovehi, unsigned long moveMethod)
{
	return SetFilePointer(hFile, distance, disttomovehi, moveMethod);
}

bool OFile::FlushData()
{
	return FlushFileBuffers(hFile) ? true : false;
}


int OFile::FileSize()
{
	unsigned long iSize;

	iSize = GetFileSize(hFile, NULL);

	return iSize;
}


unsigned long long OFile::FileSizeLarge()
{
	LARGE_INTEGER stLong;

	GetFileSizeEx(hFile, &stLong);//WIN_CALL

	return stLong.QuadPart;
}


bool OFile::GetFileTimeDateModified(int& iDay, int& iMonth, int& iYear, int& iHour, int& iMin, int& iSecond)
{
	SYSTEMTIME stTime;
	FILETIME stFTime;

	if (!GetFileTime(hFile, NULL, NULL, &stFTime) || !FileTimeToSystemTime(&stFTime, &stTime))//WIN_CALL
		return false;

	iDay = stTime.wDay;
	iMonth = stTime.wMonth;
	iYear = stTime.wYear;
	iHour = stTime.wHour;
	iMin = stTime.wMinute;
	iSecond = stTime.wSecond;

	return true;
}


#else

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Core/Defines.h"
#include "StandardLibrary/OFile.h"
#include "StandardLibrary/OSAbstraction.h"

#if defined(linux) || defined(QNX_OS)

#if defined(ANDROID) || defined(__APPLE__)

#define _fseeki64 fseek
#define _ftelli64 ftell

#else

#define _fseeki64 fseeko64
#define _ftelli64 ftello64//ltello64

#endif


#endif

OFile::OFile()
{
	m_bReadOnly = false;
	m_fp = NULL;
}


OFile::~OFile()
{
	if (m_fp != NULL)
		fclose(m_fp);
}


bool OFile::OpenFile(const wchar_t* sFileName, bool bCreateNew, bool bWriteable, bool bOverlapped)
{
	m_bReadOnly = bWriteable ? false : true;
	m_bOverlapped = false;// overlapped IO is a Windows feature

	if (m_fp != NULL)
		return false;//already open!
	if (bCreateNew)//if bCreateNew is true, we always create a new file and open it for reading and writing
	{
		m_fp = Owfopen(sFileName, L"w+b");

		return m_fp == NULL ? false : true;
	}
	if (!m_bReadOnly)//opening an existing file for read and write access
	{
		//if we attempting to open an existing file (i.e. no create) and we want to open it for writing,
		//check to see if someone has it open already
		//if it is open already and is set for read-only, then set m_bReadOnly to true and only open it for reading
		m_fp = Owfopen(sFileName, L"a+");//

		if (m_fp != NULL)
		{
			fseek(m_fp, 0, SEEK_SET);//VSpy Core always opens existing files for writing at the BEGINNING of the file
			return true;
		}

		m_bReadOnly = true;

		//now it will go below and try to open the file for read only. If it does, attempting to write will generate an error
	}

	m_fp = Owfopen(sFileName, L"r");
	if (!m_fp)
		return false;
	fseek(m_fp, 0, SEEK_SET);
	return m_fp == NULL ? false : true;
}


bool OFile::CloseFile(void)
{
	if (m_fp != NULL)
	{
		fclose(m_fp);
		m_fp = NULL;
	}

	return true;
}

bool OFile::Read(void* buffer, unsigned long bytesToRead, unsigned long& bytesRead)
{
	if (!m_fp)
		return false;
	bytesRead = (unsigned long)fread(buffer, 1, bytesToRead, m_fp);

	if (bytesRead == bytesToRead || bytesRead > 0)
		return true;

	return false;
}


bool OFile::Write(const void* buffer, unsigned long bytesToWrite, unsigned long& bytesWritten)
{
	if (!m_fp)
		return false;
	bytesWritten = (unsigned long)fwrite(buffer, 1, bytesToWrite, m_fp);

	if (bytesWritten == bytesToWrite)
		return true;

	return false;
}

//windows file i/o
//FILE_BEGIN           0
//FILE_CURRENT         1
//FILE_END             2

//std c lib
//SEEK_SET             0
//SEEK_CUR             1
//SEEK_END             2
unsigned long OFile::SetFilePtr(long distance, long* disttomovehi, unsigned long moveMethod)
{
	int iRetVal;

	if (disttomovehi == NULL)
	{
		iRetVal = fseek(m_fp, distance, (int)moveMethod);

		if (iRetVal == 0)
			return (unsigned long)ftell(m_fp);

		return false;
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

	return false;
}

unsigned long long OFile::SetFilePtr(unsigned long long offset, unsigned long moveMethod)
{
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
}

bool OFile::FlushData(void)
{
	return fflush(m_fp) == 0 ? true : false;
}


int OFile::FileSize(void)
{
	unsigned long CurPos;
	long iSize;

	CurPos = ftell(m_fp);

	fseek(m_fp, 0, SEEK_END);
	iSize = ftell(m_fp);
	fseek(m_fp, CurPos, SEEK_SET);//restore the file position

	return iSize;
}


unsigned long long OFile::FileSizeLarge(void)
{
	uint64_t CurPos;
	uint64_t Size;


	CurPos = _ftelli64(m_fp);
	_fseeki64(m_fp, 0, SEEK_END);
	Size = _ftelli64(m_fp);
	_fseeki64(m_fp, CurPos, SEEK_SET);//restore the file position

	return Size;
}


//have to figure this out out later. Might be a linux system call that takes file name or stdlib call I'm not seeing
bool OFile::GetFileTimeDateModified(int& iDay, int& iMonth, int& iYear, int& iHour, int& iMin, int& iSecond)
{
	unsigned long size;
	struct stat Status_buff;
	int fd = fileno(m_fp);

	if (fd < 0)
	{
		perror("No File");
		return false;
	}

	if (fstat(fd, &Status_buff) != 0)
		return false;


	tm* systime = localtime((const time_t*)&Status_buff.st_mtime);

	iDay = systime->tm_wday;
	iMonth = systime->tm_mon + 1;
	iYear = systime->tm_year + 1900;
	iHour = systime->tm_hour;
	iMin = systime->tm_min;
	iSecond = systime->tm_sec;

	return true;
}

#endif

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

int OFile::GetFilePtr()
{
	DWORD dwCurrentFilePosition;
	dwCurrentFilePosition = SetFilePtr(0, NULL, FILE_CURRENT);
	return dwCurrentFilePosition;
}
