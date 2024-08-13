#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include "wcxhead.h"

#pragma warning(disable : 4996) 

void PrintWindowsErrorMessage(const TCHAR* lpszFunction);

#define PROG_LOGO																\
"Total Commander WCX plugin Test ver. 0.25. Copyright (c) 2004-2024 Oleg Bondar/Andrey Romanchenko.\n"	\
"Implements algorithm described in \"WCX Writer\'s Reference\" to test plugin's\n"	\
"functionality.\n"

#define PROG_USAGE																\
"Usage:\n"																   \
"  wcxtest [-f | -l | | -t | -x] [-q] <wcx_path> [<arc_path>] [<dir_path>]\n" \
"\n"																			\
"<wcx_path> - path to WCX plugin\n"												\
"<arc_path> - path to archive file\n"											\
"<dir_path> - directory to unpack files, default is current\n"					\
"  -f - list WCX exported functions\n"											\
"  -c - Show config dialog\n"													\
"  -l - List archive contents (default)\n"										\
"  -t - Test archive contents\n"												\
"  -x - eXtract files from archive (overwrite existing)\n"						\
"  -?, -h - this topic\n"														\
"\n"																			\
"  -v - Verbose\n"																\
"\n"																			\
"ERRORLEVEL: 0 - success, non zero - some (m.b. unknown) error.\n"				\
"\n"																			\
"Switches are NOT case sensitive. It\'s order - arbitrary.\n"					\
"Program NOT tested with file names that contains non-ASCII chars.\n"


#define CALLBACK_ABORT 0
#define CALLBACK_OK    1

#define TODO_FUNCLIST	0
#define TODO_LIST		1
#define TODO_TEST		2
#define TODO_EXTRACT	3
#define TODO_CONFIG 	4

#define ERR_NO_WCX		1
#define ERR_NO_PROC		2
#define ERR_OPEN_FAIL	3
#define ERR_CLOSE_FAIL	4


typedef HANDLE	(__stdcall* OpenArchiveProc)(tOpenArchiveData* ArchiveData);
typedef HANDLE	(__stdcall* OpenArchiveWProc)(tOpenArchiveDataW* ArchiveDataW);
typedef int		(__stdcall* ReadHeaderProc)(HANDLE hArcData, tHeaderData* HeaderData);
typedef int		(__stdcall* ReadHeaderExProc)(HANDLE hArcData, tHeaderDataEx* HeaderDataEx);
typedef int		(__stdcall* ReadHeaderExWProc)(HANDLE hArcData, tHeaderDataExW* HeaderDataExW);
typedef int		(__stdcall* ProcessFileProc)(HANDLE hArcData, int Operation, char* DestPath, char* DestName);
typedef int		(__stdcall* ProcessFileWProc)(HANDLE hArcData, int Operation, WCHAR* DestPath, WCHAR* DestName);
typedef int		(__stdcall* CloseArchiveProc)(HANDLE hArcData);
typedef int		(__stdcall* PackFilesProc)(char* PackedFile, char* SubPath, char* SrcPath, char* AddList, int Flags);
typedef int		(__stdcall* PackFilesWProc)(WCHAR* PackedFile, WCHAR* SubPath, WCHAR* SrcPath, WCHAR* AddList, int Flags);
typedef int		(__stdcall* DeleteFilesProc)(char* PackedFile, char* DeleteList);
typedef int		(__stdcall* DeleteFilesWProc)(WCHAR* PackedFile, WCHAR* DeleteList);
typedef int		(__stdcall* GetPackerCapsProc)(void);
typedef void	(__stdcall* ConfigurePackerProc)(HWND Parent, HINSTANCE DllInstance);
typedef void	(__stdcall* SetChangeVolProcProc)(HANDLE hArcData, tChangeVolProc pChangeVolProc1);		
typedef void	(__stdcall* SetChangeVolProcWProc)(HANDLE hArcData, tChangeVolProcW pChangeVolProcW);
typedef void	(__stdcall* SetProcessDataProcProc)(HANDLE hArcData, tProcessDataProc pProcessDataProc); 
typedef void	(__stdcall* SetProcessDataProcWProc)(HANDLE hArcData, tProcessDataProcW pProcessDataProcW);
typedef int		(__stdcall* StartMemPackProc)(int Options, char *FileName);
typedef int		(__stdcall* PackToMemProc)(int hMemPack, char* BufIn, int InLen, int* Taken, char* BufOut, int OutLen, int* Written, int SeekBy);
typedef int		(__stdcall* DoneMemPackProc)(int hMemPack);
typedef BOOL	(__stdcall* CanYouHandleThisFileProc)(char *FileName);
typedef BOOL	(__stdcall* CanYouHandleThisFileWProc)(WCHAR* FileName);
typedef void	(__stdcall* PackSetDefaultParamsProc)(PackDefaultParamStruct* dps);
typedef int		(__stdcall* GetBackgroundFlagsProc)(void);

