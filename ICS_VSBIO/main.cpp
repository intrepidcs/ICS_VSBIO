#include <stdio.h>

#include <chrono>
#include "VSBIO.h"
#include "CMPNetworkMap.h"
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
    printf("or -cmp_unwrap <quoted full path of input vsb file> <quoted full path of output vsb file> [sort outgoing vsb frames: 0=sort (default), 1=no sort] [timestamp source: 0=CMP (default), 1=Ethernet] [output info to txt: 0=NO (default), 1=YES]\n");
    printf("Optional prefix (place before any command): -cmp_map_vsdb <vsdb file with <CmpNetworkMap>>\n");
    printf("Example: VSBIO -cmp_map_vsdb mymap.vsdb -cmp_unwrap in.vsb out.vsb\n");
    fflush(stdout);
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

    std::string cmpMapVsdbPath;
    if (args.size() >= 2 && args[0] == "-cmp_map_vsdb")
    {
        cmpMapVsdbPath = args[1];
        args.erase(args.begin(), args.begin() + 2);
    }

    if (args.size() < 2)
    {
        ShowUsage();
        return -1;
    }


    if (!cmpMapVsdbPath.empty())
    {
        if (!LoadCmpNetworkMapFromVsdb(cmpMapVsdbPath.c_str()))
        {
            printf("Warning: could not load CMP network map from %s; using built-in map.\n", cmpMapVsdbPath.c_str());
        }
    }
    auto start = std::chrono::high_resolution_clock::now();
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
    else if ((args[0] == "-cmp_unwrap") && (args.size() >= 3))
    {
        if (FileExists(args[1]))
        {
            int sortOutput = (args.size() >= 4) ? atoi(args[3].c_str()) : 0; // Default to sort (0)
            int timestampSource = (args.size() >= 5) ? atoi(args[4].c_str()) : 0; // Default to CMP timestamp (0)
            int outputInfoToTxt = (args.size() >= 6) ? atoi(args[5].c_str()) : 0; // Default to no report (0)
            retCode = UnwrapCMPToNativeVSBNetIDs(args[1].c_str(), args[2].c_str(), sortOutput, timestampSource, ShowProgressFunc, outputInfoToTxt) ? 0 : -1;
            if (retCode == 0)
            {
                printf("\nCMP unwrap completed successfully!");
            }
            else
            {
                printf("\nError unwrapping CMP messages!");
            }
        }
        else
        {
            printf("\nError opening file!");
        }
    }
    if (retCode == -1)
        ShowUsage();
    else
    {
        auto duration = std::chrono::high_resolution_clock::now() - start;
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
        duration -= minutes;
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        duration -= seconds;
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);
        duration -= microseconds;
        printf("\nOperation took: %02d:%02lld:%06lld min\n", minutes.count(), seconds.count(), microseconds.count());
    }
	return retCode;
}
