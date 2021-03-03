#include "VSBIODLL.h"
#include "OFile.h"

extern "C" bool ShowProgressFunc(int nPctDone)
{
    printf("\r%d done", nPctDone);
    return true;
}

void ShowUsage()
{
    printf("Usage: VSBIO -s <quoted full path of file to split> <num messages> <output directory>\n");
    printf("or -c <quoted directory where files to combine reside> <quoted full path of output file>");
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
    if (args.size() < 3)
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
    ShowUsage();
	return -1;
}