/* mandatory functions */
static OpenArchiveProc pOpenArchive = nullptr;
static OpenArchiveWProc pOpenArchiveW = nullptr;
static ReadHeaderProc pReadHeader = nullptr;
static ReadHeaderExProc pReadHeaderEx = nullptr;
static ReadHeaderExWProc pReadHeaderExW = nullptr;
static ProcessFileProc pProcessFile = nullptr;
static ProcessFileWProc pProcessFileW = nullptr;
static CloseArchiveProc pCloseArchive = nullptr;
/* optional functions */
static PackFilesProc pPackFiles = nullptr;
static PackFilesWProc pPackFilesW = nullptr;
static DeleteFilesProc pDeleteFiles = nullptr;
static DeleteFilesWProc pDeleteFilesW = nullptr;
static GetPackerCapsProc pGetPackerCaps = nullptr;
static ConfigurePackerProc pConfigurePacker = nullptr;
static SetChangeVolProcProc pSetChangeVolProc = nullptr;		 /* NOT quite */
static SetChangeVolProcWProc pSetChangeVolProcW = nullptr;
static SetProcessDataProcProc pSetProcessDataProc = nullptr; /* NOT quite */
static SetProcessDataProcWProc pSetProcessDataProcW = nullptr; /* NOT quite */
static GetBackgroundFlagsProc pGetBackgroundFlags = nullptr;
/* packing into memory */
static StartMemPackProc pStartMemPack = nullptr;
static PackToMemProc pPackToMem = nullptr;
static DoneMemPackProc pDoneMemPack = nullptr;
static CanYouHandleThisFileProc pCanYouHandleThisFile = nullptr;
static CanYouHandleThisFileWProc pCanYouHandleThisFileW = nullptr;
static PackSetDefaultParamsProc pPackSetDefaultParams = nullptr;


// WCX plugin filename with or wothour path. From command line.
static char wcx_path[MAX_PATH] = "";
static WCHAR wcx_pathW[MAX_PATH] = L"";

// path to archive. WCS will work with this archive. From command line.
static char arc_path[MAX_PATH] = "";
static WCHAR arc_pathW[MAX_PATH] = L"";

// directory where to put extracted files. from command line.
static char dir_path[MAX_PATH] = ".\\";
static WCHAR dir_pathW[MAX_PATH] = L".\\";

static char prog_prefix[] = "WT: ";
static WCHAR prog_prefixW[] = L"WT: ";

//what wcxtext app should do with WCX. See PROG_USAGE for more details. Defined by command line params (switches)
static int	open_todo = -1;
static int	verbose = 0;


static char * WCX_err_msg(int code)
{
	static char		buf[256];

	switch(code) {
		case E_END_ARCHIVE:		strcpy(buf, "No more files in archive");	break;
		case E_NO_MEMORY:		strcpy(buf, "Not enough memory");			break;
		case E_BAD_DATA:		strcpy(buf, "Data is bad");					break;
		case E_BAD_ARCHIVE:		strcpy(buf, "CRC error in archive data");	break;
		case E_UNKNOWN_FORMAT:	strcpy(buf, "Archive format unknown");		break;
		case E_EOPEN:			strcpy(buf, "Cannot open existing file");	break;
		case E_ECREATE:			strcpy(buf, "Cannot create file");			break;
		case E_ECLOSE:			strcpy(buf, "Error closing file");			break;
		case E_EREAD:			strcpy(buf, "Error reading from file");		break;
		case E_EWRITE:			strcpy(buf, "Error writing to file");		break;
		case E_SMALL_BUF:		strcpy(buf, "Buffer too small");			break;
		case E_EABORTED:		strcpy(buf, "Function aborted by user");	break;
		case E_NO_FILES:		strcpy(buf, "No files found");				break;
		case E_TOO_MANY_FILES:	strcpy(buf, "Too many files to pack");		break;
		case E_NOT_SUPPORTED:	strcpy(buf, "Function not supported");		break;

		default: sprintf(buf, "Unknown error code (%d)", code); break;
	}

	return buf;
}

static WCHAR* WCX_err_msgW(int code)
{
	static WCHAR buf[256];

	switch (code) 
	{
	case E_END_ARCHIVE:		wcscpy(buf, L"No more files in archive");	break;
	case E_NO_MEMORY:		wcscpy(buf, L"Not enough memory");			break;
	case E_BAD_DATA:		wcscpy(buf, L"Data is bad");					break;
	case E_BAD_ARCHIVE:		wcscpy(buf, L"CRC error in archive data");	break;
	case E_UNKNOWN_FORMAT:	wcscpy(buf, L"Archive format unknown");		break;
	case E_EOPEN:			wcscpy(buf, L"Cannot open existing file");	break;
	case E_ECREATE:			wcscpy(buf, L"Cannot create file");			break;
	case E_ECLOSE:			wcscpy(buf, L"Error closing file");			break;
	case E_EREAD:			wcscpy(buf, L"Error reading from file");		break;
	case E_EWRITE:			wcscpy(buf, L"Error writing to file");		break;
	case E_SMALL_BUF:		wcscpy(buf, L"Buffer too small");			break;
	case E_EABORTED:		wcscpy(buf, L"Function aborted by user");	break;
	case E_NO_FILES:		wcscpy(buf, L"No files found");				break;
	case E_TOO_MANY_FILES:	wcscpy(buf, L"Too many files to pack");		break;
	case E_NOT_SUPPORTED:	wcscpy(buf, L"Function not supported");		break;

	default: wsprintfW(buf, L"Unknown error code (%d)", code); break;
	}

	return buf;
}

static int __stdcall ChangeVol(char *ArcName, int Mode)
{
	char	buf[32];
	int		rc = 0;

	switch(Mode) 
	{
		case PK_VOL_ASK:
		printf("%sPlease change disk and enter Y or N to stop: ", prog_prefix);
		gets_s<32>(buf);
		rc = *buf == 'y' || *buf == 'Y';
		break;

		case PK_VOL_NOTIFY:
		printf("%sProcessing next volume/diskette\n", prog_prefix);
		rc = 1;
		break;

		default:
		printf("%sUnknown ChangeVolProc mode\n", prog_prefix);
		rc = 0;
		break;
	}

	return rc;
}

