
#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <direct.h>
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include "RangeCompressor.h"
#include "OptionsList.h"
#include "DefaultParser.h"
#include "CommandLine.h"
#include "HelpFormatter.h"
#include "Parameters.h"
#include "libsais64.h"
#include "Functions.h"

using namespace std;
using namespace log4cpp;

// *************** TODO *********************
// сделать что бы model order 4 не ела столько памяти
// разобраться с буферами при Loadblock, SaveBlock что бы не выделялось лишнего
// написать TMemoryStream.
// многопоточность
// 

static void printUsage(COptionsList& options)
{
    Category& logger = Category::getInstance(Parameters::LOGGER_NAME); // Category::getRoot();
    logger.info(CHelpFormatter::Format(Parameters::APP_NAME, &options));
}


static void defineOptions(COptionsList& options)
{    
    COption oo;
    oo.ShortName("a").LongName("add").Descr("Add files to archive").Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(oo);

    //options.AddOption("a", "add", "add", 2);
    options.AddOption("b", "blocksize", "set the block size for BWT transformation", 1);
    options.AddOption("x", "extract", "extract files from archive", 1);
    options.AddOption("l", "list", "list content of archive", 1);
    options.AddOption("t", "threads", "use specified number of threads during operation", 1);
    options.AddOption("h", "help", "show help", 0);
    options.AddOption("m", "model-order", "Use model of specified order. Valid model orders 1, 2, 3 and 4.", 1);
    options.AddOption("v", "verbose", "Print more detailed (verbose) information to screen.", 0);
    options.AddOption("sm", "stream-mode", "Use stream mode (oposite to block mode). No BWT, no MTB in this mode.", 0);
    options.AddOption("o", "output-dir", "Specifies directory where uncompressed files will be placed. Valid with -x option only.", 1);

}

#define TRYCATCH(_,__) try {(_);}catch(...){logger.warn(__);}

int main(int argc,char* argv[])
{
    try
    {
        PropertyConfigurator::configure("log4cpp.properties");
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
        cout << "Exiting." << endl;
        return 1;
    }

    Category& logger = Parameters::logger; //Category::getInstance("sub1"); // getRoot();
    //Parameters::logger = Category::getInstance("sub1");

    logger.info("Arithmetic coder start");
    
    SetImbue(cout);

    for (size_t i = 0; i < argc; i++)
    {
        logger.info("CLI param: %s", argv[i]);
    }

    /*
    string blockToLoad = "ttt\\bufencode0";
    struct _stat finfo;
    _stat(blockToLoad.c_str(), &finfo);
    
    const int BUF_SZ = finfo.st_size;

    int64_t lineNum = -1;
    uint8_t* buf = new uint8_t[BUF_SZ];
    uint8_t* bwtbuf = new uint8_t[BUF_SZ];
    int64_t* temp = new int64_t[BUF_SZ + 1];
    int64_t freq[256];

    LoadFromFile(blockToLoad.c_str(), (char*)buf, BUF_SZ);

    lineNum = libsais64_bwt(buf, bwtbuf, temp, BUF_SZ, 0, freq);

    SaveToFile("bwtbufencode00", (char*)bwtbuf, BUF_SZ);

    int64_t result = libsais64_unbwt(bwtbuf, buf, temp, BUF_SZ, nullptr, lineNum);

    return 1;
    */

    Parameters::APP_NAME = argv[0];

 
    CDefaultParser defaultParser;
    CCommandLine cmd;
    COptionsList options;

    defineOptions(options);

    if (argc < 2)
    {
        logger.error("Error: No command line arguments found.");
        printUsage(options);
        return 1;
    }

    if (!defaultParser.Parse(&options, &cmd, argv, argc))
    {
        logger.error(defaultParser.GetLastError());
        printUsage(options);

        return 1;
    }

    Parameters::BLOCK_MODE = !cmd.HasOption("sm");

    if (cmd.HasOption("t"))
    {
        //TODO add check for value range (1...24)

        TRYCATCH(Parameters::THREADS = stoi(cmd.GetOptionValue("t", 0).c_str()), "Cannot parse threads option value. Default value 1 will be used.");
    }

    if (cmd.HasOption("b"))
    {
        //TODO add check for value range (1024...100M)

        TRYCATCH(Parameters::BLOCK_SIZE = Parameters::parseNumber(cmd.GetOptionValue("b", 0)), "Cannot parse block size option value. Default value 64K will be used.");
    }
    
    if (cmd.HasOption("m"))
    {
        TRYCATCH(Parameters::MODEL_ORDER = Parameters::parseModelOrder(cmd.GetOptionValue("m", 0)), "Cannot parse model order option value or model order is out of range (1..4). Default value '3' will be used.");
    }

    if (cmd.HasOption("v"))
    {
        Parameters::VERBOSE = true;
    }

 

    if (cmd.HasOption("h"))
    {
        printUsage(options);
    }
    else if (cmd.HasOption("l"))
    {
        ArchiveHeader hd;
        hd.listContent(cmd.GetOptionValue("l", 0));
    }
    else if (cmd.HasOption("a")) 
    {
        try
        {
            RangeCompressor comp;
            comp.CompressFiles(cmd.GetOptionValues("a"));
        }
        catch (runtime_error& e)
        {
            logger.error(e.what());
            return 1;
        }
        catch (exception& e )
        {
            logger.error(e.what());
            return 1;
        }
        catch (...)
        {
            logger.error("Unknown error has been caught.");
            return 1;
        }

        //errno_t res1 = fopen_s(&EncodedFile, arcFile.c_str(), "wb");
    }
    else if (cmd.HasOption("x"))
    {
        if (cmd.HasOption("o"))
        {
            Parameters::OUTPUT_DIR = cmd.GetOptionValue("o");
            int err = _mkdir(Parameters::OUTPUT_DIR.c_str()); // try to make dir to ensure that we received correct dir string 
            if (err != 0)
                if (errno != EEXIST) // EEXIST is a valid error here, can move on
                {
                    logger.error("Output directory is not valid. Cannot uncompress file(s). Exiting.");
                    return 1;
                }
        }

        string arcFile = cmd.GetOptionValue("x", 0);

        ifstream fin(arcFile, ios::in | ios::binary);
        if (!fin)
        {
            logger.warn("Cannot open archive file '%s' for reading. Exiting.", arcFile.c_str());
            return 1;
        }

        try
        {
            RangeCompressor comp;
            comp.UncompressFiles(&fin);
        }
        catch (runtime_error& e)
        {
            logger.error(e.what());
            return 1;
        }
        catch (...)
        {
            logger.error("Unknown error has been caught.");
            return 1;
        }

        //errno_t res1 = fopen_s(&EncodedFile, arcFile.c_str(), "rb");
    }
    else
    {
        logger.warn("Incorrect command line parameters.");
        printUsage(options);
        return 1;
    }

    logger.info("Arithmetic coder finished.");
    return 0;
}
