
#include <iostream>
#include <filesystem>
#include <direct.h>
#ifdef LOG4CPP
#include <log4cpp/Category.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include "resource.h"
#else
#include "LogEngine.h"
#endif
#include <strsafe.h>
#include "Archiver.h"
#include "OptionsList.h"
#include "DefaultParser.h"
#include "CommandLine.h"
#include "HelpFormatter.h"
#include "Parameters.h"



using namespace std;

// *************** TODO *********************
// optimize model order 4 to use less memory
// check memory allocations for buffers in Loadblock, SaveBlock to avoid allocating extra memory
// finish TMemoryStream class.
// work on multithreading


void PrintWindowsErrorMessage(const TCHAR* lpszFunction)
{
    LPVOID lpMsgBuf;
    DWORD err = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    wcout << (TCHAR*)lpMsgBuf << endl;

    //const TCHAR* lpszFunction = TEXT("FindResource");

    LPVOID lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

    StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"), lpszFunction, err, lpMsgBuf);


    wcout << (TCHAR*)lpDisplayBuf << endl;

    LocalFree(lpDisplayBuf);

    cout << "Error opening 'log4cpp.properties' resource! Error code:" << err << endl;

}

#ifdef LOG4CPP
bool SaveLog4cppConfigurationFile()
{
    HRSRC resInfo = FindResource(nullptr, MAKEINTRESOURCE(IDR_RT_RCDATA1), L"RT_RCDATA");

    if (resInfo == nullptr)
    {
        PrintWindowsErrorMessage(TEXT("FindResource"));
        return false;
    }

    HGLOBAL resBytes = LoadResource(nullptr, resInfo);

    if (resBytes == nullptr)
    {
        PrintWindowsErrorMessage(TEXT("LoadResource"));
        return false;
    }

    LPVOID lockBytes = LockResource(resBytes);

    if (lockBytes == nullptr)
    {
        PrintWindowsErrorMessage(TEXT("LockResource"));
        return false;
    }

    DWORD size = SizeofResource(nullptr, resInfo);

    ofstream out;
    out.open("log4cpp.prop", ios::binary);
    out.write((char*)lockBytes, size);
    out.close();

    return true;
}
#endif


static void PrintUsage(COptionsList& options)
{
#ifdef LOG4CPP
    Global::GetLogger().info(CHelpFormatter::Format(Global::APP_NAME, &options));
#else
    Global::GetLogger().Info(convert_string<char>(CHelpFormatter::Format(Global::APP_NAME, &options)));
#endif
}

static void DefineOptions(COptionsList& options)
{    
    COption aa;
    aa.ShortName(_T("a")).LongName(_T("add")).Descr(_T("Add files to archive")).Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(aa);

    COption xx;
    xx.ShortName(_T("x")).LongName(_T("extract")).Descr(_T("Extract files from archive")).Required(false).NumArgs(55).RequiredArgs(1);
    options.AddOption(xx);

    COption dd;
    dd.ShortName(_T("d")).LongName(_T("delete")).Descr(_T("Delete files from archive")).Required(false).NumArgs(55).RequiredArgs(2);
    options.AddOption(dd);

    options.AddOption(_T("b"), _T("blocksize"), _T("Set the block size for BWT transformation"), 1); // all numArgs here are marked as required
    options.AddOption(_T("l"), _T("list"), _T("List content of archive"), 1);
    options.AddOption(_T("t"), _T("threads"), _T("Use specified number of threads during operation"), 1);
    options.AddOption(_T("h"), _T("help"), _T("Show help"), 0);
    options.AddOption(_T("m"), _T("model-type"), _T("Use model of specified order. Valid model types: o1, o2, o3, o4, fo1, bito1."), 1);
    options.AddOption(_T("c"), _T("coder-type"), _T("Use specified coder. Valid coders: huf, ahuf, ari, aari, bitari."), 1);
    options.AddOption(_T("v"), _T("verbose"), _T("Print more detailed (verbose) information to screen."), 0);
    options.AddOption(_T("sm"), _T("stream-mode"), _T("Use stream mode (oposite to block mode). No BWT, no MTB in this mode."), 0);
    options.AddOption(_T("o"), _T("output-dir"), _T("Specifies directory where uncompressed files will be placed. Valid with -x option only."), 1);
}

#ifdef LOG4CPP
#define TRYCATCH(_,__) try {(_);}catch(...){logger.warn(__);}
#else
#define TRYCATCH(_,__) try {(_);}catch(...){logger.Warn(__);}
#endif