static int __stdcall ChangeVolW(WCHAR* ArcName, int Mode)
{
	char	buf[32];
	int		rc = 0;

	switch (Mode)
	{
	case PK_VOL_ASK:
		printf("%sPlease change disk and enter Y or N to stop: ", prog_prefix);
		gets_s<32>(buf);
		rc = *buf == 'y' || *buf == 'Y';
		break;

	case PK_VOL_NOTIFY:
		printf("%sProcessing next volume/diskette\n", prog_prefix);
		rc = 1;
		break;

	default:
		printf("%sUnknown ChangeVolProc mode\n", prog_prefix);
		rc = 0;
		break;
	}

	return rc;
}

static int __stdcall ProcessData(char *FileName, int Size)
{
	char buf[MAX_PATH];

	CharToOem(FileName, buf);
	printf("%sProcessing %s (%d). Ok.\n", prog_prefix, buf, Size); 
	fflush(stdout);
	printf(".");
	return CALLBACK_OK; // 1;
}

static int __stdcall ProcessDataW(WCHAR* FileName, int Size)
{
	char buf[MAX_PATH];
	CharToOemW(FileName, buf);
	if(Size > 0)
		printf("%sProcessed %s (%d bytes). Ok.\n", prog_prefix, buf, Size); 
	else
		printf("%sProgress (%s): %d%%\n", prog_prefix, buf, -Size);
	printf(".");
	fflush(stdout);
	return CALLBACK_OK; // 1;
}

#define DIR_SEPARATOR '\\'
#define DRV_SEPARATOR ':'

void check_fndir(char *fname)
{
	struct _stat sb;
	char *s, buf[MAX_PATH];

	/* check if dir exists; if not create */
	for(s = fname; *s; ) 
	{
		if(*s == DIR_SEPARATOR) ++s;
		while (*s && *s != DIR_SEPARATOR) ++s;
		if(*s == DIR_SEPARATOR) 
		{
			*s = 0;
			/* there is no difference in speed: check if exists directory */
			if(_stat(fname, & sb) == -1) 
			{
				CharToOem(fname, buf);
				if(verbose) printf("%s-- Making dir %s\n", prog_prefix, buf);
				mkdir(fname);
			}
			*s = DIR_SEPARATOR;
		}
	}
}

#define DIR_SEPARATORW L'\\'
#define DRV_SEPARATORW L':'

void check_fndirW(WCHAR* fname)
{
	struct _stat	sb;
	WCHAR* s, buf[MAX_PATH];

	/* check if dir exists; if not create */
	for (s = fname; *s; ) {
		if (*s == DIR_SEPARATORW) ++s;
		while (*s && *s != DIR_SEPARATORW) ++s;
		if (*s == DIR_SEPARATORW)
		{
			*s = 0;
			/* there is no difference in speed: check if exists directory */
			if (_wstat(fname, &sb) == -1)
			{
				//CharToOem(fname, buf);
				if (verbose) wprintf(L"%s-- Making dir %s\n", prog_prefixW, buf);
				_wmkdir(fname);
			}
			*s = DIR_SEPARATORW;
		}
	}
}

// assuming that dest buffer is large enough
void CharToWCHAR(WCHAR* dest, const char* src)
{
	// size_t len;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dest, MAX_PATH);
	// mbstowcs_s(&len, dest, MAX_PATH, src, strlen(src));
	// if (len > 0u) dest[len] = '\0';
}

// assuming that dest buffer is large enough
void WCHARtoChar(char* dest, WCHAR* src)
{
	//size_t len;
	WideCharToMultiByte(CP_ACP, 0, src, -1, dest, MAX_PATH, NULL, NULL);
	//wcstombs_s(&len, dest, MAX_PATH, src, wcslen(src));
	//if (len > 0u) dest[len] = '\0';
}

int ProcessFile(HANDLE hArchive, tHeaderData& hdrd)
{
	if (!pReadHeader) return E_NOT_SUPPORTED; // incorrect situation, function should NOT be called when pReadHeader=NULL
	int rc = pReadHeader(hArchive, &hdrd); // zero-OK, nonzero-error
	if (rc) return rc;

	char oemFileName[1024];
	int		pfrc;

	CharToOem(hdrd.FileName, oemFileName);
	switch (open_todo)
	{
	case TODO_LIST:
		printf("%s%9u  %04u/%02u/%02u %02u:%02u:%02u  %c%c%c%c%c%c	%s", prog_prefix, hdrd.UnpSize,
			((hdrd.FileTime >> 25 & 0x7f) + 1980), hdrd.FileTime >> 21 & 0x0f, hdrd.FileTime >> 16 & 0x1f,
			hdrd.FileTime >> 11 & 0x1f, hdrd.FileTime >> 5 & 0x3f, (hdrd.FileTime & 0x1F) * 2,
			hdrd.FileAttr & 0x01 ? 'r' : '-',
			hdrd.FileAttr & 0x02 ? 'h' : '-',
			hdrd.FileAttr & 0x04 ? 's' : '-',
			hdrd.FileAttr & 0x08 ? 'v' : '-',
			hdrd.FileAttr & 0x10 ? 'd' : '-',
			hdrd.FileAttr & 0x20 ? 'a' : '-',
			oemFileName);
		fflush(stdout);
		pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		if (pfrc) 
		{
			printf(" - Error! %s\n", WCX_err_msg(pfrc));
			return pfrc;
			//goto stop;
		}
		else printf("\n");
		fflush(stdout);
		break;

	case TODO_TEST:
		if (!(hdrd.FileAttr & 0x10)) 
		{
			printf("%s%s ", prog_prefix, oemFileName);
			pfrc = pProcessFile(hArchive, PK_TEST, NULL, NULL);
			if (pfrc) 
			{
				printf("Error! %s\n", WCX_err_msg(pfrc));
				return pfrc;
				//goto stop;
			}
			else printf("Ok.\n");
			fflush(stdout);
		}
		else 
		{
			pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		}
		break;

	case TODO_EXTRACT:
		if (!(hdrd.FileAttr & 0x10)) 
		{
			char outpath[MAX_PATH];

			sprintf(outpath, "%s%s", dir_path, hdrd.FileName);
			check_fndir(outpath);
			printf("%s%s ", prog_prefix, oemFileName);
			pfrc = pProcessFile(hArchive, PK_EXTRACT, NULL, outpath);
			if (pfrc) 
			{
				printf("\nError! %s (%s)\n", WCX_err_msg(pfrc), outpath);
				return pfrc;
				//goto stop;
			}
			else printf("Ok.\n");
			fflush(stdout);
		}
		else 
		{
			pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		}
		break;

	default:
		printf("%sUnknown TODO\n", prog_prefix); fflush(stdout);
		//rc = ERR_OPEN_FAIL;
		return ERR_OPEN_FAIL;
		//goto stop;
	}

	return 0;
}

