
#include "debug.h"
#include <iostream>
#include <filesystem>
#include <string>
//#include <direct.h>

#include "LogEngine2/LogEngine.h"
#include "resource.h"
#include "Archiver.h"
#include "cli/OptionsList.h"
#include "cli/DefaultParser.h"
#include "cli/CommandLine.h"
#include "cli/HelpFormatter.h"
#include "Version.h"
#include "Parameters.h"
#include "CommonFunctions.h"


using namespace std;


// *************** TODO *********************
// optimize model order 4 to use less memory
// check memory allocations for buffers in Loadblock, SaveBlock to avoid allocating extra memory
// add multithreading for uncompress
// think how to implement multithreading on compress. problem - coder and model have states, difficult to coder and model classes in parallel from diff threads 
// put IniReader into LogEngine namespace? together with exception class FileException?

//wtos используется:
// для вызовов logger.Info(var) - они требуют ANSI
// для выброса исключения. Исключения требуют всегда ANSI
// Функции работы с файлами типа std::remove() требуют char* -> переписать через std::filesystem?


#define OPT_A U("a")
#define OPT_B U("b")
#define OPT_C U("c")
#define OPT_D U("d")
#define OPT_H U("h")
#define OPT_L U("l")
#define OPT_M U("m")
#define OPT_O U("o")
#define OPT_S U("s")
#define OPT_T U("t")
#define OPT_V U("v")
#define OPT_X U("x")

static void DefineOptions(COptionsList& options)
{    
    COption aa;
    aa.ShortName(OPT_A).LongName(U("add")).Descr(U("Add files to archive")).Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(aa);

    COption xx;
    xx.ShortName(OPT_X).LongName(U("extract")).Descr(U("Extract files from archive")).Required(false).NumArgs(55).RequiredArgs(1);
    options.AddOption(xx);

    COption dd;
    dd.ShortName(OPT_D).LongName(U("delete")).Descr(U("Delete files from archive")).Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(dd);

    options.AddOption(OPT_B, U("blocksize"), U("Set the block size for BWT transformation"), 1); // all numArgs here are marked as required
    options.AddOption(OPT_L, U("list"), U("List content of archive"), 1);
    options.AddOption(OPT_T, U("threads"), U("Use specified number of threads during operation"), 1);
    options.AddOption(OPT_H, U("help"), U("Show help"), 0);
    options.AddOption(OPT_M, U("model-type"), U("Use model of specified order. Valid model types: O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ."), 1);
    options.AddOption(OPT_C, U("coder-type"), U("Use specified coder. Valid coders: huf, ahuf, ari, ari32, ari64, bitari, fpaqari."), 1);
    options.AddOption(OPT_V, U("verbose"), U("Print more detailed (verbose) information to screen."), 0);
    options.AddOption(OPT_S, U("stream-mode"), U("Use stream mode (oposite to block mode). No BWT, no MTB in this mode."), 0);
    options.AddOption(OPT_O, U("output-dir"), U("Specifies directory where uncompressed files will be placed. Valid with -x option only."), 1);

	options.MutuallyExclusive(OPT_A, OPT_X, OPT_D);
	//options.MutuallyExclusive(OPT_B, OPT_S);
}

static void PrintUsage(COptionsList& options)
{
	Global::GetLogger().Info(wtos(CHelpFormatter::Format(convert_string<char_t>(Global::APP_NAME), &options)));
}

// this function is called ONLY when .lfg file cannot be found or error during parsing .lfg file
static void InitDefaultLogger()
{
	std::shared_ptr<LogEngine::Sink> consoleSink(new LogEngine::StdoutSinkST("console_sink"));
	consoleSink->SetPattern("%time% %msg%");
	std::shared_ptr<LogEngine::Sink> fileSink(new LogEngine::FileSinkST("file_sink", Global::LOGFILE_NAME));

	LogEngine::Logger& logger = LogEngine::GetMultiLogger(Global::LOGGER_NAME, { consoleSink, fileSink });
	logger.SetLogLevel(LogEngine::llInfo); // set log level for logger and all its sinks.

	LOG_INFO("ArithmeticLog.lfg is not found, use default logging configuration.");
}

