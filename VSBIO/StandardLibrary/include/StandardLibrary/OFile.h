#ifndef _OFILE_H
#define _OFILE_H


#ifdef _WIN32
#include <Windows.h>
#endif

class OFile
{
public:
	OFile();
	~OFile();

	bool OpenFile(const wchar_t* sFileName, bool bCreateNew, bool bWriteable, bool bOverlapped = false);
	bool CloseFile(void);

	bool Read(void* buffer, unsigned long bytesToRead, unsigned long& bytesRead);
	bool Write(const void* buffer,
		unsigned long bytesToWrite,
		unsigned long& bytesWritten
#ifdef _WIN32
		,
		LPOVERLAPPED = NULL
#endif
		);
	bool FlushData(void);

	unsigned long SetFilePtr(long distance, long* disttomovehi, unsigned long moveMethod);
	unsigned long long SetFilePtr(unsigned long long offset, unsigned long moveMethod);

	int GetFilePtr();
	long long GetFilePtrLong();

	int FileSize(void);
	unsigned long long FileSizeLarge(void);

	bool GetFileTimeDateModified(int& iDay, int& iMonth, int& iYear, int& iHour, int& iMin, int& iSecond);

#ifdef _WIN32
	HANDLE GetHandle() { return hFile; }
#endif

private:
	bool m_bReadOnly;
	bool m_bOverlapped;
#ifdef _WIN32
	HANDLE hFile;
#else
	FILE* m_fp;
#endif
};

#endif