int ProcessFileEx(HANDLE hArchive, tHeaderDataEx& hdrd)
{
	if (!pReadHeaderEx) return E_NOT_SUPPORTED; // incorrect situation, function should NOT be called when pReadHeader=NULL
	int rc = pReadHeaderEx(hArchive, &hdrd); // zero-OK, nonzero-error
	if (rc) return rc;

	char oemFileName[1024];
	int		pfrc;

	CharToOem(hdrd.FileName, oemFileName);
	switch (open_todo)
	{
	case TODO_LIST:
		printf("%s%9u  %04u/%02u/%02u %02u:%02u:%02u  %c%c%c%c%c%c	%s", prog_prefix, hdrd.UnpSize,
			((hdrd.FileTime >> 25 & 0x7f) + 1980), hdrd.FileTime >> 21 & 0x0f, hdrd.FileTime >> 16 & 0x1f,
			hdrd.FileTime >> 11 & 0x1f, hdrd.FileTime >> 5 & 0x3f, (hdrd.FileTime & 0x1F) * 2,
			hdrd.FileAttr & 0x01 ? 'r' : '-',
			hdrd.FileAttr & 0x02 ? 'h' : '-',
			hdrd.FileAttr & 0x04 ? 's' : '-',
			hdrd.FileAttr & 0x08 ? 'v' : '-',
			hdrd.FileAttr & 0x10 ? 'd' : '-',
			hdrd.FileAttr & 0x20 ? 'a' : '-',
			oemFileName);
		fflush(stdout);
		pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		if (pfrc)
		{
			printf(" - Error! %s\n", WCX_err_msg(pfrc));
			return pfrc;
			//goto stop;
		}
		else printf("\n");
		fflush(stdout);
		break;

	case TODO_TEST:
		if (!(hdrd.FileAttr & 0x10))
		{
			printf("%s%s ", prog_prefix, oemFileName);
			pfrc = pProcessFile(hArchive, PK_TEST, NULL, NULL);
			if (pfrc)
			{
				printf("Error! %s\n", WCX_err_msg(pfrc));
				return pfrc;
				//goto stop;
			}
			else printf("Ok.\n");
			fflush(stdout);
		}
		else
		{
			pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		}
		break;

	case TODO_EXTRACT:
		if (!(hdrd.FileAttr & 0x10))
		{
			char outpath[MAX_PATH];

			sprintf(outpath, "%s%s", dir_path, hdrd.FileName);
			check_fndir(outpath);
			printf("%s%s ", prog_prefix, oemFileName);
			pfrc = pProcessFile(hArchive, PK_EXTRACT, NULL, outpath);
			if (pfrc)
			{
				printf("\nError! %s (%s)\n", WCX_err_msg(pfrc), outpath);
				return pfrc;
				//goto stop;
			}
			else printf("Ok.\n");
			fflush(stdout);
		}
		else
		{
			pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL);
		}
		break;

	default:
		printf("%sUnknown TODO\n", prog_prefix); fflush(stdout);
		//rc = ERR_OPEN_FAIL;
		return ERR_OPEN_FAIL;
		//goto stop;
	}

	return 0;
}