#define TRYCATCH(_,__) try {(_);}catch(...){logger.Warn(__);}

//TODO implementation of this function is not finished.
void CheckParametersValidity(Parameters& params);

int main(int argc, char* argv[])
//int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtMemState s1, s2, s3;
	_CrtMemCheckpoint(&s1); // Take a snapshot at the start of main()
#endif

	try
	{
		// try to load .lfg file from currect directory
		LogEngine::InitFromFile(Global::LFGFILE_NAME); // throws an exception if file is not found

	}
	catch (FileException&)
	{
#ifdef WIN32
		// if ArithmeticLog.lfg is not found, try to find .lfg file in resources of .exe file
		if (SaveLogConfigFile(IDR_RT_RCDATA2, Global::LFGFILE_NAME))
		{
			try
			{
				// try to load .lfg again 
				LogEngine::InitFromFile(Global::LFGFILE_NAME); // throws an exception if file is not found
			}
			catch (FileException&)
			{
				// if .lfg file cannot be loaded again - use default logger settings
				InitDefaultLogger();
			}
		}
		else
		{
			// not able to load ArithmeticLog.lfg from resources - configure logger settings manually (console + file)
			InitDefaultLogger();
		}
#else
		// if .lfg file is not found, configure logger settings manually (console + file)
		InitDefaultLogger();
#endif
	}

	GET_LOGGER();
	logger.Imbue(Global::GetLoc()); // first call after logger initialization, set locale into logger
	cout.imbue(Global::GetLoc());
	SetConsoleOutputCP(CP_UTF8);

	logger.Debug("Arithmetic coder start");	

	logger.InfoFmt("\nARPack arithmetic compressor {}.{}.{} x64 Copyright(c) 2024 - 2026 Andrey Romanchenko {}\n", ARI_VER_MAJOR, ARI_VER_MINOR,ARI_VER_PATCH,ARI_BUILD_DATE);

    Global::APP_NAME = argv[0]; // this statement needs to be before first possible call of PrintUsage().

    CDefaultParser defaultParser;
    CCommandLine cmd;
    COptionsList options;

    DefineOptions(options);

    if (argc < 2)
    {
        //LOG_ERROR("No command line arguments found.");
        PrintUsage(options);
        return 1;
    }

	vector_string_t args;
	//args.resize(argc - 1);

	for (int i = 1; i < argc; ++i) // i=1: bypass program name (argv[0])
	{
		args.push_back(convert_string<char_t>(argv[i]));
	}

    //if (!defaultParser.Parse(&options, &cmd, argv, argc))
	if (!defaultParser.Parse(&options, &cmd, args))
    {
        LOG_ERROR(wtos(defaultParser.GetLastError()));
        PrintUsage(options);
        return 1;
    }

	Parameters params;
	params.BLOCK_MODE = !cmd.HasOption(OPT_S);

	if (cmd.HasOption(OPT_T))
	{
		//TODO add check for value range (1...24)
		TRYCATCH(params.THREADS = std::stoul(cmd.GetOptionValue(OPT_T, 0)), format("Cannot parse Threads option '-t {}'. Default value 1 will be used.", wtos(cmd.GetOptionValue(OPT_T, 0))));
	}
	if (cmd.HasOption(OPT_B))
	{
		//TODO add check for value range (1024...100M)
		TRYCATCH(params.BLOCK_SIZE = ParseBlockSize(cmd.GetOptionValue(OPT_B, 0)), format("Cannot parse Block Size option '-b {}'. Default value 64K bytes will be used.", wtos(cmd.GetOptionValue(OPT_B, 0))));
	}
	if (cmd.HasOption(OPT_V))
	{
		params.VERBOSE = true;
	}
	if (cmd.HasOption(OPT_O))
	{
		params.OUTPUT_DIR = cmd.GetOptionValue(OPT_O);
	}
	if (cmd.HasOption(OPT_M))
	{
		TRYCATCH(params.MODEL_TYPE = ParseModelType(cmd.GetOptionValue(OPT_M, 0)), format("Cannot parse Model type option value (-m {}). Default value 'O2' will be used.", wtos(cmd.GetOptionValue(OPT_M, 0))));
	}
	if (cmd.HasOption(OPT_C))
	{
		TRYCATCH(params.CODER_TYPE = ParseCoderType(cmd.GetOptionValue(OPT_C, 0)), format("Cannot parse Coder type option value (-c {}). Default value 'ARI' will be used.", wtos(cmd.GetOptionValue(OPT_C, 0))));
	}