int _tmain(int argc, _TCHAR* argv[])
//int main(int argc,char* argv[])
{
#ifdef LOG4CPP
    if (!SaveLog4cppConfigurationFile())
        return 1; // were not able to load log4cpp.properties from resources - exiting with error message

    try
    {
        log2cpp::PropertyConfigurator::configure("log4cpp.properties");
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
        cout << "Exiting." << endl;
        return 1;
    }

    log2cpp::Category& logger = Global::GetLogger(); //Category::getInstance("sub1"); // getRoot();
    //ParametersG::logger = Category::getInstance("sub1");

    logger.info("Arithmetic coder start");
#else
    LogEngine::InitFromFile("ArithmeticLog.lfg");

    LogEngine::Logger& logger = Global::GetLogger();
    logger.Info("Arithmetic coder start");
#endif


    SetImbue(cout);

    Global::APP_NAME = argv[0]; // this statement needs to be before first possible call of PrintUsage().

    CDefaultParser defaultParser;
    CCommandLine cmd;
    COptionsList options;

    DefineOptions(options);

    if (argc < 2)
    {
        LOG_ERROR("Error: No command line arguments found.");
        PrintUsage(options);
        return 1;
    }

    if (!defaultParser.Parse(&options, &cmd, argv, argc))
    {
        LOG_ERROR(convert_string<char>(defaultParser.GetLastError()));
        PrintUsage(options);
        return 1;
    }

	//Parameters params2;

	Parameters params;
	params.BLOCK_MODE = !cmd.HasOption(_T("sm"));


	if (cmd.HasOption(_T("t")))
	{
		//TODO add check for value range (1...24)
		TRYCATCH(params.THREADS = std::stoi(cmd.GetOptionValue(_T("t"), 0)), "Cannot parse Threads count option value (-t). Default value 1 will be used.");
	}

	if (cmd.HasOption(_T("b")))
	{
		//TODO add check for value range (1024...100M)
		TRYCATCH(params.BLOCK_SIZE = ParseBlockSize(cmd.GetOptionValue(_T("b"), 0)), "Cannot parse Block size option value (-b). Default value 64K bytes will be used.");
	}
	if (cmd.HasOption(_T("m")))
	{
		TRYCATCH(params.MODEL_TYPE = ParseModelType(cmd.GetOptionValue(_T("m"), 0)), "Cannot parse Model type option value (-m). Default value 'O2' will be used.");
	}

	if (cmd.HasOption(_T("c")))
	{
		TRYCATCH(params.CODER_TYPE = ParseCoderType(cmd.GetOptionValue(_T("c"), 0)), "Cannot parse Coder type option value (-c). Default value 'AARI' will be used.");
	}

	if (cmd.HasOption(_T("v")))
	{
		params.VERBOSE = true;
	}

	if (cmd.HasOption(_T("o")))
	{
		params.OUTPUT_DIR = cmd.GetOptionValue(_T("o"));
	}

	if (params.VERBOSE)
	{
		for (size_t i = 0; i < argc; i++) // for debugging purposes
#ifdef LOG4CPP
			logger.info("CLI param: %s", argv[i]);
#elif defined (__BORLANDC__)
			logger.LogFmt(LogEngine::Levels::llInfo, "CLI param: %s", argv[i]);
#else
			logger.InfoFmt("CLI param: {}", convert_string<char>(argv[i]));
#endif
	}


	try
	{
		if (cmd.HasOption(_T("h")))
		{
			PrintUsage(options);
		}
		else if (cmd.HasOption(_T("l")))
		{
			ArchiveHeader hd;
			hd.listContent(cmd.GetOptionValue(_T("l"), 0), params.VERBOSE);
		}
		else if (cmd.HasOption(_T("a")))
		{
			vector_string_t files1 = cmd.GetOptionValues(_T("a"));
            vector_string_t files2(files1.begin() + 1, files1.end()); // copy to file2 everything but files1[0]

			//copy(files1.begin()++, files1.end(), files2.begin());
			Archiver comp;
			ConsoleProgressCallback ccb;
			comp.AddCallback(&ccb);
			comp.CompressFiles(files1[0], files2, params);
			comp.RemoveCallback(&ccb);
		}
		else if (cmd.HasOption(_T("x")))
		{
			if (cmd.HasOption(_T("o")))
			{
				int err = _wmkdir(params.OUTPUT_DIR.c_str()); // try to make dir to ensure that we received correct dir string
				if (err != 0)
					if (errno != EEXIST) // EEXIST is a valid error here
					{
						LOG_ERROR("Output directory is not valid. Cannot move on to uncompress file(s). Exiting.");
						return 1;
					}
			}

			string_t arcFile = cmd.GetOptionValue(_T("x"), 0);
			auto values = cmd.GetOptionValues(_T("x"));

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
		else if (cmd.HasOption(_T("d")))
		{
			string_t arcFile = cmd.GetOptionValue(_T("d"), 0);

			vector_string_t files1 = cmd.GetOptionValues(_T("d"));
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
		return 1;
	}
	catch (exception& e)
	{
		LOG_ERROR(e.what());
		return 1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown error has been caught.");
		return 1;
	}

	LOG_INFO("Arithmetic coder finished.");
	return 0;
}