int ProcessFileExW(HANDLE hArchive, tHeaderDataExW& hdrd)
{
	if (!pReadHeaderExW) return E_NOT_SUPPORTED; // incorrect situation, function should NOT be called when pReadHeader=NULL
	
	int rc = pReadHeaderExW(hArchive, &hdrd); // zero-OK, nonzero-error
	if (rc) return rc;

	char FileNameA[1024];
	int		pfrc;

	//CharToOem(hdrd.FileName, buf);
	WCHARtoChar(FileNameA, hdrd.FileName);

	switch (open_todo)
	{
	case TODO_LIST:
		printf("%s%9u  %04u/%02u/%02u %02u:%02u:%02u  %c%c%c%c%c%c	%s", prog_prefix, hdrd.UnpSize,
			((hdrd.FileTime >> 25 & 0x7f) + 1980), hdrd.FileTime >> 21 & 0x0f, hdrd.FileTime >> 16 & 0x1f,
			hdrd.FileTime >> 11 & 0x1f, hdrd.FileTime >> 5 & 0x3f, (hdrd.FileTime & 0x1F) * 2,
			hdrd.FileAttr & 0x01 ? 'r' : '-',
			hdrd.FileAttr & 0x02 ? 'h' : '-',
			hdrd.FileAttr & 0x04 ? 's' : '-',
			hdrd.FileAttr & 0x08 ? 'v' : '-',
			hdrd.FileAttr & 0x10 ? 'd' : '-',
			hdrd.FileAttr & 0x20 ? 'a' : '-',
			FileNameA);
		fflush(stdout);
		pfrc = pProcessFileW(hArchive, PK_SKIP, NULL, NULL);
		if (pfrc)
		{
			printf(" - Error! %s\n", WCX_err_msg(pfrc));
			return pfrc;
			//goto stop;
		}
		else 
			printf("\n");
		fflush(stdout);
		break;

	case TODO_TEST:
		if (!(hdrd.FileAttr & 0x10)) // file is not a directory
		{
			printf("%s%s ", prog_prefix, FileNameA);
			pfrc = pProcessFileW(hArchive, PK_TEST, NULL, NULL);
			if (pfrc)
			{
				printf("Error! %s\n", WCX_err_msg(pfrc));
				return pfrc;
				//goto stop;
			}
			else 
				printf("Ok.\n");
			fflush(stdout);
		}
		else
		{
			pfrc = pProcessFileW(hArchive, PK_SKIP, NULL, NULL); //skip directories
		}
		break;

	case TODO_EXTRACT:
		if (!(hdrd.FileAttr & 0x10))  // file is not a directory
		{
			WCHAR outpathW[MAX_PATH];
			//char outpathA[MAX_PATH];

			wsprintfW(outpathW, L"%s%s", dir_pathW, hdrd.FileName);
			//sprintf(outpathA, "%s%s", dir_path, FileNameA);

			check_fndirW(outpathW);
			printf("%sExtracting file '%s' \n", prog_prefix, FileNameA);
			pfrc = pProcessFileW(hArchive, PK_EXTRACT, NULL, outpathW);
			if (pfrc)
			{
				wprintf(L"\nError! %s (%s)\n", WCX_err_msgW(pfrc), outpathW);
				return pfrc;
				//goto stop;
			}
			else 
				printf("DONE.\n");
			fflush(stdout);
		}
		else
		{
			pfrc = pProcessFile(hArchive, PK_SKIP, NULL, NULL); // skip directories
		}
		break;

	default:
		printf("%sUnknown TODO\n", prog_prefix); 
		fflush(stdout);
		//rc = ERR_OPEN_FAIL;
		return ERR_OPEN_FAIL;
		//goto stop;
	}

	return 0;
}

