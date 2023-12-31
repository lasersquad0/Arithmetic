
#include <stdio.h>
#include <iostream>
#include <filesystem>
#include <direct.h>
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include "Archiver.h"
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
// ������� ��� �� model order 4 �� ��� ������� ������
// ����������� � �������� ��� Loadblock, SaveBlock ��� �� �� ���������� �������
// �������� TMemoryStream.
// ���������������
// 

static void PrintUsage(COptionsList& options)
{
    Global::GetLogger().info(CHelpFormatter::Format(Global::APP_NAME, &options));
}


static void DefineOptions(COptionsList& options)
{    
    COption aa;
    aa.ShortName("a").LongName("add").Descr("Add files to archive").Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(aa);

    COption xx;
    xx.ShortName("x").LongName("extract").Descr("Extract files from archive").Required(false).NumArgs(55).RequiredArgs(1);
    options.AddOption(xx);

    COption dd;
    dd.ShortName("d").LongName("delete").Descr("Delete files from archive").Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(dd);

    options.AddOption("b", "blocksize", "Set the block size for BWT transformation", 1);
    //options.AddOption("x", "extract", "Extract files from archive", 1);
    options.AddOption("l", "list", "List content of archive", 1);
    options.AddOption("t", "threads", "Use specified number of threads during operation", 1);
    options.AddOption("h", "help", "Show help", 0);
    options.AddOption("m", "model-type", "Use model of specified order. Valid model types: o1, o2, o3, o4, fo1, bito1.", 1);
    options.AddOption("c", "coder-type", "Use specified coder. Valid coders: huf, ahuf, ari, aari, bitari.", 1);
    options.AddOption("v", "verbose", "Print more detailed (verbose) information to screen.", 0);
    options.AddOption("sm", "stream-mode", "Use stream mode (oposite to block mode). No BWT, no MTB in this mode.", 0);
    options.AddOption("o", "output-dir", "Specifies directory where uncompressed files will be placed. Valid with -x option only.", 1);
}


#define TRYCATCH(_,__) try {(_);}catch(...){logger.warn(__);}


int main(int argc,char* argv[])
{
    if (!SaveLog4cppConfigurationFile()) 
        return 1; // were not able to load log4cpp.properties from resources - exiting with error message

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

    Category& logger = Global::GetLogger(); //Category::getInstance("sub1"); // getRoot();
    //ParametersG::logger = Category::getInstance("sub1");

    logger.info("Arithmetic coder start");
    
    SetImbue(cout);

    Global::APP_NAME = argv[0]; // this statement needs to be before first possible call of PrintUsage().

    CDefaultParser defaultParser;
    CCommandLine cmd;
    COptionsList options;

    DefineOptions(options);

    if (argc < 2)
    {
        logger.error("Error: No command line arguments found.");
        PrintUsage(options);

        return 1;
    }

    if (!defaultParser.Parse(&options, &cmd, argv, argc))
    {
        logger.error(defaultParser.GetLastError());
        PrintUsage(options);

        return 1;
    }


    Parameters params;
    params.BLOCK_MODE = !cmd.HasOption("sm");
    

    if (cmd.HasOption("t"))
    {
        //TODO add check for value range (1...24)
        TRYCATCH(params.THREADS = stoi(cmd.GetOptionValue("t", 0).c_str()), "Cannot parse Threads count option value (-t). Default value 1 will be used.");
    }

    if (cmd.HasOption("b"))
    {
        //TODO add check for value range (1024...100M)
        TRYCATCH(params.BLOCK_SIZE = ParseBlockSize(cmd.GetOptionValue("b", 0)), "Cannot parse Block size option value (-b). Default value 64K bytes will be used.");
    }
    if (cmd.HasOption("m"))
    {
        TRYCATCH(params.MODEL_TYPE = ParseModelType(cmd.GetOptionValue("m", 0)), "Cannot parse Model type option value (-m). Default value 'O3' will be used.");
    }

    if (cmd.HasOption("c"))
    {
        TRYCATCH(params.CODER_TYPE = ParseCoderType(cmd.GetOptionValue("c", 0)), "Cannot parse Coder type option value (-c). Default value 'AARI' will be used.");
    }

    if (cmd.HasOption("v"))
    {
        params.VERBOSE = true;
    }

    if (cmd.HasOption("o"))
    {
        params.OUTPUT_DIR = cmd.GetOptionValue("o");
    }

    if (params.VERBOSE)
    {
        for (size_t i = 0; i < argc; i++) // for debugging purposes
            logger.info("CLI param: %s", argv[i]);
    }

 
    try
    {
        if (cmd.HasOption("h"))
        {
            PrintUsage(options);
        }
        else if (cmd.HasOption("l"))
        {
            ArchiveHeader hd;
            hd.listContent(cmd.GetOptionValue("l", 0), params.VERBOSE);
        }
        else if (cmd.HasOption("a"))
        {
            vector_string_t files1 = cmd.GetOptionValues("a");
            vector_string_t files2(files1.begin() + 1, files1.end());

            //copy(files1.begin()++, files1.end(), files2.begin()); // copy to file2 everything but files[0];

            Archiver comp;
            comp.CompressFiles(files2, files1[0], params);
        }
        else if (cmd.HasOption("x"))
        {
            if (cmd.HasOption("o"))
            {
                int err = _mkdir(params.OUTPUT_DIR.c_str()); // try to make dir to ensure that we received correct dir string 
                if (err != 0)
                    if (errno != EEXIST) // EEXIST is a valid error here
                    {
                        logger.error("Output directory is not valid. Cannot move on to uncompress file(s). Exiting.");
                        return 1;
                    }
            }

            string arcFile = cmd.GetOptionValue("x", 0);
            auto values = cmd.GetOptionValues("x");

            if (values.size() > 1)
            {
                Archiver comp;

                for (size_t i = 1; i < values.size(); i++) // first item is archive name
                {
                    comp.ExtractFile(arcFile, values[i], params.OUTPUT_DIR);
                }
            }
            else
            {
                ifstream fin(arcFile, ios::in | ios::binary);
                if (!fin)
                {
                    logger.warn("Cannot open archive file '%s' for reading. Exiting.", arcFile.c_str());
                    return 1;
                }

                Archiver comp;
                comp.UncompressFiles(&fin, params);
            }

        }
        else if (cmd.HasOption("d"))
        {
            string arcFile = cmd.GetOptionValue("d", 0);
            
            vector_string_t files1 = cmd.GetOptionValues("d");
            vector_string_t files2(files1.begin() + 1, files1.end());

            //string fileToDelete = cmd.GetOptionValue("d", 1);
            Archiver arch;
            arch.RemoveFiles(arcFile, files2);
        }
        else
        {
            logger.warn("Incorrect command line parameters.");
            PrintUsage(options);
            return 1;
        }
    }
    catch (runtime_error& e)
    {
        logger.error(e.what());
        return 1;
    }
    catch (exception& e)
    {
        logger.error(e.what());
        return 1;
    }
    catch (...)
    {
        logger.error("Unknown error has been caught.");
        return 1;
    }


    logger.info("Arithmetic coder finished.");
    return 0;
}
