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
    printf("or -c <quoted directory where files to combine reside> <quoted full path of output file>\n");
    printf("or -db <quoted full path of vsb file> <quoted full path to db file to create>");
    printf("or -append <quoted full path of vsb file> <quoted full path to db file to create or append to>");
    printf("or -filter <quoted full path of db file> <quoted full path to vsb file to create> <quoted filter expression>");
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

    if ((args[0] == "-s") && (args.size() == 4))
    {
        unsigned int numMessages = (unsigned int)strtoul(args[2].c_str(), NULL, 0);
        if ((numMessages > 0) && FileExists(args[1]))
        {
            if (Split(args[1].c_str(), numMessages, args[3].c_str(), ShowProgressFunc))
            {
                printf("\nFile was split successfully!");
                return 0;
            }
            else
            {
                printf("\nError splitting file!");
                return -1;
            }
        }
    }
    else if ((args[0] == "-c") && (args.size() == 3))
    {
        if (IsDirectory(args[1]) && Concatenate(args[1].c_str(), args[2].c_str(), ShowProgressFunc))
        {
            printf("\nFiles were concatenated successfully!");
            return 0;
        }
        else
        {
            printf("\nError concatenating files!");
            return -1;
        }
    }
    else if ((args[0] == "-db") && (args.size() == 3))
    {
        if (FileExists(args[1]))
        {
            return CreateDb(args[1].c_str(), args[2].c_str(), false, ShowProgressFunc) ? 0 : -1;
        }
        else
        {
            printf("\nError opening file!");
            return -1;
        }
    }
    else if ((args[0] == "-append") && (args.size() == 3))
    {
        if (FileExists(args[1]))
        {
            return CreateDb(args[1].c_str(), args[2].c_str(), true, ShowProgressFunc) ? 0 : -1;
        }
        else
        {
            printf("\nError opening file!");
            return -1;
        }
    }
    else if ((args[0] == "-filter") && (args.size() >= 3))
    {
        if (FileExists(args[1]))
        {
            return WriteVsb(args[1].c_str(), args[2].c_str(), (args.size() == 3) ? NULL : args[3].c_str(),
                ShowProgressFunc) ? 0 : -1;
        }
        else
        {
            printf("\nError opening file!");
            return -1;
        }
    }
    ShowUsage();
	return -1;
}
