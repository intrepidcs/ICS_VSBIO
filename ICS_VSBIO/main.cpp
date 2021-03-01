#include "VSBIODLL.h"
#include <tchar.h>
#include "OFile.h"

extern "C" bool ShowProgressFunc(int nPctDone)
{
    printf("\r%d done", nPctDone);
    return true;
}

#if defined(_WIN32)
int wmain(int argc, _TCHAR* argv[])
#else
#define _T(x) x
int main(int argc, const char** argv)
#endif
{
    if (argc < 2)
        wprintf(_T("Usage: VSBIO <File to split/Folder to Concat from>"));

#if defined(_WIN32)
    std::string sParm = mbstring(argv[1]);
#else
    std::string sParm = argv[1];
#endif
    std::string sDirectory, sName, sExtension;
    SplitPath(sParm, sDirectory, sName, sExtension);

    if (FileExists(sParm))
        Split(sParm.c_str(), 100000, sDirectory.c_str(), ShowProgressFunc);
    else if (IsDirectory(sParm))
        Concatenate(sParm.c_str(), CombinePath(sDirectory, "Combined.vsb").c_str(), ShowProgressFunc);

	return 0;
}
