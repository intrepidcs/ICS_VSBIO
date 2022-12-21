#include <stdio.h>

#include "VSBIO.h"
#include "VSBIODLL.h"

#if defined _WIN32
#include <tchar.h>
#endif

#include "VSBDatabase.h"

extern "C" bool ShowProgressFunc(int nPctDone)
{
    printf("\r%d%% done", nPctDone);
    return true;
}

void ShowUsage()
{
    printf("Usage: VSBIO -s <quoted full path of file to split> <num messages> <output directory>\n");
    printf("or -z <quoted full path of file to split> <decimal max file size in MB> <output directory>\n");
    printf("or -c <quoted directory where files to combine reside> <quoted full path of output file>\n");
    printf("or -db <quoted full path of vsb file> <quoted full path to db file to create>\n");
    printf("or -append <quoted full path of vsb file> <quoted full path to db file to create or append to>\n");
    printf("or -filter <quoted full path of db file> <quoted full path to vsb file to create> <quoted filter expression>\n");
}

#if defined(_WIN32)
int wmain(int argc, _TCHAR* argv[])
#else
int main(int argc, const char** argv)
#endif
{
    std::vector<std::string> args;
    for (int arg = 1; arg < argc; ++arg)
#if defined(_WIN32)
        args.push_back(mbstring(argv[arg]));
#else
        args.push_back(argv[arg]);
#endif
    if (args.size() < 2)
    {
        ShowUsage();
        return -1;
    }

    int retCode = -1;
    if ((args[0] == "-s") && (args.size() == 4))
    {
        unsigned int numMessages = (unsigned int)strtoul(args[2].c_str(), NULL, 0);
        if ((numMessages > 0) && FileExists(args[1]))
        {
            retCode = Split(args[1].c_str(), numMessages, args[3].c_str(), ShowProgressFunc);
            if (retCode)
            {
                printf("\nFile was split successfully!");
            }
            else
            {
                printf("\nError splitting file!");
            }
        }
    }
    else if ((args[0] == "-z") && (args.size() == 4))
    {
        double maxFileSize = (unsigned int)strtod(args[2].c_str(), NULL);
        if ((maxFileSize > 0) && FileExists(args[1]))
        {
            retCode = SplitBySize(args[1].c_str(), maxFileSize, args[3].c_str(), ShowProgressFunc);
            if (retCode)
            {
                printf("\nFile was split successfully!");
            }
            else
            {
                printf("\nError splitting file!");
            }
        }
    }
    else if ((args[0] == "-c") && (args.size() == 3))
    {
        if (IsDirectory(args[1]))
        {
            retCode = Concatenate(args[1].c_str(), args[2].c_str(), ShowProgressFunc);
            if (retCode)
        {
            printf("\nFiles were concatenated successfully!");
        }
        else
        {
            printf("\nError concatenating files!");
            }
        }
    }
    else if ((args[0] == "-db") && (args.size() == 3))
    {
        if (FileExists(args[1]))
        {
            retCode = CreateDatabase(args[1].c_str(), args[2].c_str(), ShowProgressFunc) ? 0 : -1;
        }
        else
        {
            printf("\nError opening file!");
        }
    }
    else if ((args[0] == "-append") && (args.size() == 3))
    {
        if (FileExists(args[1]))
        {
            retCode = AddToDatabase(args[1].c_str(), args[2].c_str(), ShowProgressFunc) ? 0 : -1;
        }
        else
        {
            printf("\nError opening file!");
        }
    }
    else if ((args[0] == "-filter") && (args.size() >= 3))
    {
        if (FileExists(args[1]))
        {
            retCode = WriteFilteredVsb(args[1].c_str(), args[2].c_str(), (args.size() == 3) ? NULL : args[3].c_str(),
                ShowProgressFunc);
        }
        else
        {
            printf("\nError opening file!");
        }
    }
    ShowUsage();
	return retCode;
}