/*	if (params.VERBOSE)
	{
		for (size_t i = 0; i < argc; i++) // for debugging purposes
			LOG_INFO1("CLI param: {}", wtos(argv[i]));
	}
*/

	try
	{
		CheckParametersValidity(params);

		if (cmd.HasOption(OPT_H))
		{
			PrintUsage(options);
		}
		else if (cmd.HasOption(OPT_L))
		{
			ArchiveHeader hd;
			hd.ListContent(cmd.GetOptionValue(OPT_L, 0), params.VERBOSE);
		}
		else if (cmd.HasOption(OPT_A))
		{
			vector_string_t files1 = cmd.GetOptionValues(OPT_A);
            vector_string_t files2(files1.begin() + 1, files1.end()); // copy to file2 everything but files1[0]

			Archiver comp;
			ConsoleProgressCallback ccb;
			comp.AddCallback(&ccb);
			comp.CompressFiles(files1[0], files2, params);
			comp.RemoveCallback(&ccb);
		}
		else if (cmd.HasOption(OPT_X))
		{
			if (cmd.HasOption(OPT_O))
			{
				int err = std::filesystem::create_directories(params.OUTPUT_DIR); // try to make dir to ensure that we received correct dir string
				if (err != 0)
					if (errno != EEXIST) // EEXIST is a valid error here
					{
						LOG_ERROR("Output directory is not valid. Cannot move on to uncompress file(s). Exiting.");
						return 1;
					}
			}

			string_t arcFile = cmd.GetOptionValue(OPT_X, 0);
			auto& values = cmd.GetOptionValues(OPT_X);

			if (values.size() > 1)
			{
				Archiver comp;  // extract certain files from archive
				ConsoleProgressCallback ccb;
				comp.AddCallback(&ccb);

				for (size_t i = 1; i < values.size(); i++) // first item is archive name
				{
					comp.ExtractFile(arcFile, values[i], params.OUTPUT_DIR);
				}
				comp.RemoveCallback(&ccb);
			}
			else // uncompress ALL files from archive
			{
				Archiver comp;
				ConsoleProgressCallback ccb;
				comp.AddCallback(&ccb);
				comp.UncompressFiles(arcFile, params);
				comp.RemoveCallback(&ccb);
			}
		}
		else if (cmd.HasOption(OPT_D))
		{
			string_t arcFile = cmd.GetOptionValue(OPT_D, 0);

			vector_string_t files1 = cmd.GetOptionValues(OPT_D);
            vector_string_t files2(files1.begin() + 1, files1.end());

            //vect_string_t files2 = convert_vector(tmp);
			//string fileToDelete = cmd.GetOptionValue("d", 1);
			Archiver arch;
			arch.RemoveFiles(arcFile, files2);
		}
		else
		{

			LOG_WARN("Incorrect command line parameters.");
			PrintUsage(options);
			return 1;
		}
	}
	catch (runtime_error& e)
	{
		LOG_ERROR(e.what());
		LOG_ERROR("Exiting.");
		return 1;
	}
	catch (exception& e)
	{
		LOG_ERROR(e.what());
		LOG_ERROR("Exiting.");
		return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown error has been caught.");
		LOG_ERROR("Exiting.");
		return 1;
	}

	LOG_DEBUG("Arithmetic coder finished.");

#ifdef WIN32
	_CrtMemCheckpoint(&s2); // Take a snapshot at the end of main()
	_CrtMemCheckpoint(&s2); // Take a snapshot at the end of main()
	if (_CrtMemDifference(&s3, &s1, &s2)) _CrtMemDumpStatistics(&s3); // Dump memory statistics excluding global variables
#endif

	return 0;
}
