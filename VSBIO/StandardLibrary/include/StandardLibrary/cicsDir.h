#ifndef cicsDir_defined
#define cicsDir_defined

#include "Core/Defines.h"
#include <string>

class cicsString;
class cicsStringArray;

#if defined(WIN32) || defined(_WIN32)
#define cicsDir_Path_Sep ICSUNI("\\")
#else
#define cicsDir_Path_Sep ICSUNI("/")
#endif

class cicsDir
{
private:
	static bool IsDots(const char* str);

public:
	cicsDir();
	virtual ~cicsDir();

	typedef enum _sort_type_t {
		NO_SORT, /* No sort */
		SORT_BY_DATE_CREATED_FORWARD, /*Sort by creation date, first element created first*/
		SORT_BY_DATE_MODIFIED_FORWARD, /*Sort by modified date, first element modified first*/
		SORT_BY_DATE_CREATED_REVERSE, /*Sort by modified date, first element modified last*/
		SORT_BY_DATE_MODIFIED_REVERSE, /*Sort by creation date, first element created last*/
	} sort_type_t;

	static bool ReplaceFilenameExtension(cicsString& sPath, cicsString sNewExtension);
	static bool GetFilesInDirectory(const cicsString& sPath, const cicsString& sFileSpec, cicsStringArray& sFiles);
	static bool MakeDirectory(const cicsString& sPath);
	static bool RemoveFile(const cicsString& sPath);
	static bool GetExePath(cicsString& sPath);
	static void GetTempPath(std::string& sPath);
	static void GetTempPath(std::wstring& sPath);
	static void GetTempPath(cicsString& sPath);
	static bool CopyFileICS(const cicsString& sPathSource, const cicsString& sPathDestination);
	static bool FileShortName(const cicsString& sPath, cicsString& sOutShortPath);
	static bool DeleteFile(const cicsString& sPath);
	static bool RenameFile(const cicsString& sPath, const cicsString& sNewName);
	bool FileDetails(const cicsString& sPath, cicsString& sDetailsCSV);
	static bool GetPathFreeSpace(const cicsString& sPath, unsigned int& iKiloBytes, unsigned int& iTotalBytes);
	static bool GetDirectoriesInDirectory(const cicsString& sPath, const cicsString& sFileMask, cicsStringArray& sDirs);
	static cicsString CombinePath(const cicsString& sPath, const cicsString& sFileName);
	bool substring(char* s1, char* s2);

	/**
	 * Gets file list within a specific directory.
	 * @param sPath string directory path to search in. Example: "c:\\windows".
	 * @param sFileMask string file mask to filter search results. Example: "\\*.doc".
	 * @param sort_type enumerated sort type to order results. Example cicsDir::SORT_BY_DATE_CREATED_FORWARD.
	 * @param sFiles string array reference to put results in.
	 */
	static bool GetFilesInDirectory(const cicsString& sPath, const cicsString& sFileMask, sort_type_t sort_type, cicsStringArray& sFiles);

	static bool DeleteDirectory(const cicsString& sPath);
	static bool FileExists(const cicsString& sPath);
	static bool DirectoryExists(const cicsString& sPath);
	static int64_t GetLargeFileSize(const cicsString& sPath);
	static bool isAbsolutePath(const cicsString& path);
};


#endif