//extern 
int main(int argc, char *argv[])
{
	int					i, j, rc = 0;
	char* s; // , buf[1024];
	HINSTANCE			hwcx = NULL;
	HANDLE				hArchive = NULL;
	tOpenArchiveData	arcd;
	tOpenArchiveDataW	arcdW;
	tHeaderData			hdrd{0};
	tHeaderDataEx		hdrdEx{0};
	tHeaderDataExW		hdrdExW{0};

	if (argc < 2) 
	{
		printf(PROG_LOGO);
		printf(PROG_USAGE);
		return 0;
	}

	if (argc == 2) open_todo = TODO_FUNCLIST;

	/* switches */
	for(i = 1; i < argc; ++i) 
	{
		s = argv[i];
		if(*s != '-' && *s != '/') continue;
		++s;
		switch(*s) {
			case 'f':
			case 'F': /* list of functions mode */
			if(open_todo < 0) 
				open_todo = TODO_FUNCLIST;
			else 
			{
				printf("Syntax error: too many switches.\n");
				printf(PROG_USAGE);
				return 0;
			}
			break;


			case 'c':
			case 'C': /* config mode */
				if (open_todo < 0)
					open_todo = TODO_CONFIG;
				else
				{
					printf("Syntax error: too many switches.\n");
					printf(PROG_USAGE);
					return 0;
				}
				break;



			case 'l':
			case 'L': /* list mode */
			if(open_todo < 0) 
				open_todo = TODO_LIST;
			else
			{
				printf("Syntax error: too many switches.\n");
				printf(PROG_USAGE);
				return 0;
			}
			break;

			case 't':
			case 'T': /* test mode */
			if(open_todo < 0) 
				open_todo = TODO_TEST;
			else 
			{
				printf("Syntax error: too many switches.\n");
				printf(PROG_USAGE);
				return 0;
			}
			break;

			case 'x':
			case 'X': /* extract mode */
			if(open_todo < 0) 
				open_todo = TODO_EXTRACT;
			else 
			{
				printf("Syntax error: too many switches.\n");
				printf(TODO_FUNCLIST);
				return 0;
			}
			break;

			case 'v':
			case 'V':
			verbose = 1;
			break;

			case '?':
			case 'h':
			case 'H':
			printf(PROG_LOGO);
			printf(TODO_FUNCLIST);
			return 0;

			default:
			printf("Syntax error: invalid switch.\n");
			printf(PROG_USAGE);
			return 0;
		}
	}

	if(open_todo < 0) open_todo = TODO_FUNCLIST;
	if(!verbose) *prog_prefix = 0;

	/* parameters */
	for(i = 1, j = 0; i < argc; ++i) {
		s = argv[i];
		if(*s == '-' || *s == '/') continue;
		switch(j) 
		{
			case 0:
			strcpy(wcx_path, argv[i]);
			CharToWCHAR(wcx_pathW, wcx_path);
			break;

			case 1:
			strcpy(arc_path, argv[i]);
			CharToWCHAR(arc_pathW, arc_path);
			break;

			case 2:
			strcpy(dir_path, argv[i]);
			CharToWCHAR(dir_pathW, dir_path);
			break;

			default:
			printf("Syntax error: too many arguments.\n");
			printf(PROG_USAGE);
			return 0;
		}
		++j;
	}

	if(!*wcx_path) 
	{
		printf("Syntax error: no WCX name.\n");
		printf(PROG_USAGE);
		return 0;
	}
	if(!*arc_path && (open_todo == TODO_LIST || open_todo == TODO_TEST || open_todo == TODO_EXTRACT)) 
	{
		printf("Syntax error: no archive name.\n");
		printf(PROG_USAGE);
		return 0;
	}
	if(*dir_path && dir_path[strlen(dir_path) - 1] != '\\') strcat(dir_path, "\\"); // make sure that dir_path ends with '\'

	if(verbose) 
	{
		switch(open_todo) 
		{
			case TODO_FUNCLIST:
			//printf("%sExported functions in \"%s\":\n", prog_prefix, wcx_path);
			break;

			case TODO_LIST:
			printf("%sUsing \"%s\" for list files in \"%s\".\n", prog_prefix, wcx_path, arc_path);
			break;

			case TODO_TEST:
			printf("%sUsing \"%s\" for test files in \"%s\".\n", prog_prefix, wcx_path, arc_path);
			break;

			case TODO_EXTRACT:
			printf("%sUsing \"%s\" for extract files from \"%s\" to \"%s\".\n", prog_prefix, wcx_path, arc_path, dir_path);
			break;

			case TODO_CONFIG:
			printf("%sUsing \"%s\" to configure settings.\n", prog_prefix, wcx_path);
			break;

			default:
			printf("unknown to do with");
			break;
		}
	}

	if(verbose) printf("%sLoading plugin %s... ", prog_prefix, wcx_path);
	if(!(hwcx = LoadLibrary(wcx_path))) 
	{
		if(verbose) 
			printf("Failed.\n");
		else
		{
			printf("Failed loading plugin '%s'.\n", wcx_path);
			PrintWindowsErrorMessage("LoadLibrary");
		}
		return ERR_NO_WCX;
	}

	if(verbose) printf("Ok.\n");

	/* mandatory */
	pOpenArchive =			(OpenArchiveProc)GetProcAddress(hwcx, "OpenArchive");
	pReadHeader =			(ReadHeaderProc)GetProcAddress(hwcx, "ReadHeader");
	pProcessFile =			(ProcessFileProc)GetProcAddress(hwcx, "ProcessFile");
	pCloseArchive =			(CloseArchiveProc)GetProcAddress(hwcx, "CloseArchive");
	pSetChangeVolProc =		(SetChangeVolProcProc)GetProcAddress(hwcx, "SetChangeVolProc");
	pSetProcessDataProc =	(SetProcessDataProcProc)GetProcAddress(hwcx, "SetProcessDataProc");
	/* optional */
	pOpenArchiveW =			(OpenArchiveWProc)GetProcAddress(hwcx, "OpenArchiveW");
	pReadHeaderEx =			(ReadHeaderExProc)GetProcAddress(hwcx, "ReadHeaderEx");
	pReadHeaderExW =		(ReadHeaderExWProc)GetProcAddress(hwcx, "ReadHeaderExW");
	pProcessFileW =			(ProcessFileWProc)GetProcAddress(hwcx, "ProcessFileW");
	pPackFiles =			(PackFilesProc)GetProcAddress(hwcx, "PackFiles");
	pPackFilesW =			(PackFilesWProc)GetProcAddress(hwcx, "PackFilesW");
	pDeleteFiles =			(DeleteFilesProc)GetProcAddress(hwcx, "DeleteFiles");
	pDeleteFilesW =			(DeleteFilesWProc)GetProcAddress(hwcx, "DeleteFilesW");
	pGetPackerCaps =		(GetPackerCapsProc)GetProcAddress(hwcx, "GetPackerCaps");
	pConfigurePacker =		(ConfigurePackerProc)GetProcAddress(hwcx, "ConfigurePacker");
	pSetChangeVolProcW =	(SetChangeVolProcWProc)GetProcAddress(hwcx, "SetChangeVolProcW");
	pSetProcessDataProcW =	(SetProcessDataProcWProc)GetProcAddress(hwcx, "SetProcessDataProcW");
	pStartMemPack =			(StartMemPackProc)GetProcAddress(hwcx, "StartMemPack");
	pPackToMem =			(PackToMemProc)GetProcAddress(hwcx, "PackToMem");
	pDoneMemPack =			(DoneMemPackProc)GetProcAddress(hwcx, "DoneMemPack");
	pCanYouHandleThisFile = (CanYouHandleThisFileProc)GetProcAddress(hwcx, "CanYouHandleThisFile");
	pCanYouHandleThisFileW= (CanYouHandleThisFileWProc)GetProcAddress(hwcx, "CanYouHandleThisFileW");
	pPackSetDefaultParams =	(PackSetDefaultParamsProc)GetProcAddress(hwcx, "PackSetDefaultParams");
	pGetBackgroundFlags =	(GetBackgroundFlagsProc)GetProcAddress(hwcx, "GetBackgroundFlags");

	if(open_todo == TODO_FUNCLIST) 
	{
		printf("%sExported WCX functions in \"%s\":\n", prog_prefix, wcx_path);
		if(pOpenArchive			) printf("%s  OpenArchive\n", prog_prefix);
		if(pOpenArchiveW		) printf("%s  OpenArchiveW\n", prog_prefix);
		if(pReadHeader			) printf("%s  ReadHeader\n", prog_prefix);
		if(pReadHeaderEx		) printf("%s  ReadHeaderEx\n", prog_prefix);
		if(pReadHeaderExW		) printf("%s  ReadHeaderExW\n", prog_prefix);
		if(pProcessFile			) printf("%s  ProcessFile\n", prog_prefix);
		if(pProcessFileW		) printf("%s  ProcessFileW\n", prog_prefix); 
		if(pCloseArchive		) printf("%s  CloseArchive\n", prog_prefix);
		if(pPackFiles			) printf("%s  PackFiles\n", prog_prefix);
		if(pPackFilesW			) printf("%s  PackFilesW\n", prog_prefix);
		if(pDeleteFiles			) printf("%s  DeleteFiles\n", prog_prefix);
		if(pDeleteFilesW		) printf("%s  DeleteFilesW\n", prog_prefix);
		if(pGetPackerCaps		) printf("%s  GetPackerCaps\n", prog_prefix);
		if(pConfigurePacker		) printf("%s  ConfigurePacker\n", prog_prefix);
		if(pSetChangeVolProc	) printf("%s  SetChangeVolProc\n", prog_prefix);
		if(pSetChangeVolProcW	) printf("%s  SetChangeVolProcW\n", prog_prefix);
		if(pSetProcessDataProc	) printf("%s  SetProcessDataProc\n", prog_prefix);
		if(pSetProcessDataProcW	) printf("%s  SetProcessDataProcW\n", prog_prefix);
		if(pStartMemPack		) printf("%s  StartMemPack\n", prog_prefix);
		if(pPackToMem			) printf("%s  PackToMem\n", prog_prefix);
		if(pDoneMemPack			) printf("%s  DoneMemPack\n", prog_prefix);
		if(pCanYouHandleThisFile) printf("%s  CanYouHandleThisFile\n", prog_prefix);
		if(pCanYouHandleThisFileW)printf("%s  CanYouHandleThisFileW\n", prog_prefix);
		if(pPackSetDefaultParams) printf("%s  PackSetDefaultParams\n", prog_prefix);
		if(pGetBackgroundFlags)   printf("%s  GetBackGroundFlags\n", prog_prefix);

		if(pGetPackerCaps) 
		{
			int	pc = pGetPackerCaps(), f = 0;

			printf("%sPackerCaps: %u =", prog_prefix, pc);
			if(pc & PK_CAPS_NEW			) { printf("%s PK_CAPS_NEW", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_MODIFY		) { printf("%s PK_CAPS_MODIFY", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_MULTIPLE	) { printf("%s PK_CAPS_MULTIPLE", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_DELETE		) { printf("%s PK_CAPS_DELETE", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_OPTIONS		) { printf("%s PK_CAPS_OPTIONS", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_MEMPACK		) { printf("%s PK_CAPS_MEMPACK", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_BY_CONTENT	) { printf("%s PK_CAPS_BY_CONTENT", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_SEARCHTEXT	) { printf("%s PK_CAPS_SEARCHTEXT", f ? " |" : ""); f = 1; }
			if(pc & PK_CAPS_HIDE		) { printf("%s PK_CAPS_HIDE", f ? " |" : ""); f = 1; }
			printf("\n");
		}

		if (pGetBackgroundFlags)
		{
			int	pc = pGetBackgroundFlags(), f = 0;
			printf("%sBackgroundFlags: %u =", prog_prefix, pc);
			if (pc & BACKGROUND_PACK) { printf("%s BACKGROUND_PACK", f ? " |" : ""); f = 1; }
			if (pc & BACKGROUND_UNPACK) { printf("%s BACKGROUND_UNPACK", f ? " |" : ""); f = 1; }
			if (pc & BACKGROUND_MEMPACK) { printf("%s BACKGROUND_MEMPACK", f ? " |" : ""); f = 1; }
			printf("\n");
		}

		goto stop;
	}

	// function OpenArchive must exist in any case. if OpenArchiveW exists then it will be called, but OpenArchive must exist too for proper TC work
	// function ReadHeader must exist in WCX even is you use ReadHeaderEx or ReadHeaderExW
	// SetProcessDataProc must exist in WCX. if both SetProcessDataProc and SetProcessDataProcW present -  both of them are called by TC for some reason
	if ( !(pOpenArchive && pReadHeader && pProcessFile && pCloseArchive && pSetChangeVolProc && pSetProcessDataProc) )
	{
		printf("%sError: There IS NOT mandatory function(s):", prog_prefix);
		if (!pOpenArchive) printf(" OpenArchive");
		if (!pReadHeader) printf(", ReadHeader ");
		if (!pProcessFile) printf(", ProcessFile");
		if (!pCloseArchive) printf(", CloseArchive");
		if (!pSetChangeVolProc) printf(" SetChangeVolProc");
		if (!pSetProcessDataProc) printf(", SetProcessDataProc");
		printf("\n");
		rc = ERR_NO_PROC;
		goto stop;
	}

	if (open_todo == TODO_CONFIG)
	{
		if (verbose) printf("%sOpening configure settings dialog %s... ", prog_prefix, arc_path);
		//if (verbose) printf("%s--------\n", prog_prefix);
		pConfigurePacker(NULL, hwcx);
		if (verbose) printf("Done.\n");
		goto stop;
	}

	// after loading WCX TC always calls PackSetDefaultParams before any OpenArchive or PackFiles call.
	if (pPackSetDefaultParams)
	{
		PackDefaultParamStruct dps;
		dps.PluginInterfaceVersionHi = 2; // latest wcx plugin API version on Aug 2024
		dps.PluginInterfaceVersionLow = 22;
		strcpy_s(dps.DefaultIniName, sizeof(dps.DefaultIniName), "TestIniFileName.ini");
		if (verbose) printf("%sCalling PackSetDefaultParams. IniFile: %s Version: %d.%d ... ", prog_prefix, dps.DefaultIniName, dps.PluginInterfaceVersionHi, dps.PluginInterfaceVersionLow);
		pPackSetDefaultParams(&dps);
		if (verbose) printf("Ok.\n");
	}

	if(verbose) printf("%sOpening archive %s...", prog_prefix, arc_path);
	memset(&arcd, 0, sizeof(arcd));
	memset(&arcdW, 0, sizeof(arcdW));
	arcd.ArcName = arc_path;
	arcdW.ArcName = arc_pathW;
	switch(open_todo) 
	{
		case TODO_LIST:
		arcd.OpenMode = PK_OM_LIST;
		arcdW.OpenMode = PK_OM_LIST;
		break;

		case TODO_TEST:
		case TODO_EXTRACT:
		arcd.OpenMode = PK_OM_EXTRACT;
		arcdW.OpenMode = PK_OM_EXTRACT;
		break;

		default:
		printf("%sUnknown TODO\n", prog_prefix);
		rc = ERR_OPEN_FAIL;
		goto stop;
	}

	if (pOpenArchiveW)
	{
		hArchive = pOpenArchiveW(&arcdW);
		if (!hArchive)
		{
			if (verbose)
				printf("%sFailed: %s\n", prog_prefix, WCX_err_msg(arcdW.OpenResult));
			else
				printf("%sFailed opening archive: %s\n", prog_prefix, WCX_err_msg(arcdW.OpenResult));
			rc = ERR_OPEN_FAIL;
			goto stop;
		}
	}
	else if (pOpenArchive)
	{
		hArchive = pOpenArchive(&arcd);
		//if(!(hArchive = pOpenArchive(&arcd))) 
		if (!hArchive)
		{
			if (verbose)
				printf("%sFailed: %s\n", prog_prefix, WCX_err_msg(arcd.OpenResult));
			else
				printf("%sFailed opening archive: %s\n", prog_prefix, WCX_err_msg(arcd.OpenResult));
			rc = ERR_OPEN_FAIL;
			goto stop;
		}
	}
	if (verbose) printf("Ok\n");

	if(verbose) printf("%sHandle returned by WCX: %p\n", prog_prefix, hArchive), fflush(stdout);

	if (pSetChangeVolProcW) pSetChangeVolProcW(hArchive, ChangeVolW);
	else if (pSetChangeVolProc) pSetChangeVolProc(hArchive, ChangeVol);

	if(pSetProcessDataProcW) pSetProcessDataProcW(hArchive, ProcessDataW);
	else if (pSetProcessDataProc) pSetProcessDataProc(hArchive, ProcessData);

	switch (open_todo)
	{
	case TODO_LIST:
		if (verbose) printf("%sList of files in %s\n", prog_prefix, arc_path);
		printf("%s Length    YYYY/MM/DD HH:MM:SS   Attr   Name\n", prog_prefix);
		printf("%s---------  ---------- --------  ------  ------------\n", prog_prefix);
		break;

	case TODO_TEST:
		if (verbose) printf("%sTesting files in %s\n", prog_prefix, arc_path);
		if (verbose) printf("%s--------\n", prog_prefix);
		break;

	case TODO_EXTRACT:
		if (verbose) printf("%sExtracting files from %s\n", prog_prefix, arc_path);
		if (verbose) printf("%s--------\n", prog_prefix);
		break;

	default:
		printf("%sUnknown TODO\n", prog_prefix);
		rc = ERR_OPEN_FAIL;
		goto stop;
	}

	/* main loop */
	//while(!(rc = pReadHeader(hArchive, &hdrd))) 
	while(1)
	{
		if (pReadHeaderExW) rc = ProcessFileExW(hArchive, hdrdExW);
		else if (pReadHeaderEx) rc = ProcessFileEx(hArchive, hdrdEx);
		else if (pReadHeader) rc = ProcessFile(hArchive, hdrd);

		if (rc) break;
	}

	if(verbose) printf("%s--------\n", prog_prefix);
	if(verbose) printf("%s%s\n", prog_prefix, WCX_err_msg(rc)); 
	fflush(stdout);
	if(rc == E_END_ARCHIVE) rc = 0;

	/* cleanup */
stop:
	if(hArchive) 
	{
		if(verbose) 
		{
			printf("%sClosing archive... ", prog_prefix);
			fflush(stdout);
		}
		if(pCloseArchive(hArchive)) 
		{
			if(verbose) printf("Failed: %d\n", rc); 
			else printf("Failed closing archive: %d\n", rc);
			fflush(stdout);
			rc = ERR_CLOSE_FAIL;
		} else 
		{
			if(verbose) 
			{
				printf("Ok.\n");
				fflush(stdout);
			}
			hArchive = NULL;
		}
	}

	if(hwcx) 
	{
		if(verbose) 
		{
			printf("%sUnloading plugin... ", prog_prefix);
			fflush(stdout);
		}
		if(!FreeLibrary(hwcx)) 
		{
			if(verbose) printf("Failed.\n");
			else printf("Failed unloading plugin.\n");
			fflush(stdout);
			rc = ERR_NO_WCX;
		}
		else 
		{
			if(verbose) 
			{
				printf("Ok.\n");
				fflush(stdout);
			}
			hwcx = NULL;
		}
	}

	if(verbose) 
	{
		printf("%sERRORLEVEL: %d\n", prog_prefix, rc);
		fflush(stdout);
	}
	return rc;
}
