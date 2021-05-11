#ifndef _OFILE_H
#define _OFILE_H

#include <string>
#include <vector>
#include <map>
#include <list>

#ifdef _WIN32
#include <Windows.h>
#endif

class OFile
{
public:

    OFile() : m_bReadOnly(false)
    {
#ifdef _WIN32
	    hFile = NULL;
#else
	    m_fp = NULL;
#endif
    }

    ~OFile() { CloseFile(); }

	bool OpenFile(const char* sFileName, bool bCreateNew, bool bWriteable, bool bOverlapped = false);
	bool CloseFile(void);

	bool Read(void* buffer, unsigned long bytesToRead, unsigned long& bytesRead);
	bool Write(const void* buffer, unsigned long bytesToWrite, unsigned long& bytesWritten
#ifdef _WIN32
		,
		LPOVERLAPPED = NULL
#endif
		);

	unsigned long SetFilePtr(long distance, long* disttomovehi, unsigned long moveMethod);
	unsigned long long SetFilePtr(unsigned long long offset, unsigned long moveMethod);
	long long GetFilePtrLong();
	unsigned long long FileSizeLarge(void);

private:
	bool m_bReadOnly;
	bool m_bOverlapped;
#ifdef _WIN32
	HANDLE hFile;
#else
	FILE* m_fp;
#endif
};

#if defined(_WIN32)
#define OS_Path_Sep "\\"
#else
#define OS_Path_Sep "/"
#endif

std::wstring widestring(const std::string &text);
std::string mbstring(const std::wstring &text);

bool FileExists(const std::string& sPath);
bool IsDirectory(const std::string& sPath);

std::string CombinePath(const std::string& sPath, const std::string& sFileName);
size_t GetLastSlash(const std::string& sFilePath);
void SplitPath(const std::string& sFullPath, std::string& sDirectory, std::string& sFileName, std::string& sExtension);
std::vector<std::string> GetFilesInDirectory(const std::string& sPath, const std::string& sExtension);
std::string GetJson(const std::map<std::string, std::string>& strings);
std::list<std::string> split(const char* str, char c);

#endif
