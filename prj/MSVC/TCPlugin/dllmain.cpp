// dllmain.cpp : Defines the entry point for the DLL application.

#include <io.h>
//#include <fcntl.h>
#include <chrono>
//#include <source_location>
#include "framework.h"
#include "wcxhead.h"
#include "LogEngine.h"
#include "Archiver.h"
#include "TCCallback.h"
#include "resource.h"
#include "Utils.h"


using namespace std;

static const char* FileOperations[3] = { "PK_SKIP", "PK_TEST", "PK_EXTRACT" };
static const char* ArchiveOpenMode[2] = { "PK_OM_LIST", "PK_OM_EXTRACT" };

#define ARCHIVES_MAX 10
#define INVALID_ARC_HANDLE 0
#define BIG_FILESIZE_MASK 0xFFFFFFFF
#define FUNC std::string("[" __FUNCTION__ "]")

#define IsOperationCorrect(_) (_ >= PK_SKIP && _ <= PK_EXTRACT)
#define IsOpenModeCorrect(_) (_ >= PK_OM_LIST && _ <= PK_OM_EXTRACT)
#define IsArcDataCorrect(_) (_ > 0 && _ <= ARCHIVES_MAX)

typedef struct
{
    char ArchiveName[260];
    wchar_t ArchiveNameW[260];
    vect_fr_t ArchiveFiles;
    bool ArchiveOpen;
    int ArchivePos;
    tChangeVolProc ChangeVolProc;
    tChangeVolProcW ChangeVolProcW;
    tProcessDataProc ProcessDataProc;
    tProcessDataProcW ProcessDataProcW;
} ArchiveRec;


CRITICAL_SECTION slotcriticalsection;
//BOOL slotcriticalsectioninitialized = false;

// Keep a list of currently open archives (up to a maximum of ARCHIVES_MAX) 
static ArchiveRec ArchiveList[ARCHIVES_MAX + 1];  // 0 for packing, 1..ARCHIVES_MAX for unpacking! We cannot use index 0 for unpacking because it is a handle 

// Compression parmeters for new archives. Loaded from special TC .ini file when plugin is initialised
PluginParameters ARPackParams;

#undef LOG_INFO
#undef LOG_WARN
#undef LOG_ERROR

#define LOG_INFO(_)  LogEngine::GetLogger(LOGGER_NAME).Info(_)
#define LOG_WARN(_)  LogEngine::GetLogger(LOGGER_NAME).Warn(_)
#define LOG_ERROR(_) LogEngine::GetLogger(LOGGER_NAME).Error(_)

// Default ini file name where plugin stores its parameters. Path to this ini file is got from PackSetDefaultParams call 
#define DEF_INIFILENAME _T("wcx_arpacker.ini")
char_t IniFileName[MAX_PATH] = DEF_INIFILENAME;

//Божена скажи пожалуйста как Тим Романченко может дать сроки заказчику если мы не знаем какой командой вы планируете делать проект, какие взаимосвязи между задачами и т.п.

void InitLogger()
{
    char_t logfn[MAX_PATH];
    // this call will not actually read inifile because it is not initialized at this moment of time
    GetPrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_LOGFILENAME, LOGGER_FILE_NAME, logfn, MAX_PATH, IniFileName);
    auto lastErr = GetLastError();

    LogEngine::SetProperty(APPNAME_PROPERTY, "ARPacker Total Commander plug-in");
    LogEngine::Logger& logger = LogEngine::GetFileLogger(LOGGER_NAME, convert_string<char>(logfn));
    LogEngine::PatternLayout* lay = dynamic_cast<LogEngine::PatternLayout*>(logger.GetSink(LOGGER_NAME)->GetLayout());
    lay->SetAllPatterns("%LOGLEVEL% %DATETIME% #%THREAD% : %MSG%");
    logger.Info("LogEngine STARTED");
    if(lastErr == ERROR_FILE_NOT_FOUND) logger.WarnFmt("Ini file '{}' not found.", convert_string<char>(IniFileName));
}

void ShutDownLogger()
{
    LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);
    logger.Info("LogEngine STOPPED");
    LogEngine::ClearLoggers();
}



BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);
    UNREFERENCED_PARAMETER(hModule);
    //DWORD thr;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitLogger();
        LOG_INFO("DLL_PROCESS_ATTACH called");
        memset(ArchiveList, 0, sizeof(ArchiveRec)* (ARCHIVES_MAX + 1)); 
        InitializeCriticalSection(&slotcriticalsection);
        break;
    case DLL_THREAD_ATTACH:
       // LOG_INFO("DLL_THREAD_ATTACH called"); // called too often in TC
        break;
    case DLL_THREAD_DETACH:
       // LOG_INFO("DLL_THREAD_DETACH called"); // called too often in TC
        break;
    case DLL_PROCESS_DETACH: 
        LOG_INFO("DLL_PROCESS_DETACH called");
        ShutDownLogger();
        DeleteCriticalSection(&slotcriticalsection);
        break;
    }

    return TRUE;
}

//this is mandatory function that must present in plugin
int __stdcall OpenArchive(tOpenArchiveData* ArchiveData)
{ 
    LOG_WARN("WEIRD: [OpenArchive] called however it should not be called");

    ArchiveData->OpenResult = E_EOPEN;
    return INVALID_ARC_HANDLE;
}

int __stdcall OpenArchiveW(tOpenArchiveDataW* ArchiveData)
{
    LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);

    char ArcNameA[MAX_PATH];
    WCHARtoChar(ArcNameA, ArchiveData->ArcName);

    if(IsOpenModeCorrect(ArchiveData->OpenMode))
        logger.InfoFmt("{} OpenMode={} ArcName={}", FUNC, ArchiveOpenMode[ArchiveData->OpenMode], ArcNameA);
    else
        logger.InfoFmt("{} OpenMode={} ArcName={}", FUNC, ArchiveData->OpenMode, ArcNameA);

    bool foundslot = false;
    int ArcHandle = INVALID_ARC_HANDLE;
    EnterCriticalSection(&slotcriticalsection);
    for (int i = 1; i <= ARCHIVES_MAX; i++)
        if (!ArchiveList[i].ArchiveOpen) // free slot
        {
            foundslot = true;
            ArcHandle = i;
            break;
        }
    LeaveCriticalSection(&slotcriticalsection);

    if (!foundslot) 
    {
        ArchiveData->OpenResult = E_NO_MEMORY;
        return INVALID_ARC_HANDLE;
    }

    ifstream fin(ArchiveData->ArcName, ios::in | ios::binary, _SH_DENYNO); // this is WCHAR version of constructor
    if (!fin)
    {
        logger.ErrorFmt("{} Cannot open archive file '{}' for reading.", FUNC, ArcNameA);        
        ArchiveData->OpenResult = E_EOPEN;
        return INVALID_ARC_HANDLE;
    }

    try
    {
        ArchiveHeader aheader;
        ArchiveList[ArcHandle].ArchiveFiles.clear();
        ArchiveList[ArcHandle].ArchiveFiles = aheader.LoadHeader(&fin);
    }
    catch (...) // wrong archive format
    {
        logger.ErrorFmt("{} Error reading archive file '{}'.", FUNC, ArcNameA);

        ArchiveList[ArcHandle].ArchiveFiles.clear();
        ArchiveData->OpenResult = E_BAD_ARCHIVE;
        return INVALID_ARC_HANDLE;
    }

    
    strcpy_s(ArchiveList[ArcHandle].ArchiveName, MAX_PATH, ArcNameA);
    wcscpy_s(ArchiveList[ArcHandle].ArchiveNameW, MAX_PATH, ArchiveData->ArcName);
    ArchiveList[ArcHandle].ArchiveOpen = true;
    ArchiveList[ArcHandle].ArchivePos = 0; 
    
    ArchiveData->OpenResult = 0;

    return ArcHandle;  // return pseudohandle 
}

int __stdcall CloseArchive(int hArcData)
{
    LOG_INFO(FUNC + " called");

    if (IsArcDataCorrect(hArcData)) 
    {
        ArchiveList[hArcData].ArchiveFiles.clear();
        ArchiveList[hArcData].ArchiveOpen = false;
        return 0;
    }
    else
        return E_ECLOSE;
}

//this is mandatory function that must present in plugin 
int __stdcall ReadHeader(int hArcData, tHeaderData* HeaderData)
{
    UNREFERENCED_PARAMETER(hArcData);
    UNREFERENCED_PARAMETER(HeaderData);
    LOG_WARN("WEIRD: [ReadHeader] called however it should not be called");

    return E_EREAD;
}

//int __stdcall ReadHeaderEx(int hArcData, tHeaderDataEx* HeaderDataEx)
//{
//    UNREFERENCED_PARAMETER(hArcData);
//    UNREFERENCED_PARAMETER(HeaderDataEx);
//
//    LOG_WARN("WEIRD: [ReadHeaderEx] called however it should not be called");
//
//    return E_EREAD;
//}

int __stdcall ReadHeaderExW(int hArcData, tHeaderDataExW* HeaderDataExW)
{
    LogEngine::Logger& logger= LogEngine::GetLogger(LOGGER_NAME);
    logger.Info(FUNC + " called");

    if (IsArcDataCorrect(hArcData) && ArchiveList[hArcData].ArchiveOpen)
    {
        if (ArchiveList[hArcData].ArchivePos >= ArchiveList[hArcData].ArchiveFiles.size())
        {
            ArchiveList[hArcData].ArchivePos = 0;
            return E_END_ARCHIVE;
        }

        FileRecord& fr = ArchiveList[hArcData].ArchiveFiles[ArchiveList[hArcData].ArchivePos];
           
        logger.InfoFmt("{} FileName={}", FUNC, convert_string<char>(fr.fileName));
        
        wcscpy_s(HeaderDataExW->FileName, MAX_PATH, fr.fileName.c_str());
        wcscpy_s(HeaderDataExW->ArcName, MAX_PATH, ArchiveList[hArcData].ArchiveNameW);

        //HeaderDataExW->FileAttr = 0x3F;
        HeaderDataExW->FileCRC = (int)fr.CRC32Value;
        HeaderDataExW->PackSize = fr.compressedSize & BIG_FILESIZE_MASK;
        HeaderDataExW->PackSizeHigh = (fr.compressedSize >> 32) & BIG_FILESIZE_MASK;
        HeaderDataExW->UnpSize = fr.fileSize & BIG_FILESIZE_MASK;
        HeaderDataExW->UnpSizeHigh = (fr.fileSize >> 32) & BIG_FILESIZE_MASK;
        HeaderDataExW->FileTime = MakeTCTime(fr.GetModifiedDateAsTimeT());

        HeaderDataExW->HostOS = 0; // for compatibility with unrar.dll only, and should be set to zero.
        memset(HeaderDataExW->Reserved, 0, 1024);

        ArchiveList[hArcData].ArchivePos++;  // next file in multifile archive 
        
        return 0;
    }
    else 
    {
        return E_EREAD; //either hArcData is incorrect or archive referred by hArcData wasn't opened
    }
}

int __stdcall ProcessFileW(int hArcData, int Operation, WCHAR* DestPath, WCHAR* DestName)
{
    LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);

    if(IsOperationCorrect(Operation)) 
        logger.InfoFmt("{} Op={}", FUNC, FileOperations[Operation]);
    else 
        logger.InfoFmt("{} Op={}", FUNC, Operation);
    
    if (DestPath == nullptr)
        logger.InfoFmt("{} DestPath is NULL", FUNC);
    else
        logger.InfoFmt("{} DestPath: {}", FUNC, convert_string<char>(DestPath));

    std::string DestNameA; // used below several times

    if (DestName == nullptr)
        logger.InfoFmt("{} DestName is NULL", FUNC);
    else
    {
        DestNameA = convert_string<char>(DestName);
        logger.InfoFmt("{} DestName: {}", FUNC, DestNameA);
    }

    if (!IsArcDataCorrect(hArcData)) return E_EREAD;

    if (ArchiveList[hArcData].ArchiveOpen)
    {
        if (Operation == PK_SKIP) return 0;

        // Operation PK_TEST - when user wants to test archive.
        // usually this is File/Test Archive(s) main menu command in Total Commander
        if (Operation == PK_TEST)
        {
            logger.InfoFmt("{} Current Archive File Pos: {}", FUNC, ArchiveList[hArcData].ArchivePos - 1);
            assert(ArchiveList[hArcData].ArchivePos > 0);
            
            // because DestPath and DestName are NULL for PK_TEST operation
            FileRecord& fr = ArchiveList[hArcData].ArchiveFiles[ArchiveList[hArcData].ArchivePos - 1];
            
            try
            {
                string_t dirW = ExtractFileDirW(DestName);
                TCCallbackW cb(fr.fileName, ArchiveList[hArcData].ProcessDataProcW);
                Archiver arch;
                arch.AddCallback(&cb);
                arch.ExtractFile(ArchiveList[hArcData].ArchiveNameW, fr.fileName, dirW, true); //extract file to "/dev/null"
                arch.RemoveCallback(&cb);
            }
            catch (exception& e)
            {
                logger.Error(FUNC + e.what());
                return E_EREAD;
            }
            catch (...)
            {
                logger.Error(FUNC + " UNKNOWN ERROR");
                return E_EREAD;
            }
        } 
        else if (Operation == PK_EXTRACT && DestName != nullptr)
        {
            //char buf[MAX_PATH];
            //WCHARtoChar(buf, DestName);

            logger.InfoFmt("{} ArchiveName : {}", FUNC, ArchiveList[hArcData].ArchiveName);
            logger.InfoFmt("{} FileName : {}", FUNC, ExtractFileName(DestNameA));
            logger.InfoFmt("{} Dir to extract : {}", FUNC, ExtractFileDir(DestNameA));

            //if (ArchiveList[hArcData].ProcessDataProcW) 
            //{
            //    if (ArchiveList[hArcData].ProcessDataProcW(DestName ? DestName : ArchiveList[hArcData].ArchiveNameW, 0) == 0)
            //        return E_EABORTED;
            //}

            try
            {
                string_t filenameW = ExtractFileNameW(DestName);
                string_t dirW = ExtractFileDirW(DestName);

                TCCallbackW cb(filenameW, ArchiveList[hArcData].ProcessDataProcW);
                Archiver arch;
                arch.AddCallback(&cb);
                arch.ExtractFile(ArchiveList[hArcData].ArchiveNameW, filenameW, dirW);
                arch.RemoveCallback(&cb);
            }
            catch (exception& e)
            {
                logger.Error(FUNC + e.what());
                return E_EREAD;
            }
            catch (...)
            {
                logger.Error(FUNC + " UNKNOWN ERROR");
                return E_EREAD;
            }
        }
        else // incorrect operation or DestName=NULL
            return E_NOT_SUPPORTED;
    }
    else // incorrect archive handle hArcData
        return E_EREAD;
        
        return 0;
}

//this is mandatory function that must present in plugin 
int __stdcall ProcessFile(int hArcData, int Operation, char* DestPath, char* DestName)
{
    UNREFERENCED_PARAMETER(hArcData);
    UNREFERENCED_PARAMETER(Operation);
    UNREFERENCED_PARAMETER(DestPath);
    UNREFERENCED_PARAMETER(DestName);

    LOG_WARN("WEIRD: [ProcessFile] called however it should not be called");

    return E_EREAD;
}

int __stdcall PackFilesW(WCHAR* PackedFile, WCHAR* SubPath, WCHAR* SrcPath, WCHAR* AddList, int Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    LOG_INFO(FUNC + " called");
    LOG_INFO(FUNC + " PackedFile:" + convert_string<char>(PackedFile));
    LOG_INFO(FUNC + " SubPath:" + (SubPath==nullptr? "is NULL" : convert_string<char>(SubPath)));
    LOG_INFO(FUNC + " SrcPath:" + convert_string<char>(SrcPath));
    LOG_INFO(FUNC + " AddList[0]:" + convert_string<char>(AddList));

    WCHAR* p;
    int retval = 0;

    p = AddList;
    if (!p || !p[0]) return E_NO_FILES;

    try
    {
        vect_string_t files;

        while (p[0])
        {
            string_t FileName = string_t(SrcPath) + p;
            files.push_back(FileName);

            while (p[0]) p++; //moving to the next file
            p++;
        }

        TCCallbackW cb(p, ArchiveList[0].ProcessDataProcW);
        Archiver arch;
        arch.AddCallback(&cb);
        arch.CompressFiles(PackedFile, files, ARPackParams);
        arch.RemoveCallback(&cb);

    }
    catch (exception& e)
    {
        LOG_ERROR(FUNC + e.what());
        return E_EREAD;
    }
    catch (...)
    {
        LOG_ERROR(FUNC + " UNKNOWN ERROR");
        return E_EREAD;
    }

    return retval;
}

int __stdcall DeleteFiles(char* PackedFile, char* DeleteList)
{
    LOG_INFO(FUNC + " called");

    if (DeleteList == nullptr)
        return E_BAD_DATA;
    
    vect_string_t strList;
    StringListFromDeleteList(DeleteList, strList);

    try
    {
        Archiver arch;
        arch.RemoveFiles(convert_string<char_t>(PackedFile), strList);
    }
    catch (exception& e)
    {
        LOG_ERROR(FUNC + e.what());
        return E_EREAD;
    }
    catch (...)
    {
        LOG_ERROR(FUNC + " Unknown error caught!");
        return E_EREAD;
    }
    
    return 0;
}

int __stdcall DeleteFilesW(WCHAR* PackedFile, WCHAR* DeleteList)
{
    LOG_INFO(FUNC + " called");

    if (DeleteList == nullptr)
        return E_BAD_DATA;

    vect_string_t strList;
    StringListFromDeleteList(DeleteList, strList);

    try
    {
        Archiver arch;
        arch.RemoveFiles(PackedFile, strList);
    }
    catch (exception& e)
    {
        LOG_ERROR(FUNC + e.what());
        return E_EREAD;
    }
    catch (...)
    {
        LOG_ERROR(FUNC + " Unknown error caught!");
        return E_EREAD;
    }

    return 0;
}

int __stdcall GetPackerCaps()
{
    LOG_INFO(FUNC + " called");
    return PK_CAPS_MULTIPLE | PK_CAPS_DELETE | PK_CAPS_OPTIONS | PK_CAPS_NEW;
   // return PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_DELETE | PK_CAPS_OPTIONS | PK_CAPS_MEMPACK | PK_CAPS_BY_CONTENT | PK_CAPS_SEARCHTEXT;
}


//HANDLE hBrush;
BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    wchar_t value[PARAM_BUFSIZE]{};
    LRESULT res;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwndDlg, IDC_EDIT_BLOCKSIZE, WM_SETTEXT, 0, (LPARAM)std::to_wstring(ARPackParams.BLOCK_SIZE).c_str());
        
        for (size_t i = 0; i < sizeof(ARPackParams.ModelTypeCode) / sizeof(ARPackParams.ModelTypeCode[0]); i++)
        {
            res = SendDlgItemMessage(hwndDlg, IDC_COMBO_MODEL, CB_ADDSTRING, 0, (LPARAM)ARPackParams.ModelTypeCode[i].c_str());
            assert(res >= 0);
        }
        res = SendDlgItemMessage(hwndDlg, IDC_COMBO_MODEL, CB_SETCURSEL, (WPARAM)ARPackParams.MODEL_TYPE, 0);
        assert(res >= 0);

        for (size_t i = 0; i < sizeof(ARPackParams.CoderNames) / sizeof(ARPackParams.CoderNames[0]); i++)
        {
            res = SendDlgItemMessage(hwndDlg, IDC_COMBO_CODER, CB_ADDSTRING, 0, (LPARAM)ARPackParams.CoderNames[i].c_str());
            assert(res >= 0);
        }
        res = SendDlgItemMessage(hwndDlg, IDC_COMBO_CODER, CB_SETCURSEL, (WPARAM)ARPackParams.CODER_TYPE, 0);
        assert(res >= 0);

        res = SendDlgItemMessage(hwndDlg, IDC_EDIT_THREADS, WM_SETTEXT, 0, (LPARAM)std::to_wstring(ARPackParams.THREADS).c_str());
        assert(res >= 0);

        res = CheckDlgButton(hwndDlg, IDC_CHECK_STREAMMODE, !ARPackParams.BLOCK_SIZE ? BST_CHECKED : BST_UNCHECKED);
        assert(res > 0);

        res = SendDlgItemMessage(hwndDlg, IDC_EDIT_LOGFILENAME, WM_SETTEXT, 0, (LPARAM)ARPackParams.LOGFILENAME.c_str());
        assert(res >= 0);

        //hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        return TRUE;
        break;
    /*case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
        SetBkColor((HDC)wParam, GetSysColor(COLOR_BTNFACE));
        SetTextColor((HDC)wParam, GetSysColor(COLOR_BTNTEXT));
        return (BOOL)hBrush;
        break;
 */   case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            
            EnterCriticalSection(&slotcriticalsection);

            SendDlgItemMessage(hwndDlg, IDC_EDIT_BLOCKSIZE, WM_GETTEXT, PARAM_BUFSIZE, (LPARAM)value);
            ARPackParams.BLOCK_SIZE = _wtoi(value);

            SendDlgItemMessage(hwndDlg, IDC_COMBO_MODEL, WM_GETTEXT, PARAM_BUFSIZE, (LPARAM)value);
            ARPackParams.MODEL_TYPE = ARPackParams.ModelIdByName(value);
            
            SendDlgItemMessage(hwndDlg, IDC_COMBO_CODER, WM_GETTEXT, PARAM_BUFSIZE, (LPARAM)value);
            ARPackParams.CODER_TYPE = ARPackParams.CoderIdByName(value);

            SendDlgItemMessage(hwndDlg, IDC_EDIT_THREADS, WM_GETTEXT, PARAM_BUFSIZE, (LPARAM)value);
            ARPackParams.THREADS = _wtoi(value); 
            ARPackParams.BLOCK_MODE = IsDlgButtonChecked(hwndDlg, IDC_CHECK_STREAMMODE) == BST_UNCHECKED;
            ARPackParams.LOGFILENAME.resize(MAX_PATH);
            SendDlgItemMessage(hwndDlg, IDC_EDIT_LOGFILENAME, WM_GETTEXT, MAX_PATH, (LPARAM)ARPackParams.LOGFILENAME.data()); 
        
            LeaveCriticalSection(&slotcriticalsection);

            ARPackParams.SaveToIni(IniFileName);

            //DeleteObject(hBrush);
            EndDialog(hwndDlg, 1);
            break;
        case IDCANCEL:    /* Cancel */
            //DeleteObject(hBrush);
            EndDialog(hwndDlg, 0);
            break;
        case IDC_EDIT_BLOCKSIZE:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                SendDlgItemMessage(hwndDlg, IDC_EDIT_BLOCKSIZE, WM_GETTEXT, PARAM_BUFSIZE, (LPARAM)value);
                try
                {
                    uint32_t v = ParseBlockSize(value);
                    SendDlgItemMessage(hwndDlg, IDC_STATIC_BLOCKSIZE, WM_SETTEXT, 0, (LPARAM)_T("correct"));
                }
                catch(...)
                {
                    SendDlgItemMessage(hwndDlg, IDC_STATIC_BLOCKSIZE, WM_SETTEXT, 0, (LPARAM)_T(" NOT correct"));
                }
            }
            break;
        }
        break;
    }
    return FALSE;
}

void __stdcall ConfigurePacker(HWND ParentHandle, HINSTANCE hinstance)
{
    LOG_INFO(FUNC + " START");
    LOG_INFO(FUNC + " IniFile:" + convert_string<char>(IniFileName));
    
    // You may set here packing parameters here (NOT unpacking!) 
    DialogBox(hinstance, MAKEINTRESOURCE(IDD_DIALOG1), ParentHandle, (DLGPROC)&DialogProc);
    LOG_INFO(FUNC + " END");
}

int __stdcall GetBackgroundFlags(void)
{
    LOG_INFO(FUNC + " called");
    return BACKGROUND_UNPACK | BACKGROUND_PACK; // | BACKGROUND_MEMPACK;   // Support two out of three: Pack and Unpack
}

/*
BOOL __stdcall CanYouHandleThisFile(char* filename)
{
    FILE* ArchiveHandle;
    char sig[8];    // File signature
    int n;
    if (filename == NULL)
        return false;

    if (filename[0] == 0)
        return false;

    if ((ArchiveHandle = fopen(filename, "rb")) == NULL)
        return false;

    n = fread(sig, 1, 8, ArchiveHandle);
    if (n != 8) {
        fclose(ArchiveHandle);
        return false;
    }
    fclose(ArchiveHandle);
    if (!((sig[0] == 'B') && (sig[1] == 'Z') && (sig[2] == 'h'))) {
        return false;
    }
    return true;
}

BOOL __stdcall CanYouHandleThisFileW(WCHAR* filename)
{
    FILE* ArchiveHandle;
    char sig[8];    // File signature
    int n;
    if (filename == NULL)
        return false;

    if (filename[0] == 0)
        return false;

    if ((ArchiveHandle = _wfopen(filename, L"rb")) == NULL)
        return false;

    n = fread(sig, 1, 8, ArchiveHandle);
    if (n != 8) {
        fclose(ArchiveHandle);
        return false;
    }
    fclose(ArchiveHandle);
    if (!((sig[0] == 'B') && (sig[1] == 'Z') && (sig[2] == 'h'))) {
        return false;
    }
    return true;
}
*/
void __stdcall PackSetDefaultParams(PackDefaultParamStruct* dps)
{
    // initialize IniFileName, keep directory for ini file provided by TC but use our ini file name DEF_INIFILENAME
    string_t pluginIni = convert_string<char_t>(ExtractFileDir(dps->DefaultIniName));
    pluginIni.append(DEF_INIFILENAME);
    wcscpy_s(IniFileName, MAX_PATH - 1, pluginIni.c_str());

    ARPackParams.LoadFromIni(IniFileName); // loading parameters only after initializing IniFileName

    ShutDownLogger(); // free all previous loggers to avoid name conflict
    
    // initialize logger again with new filename
    LogEngine::Logger& logger = LogEngine::GetFileLogger(LOGGER_NAME, convert_string<char>(ARPackParams.LOGFILENAME));
    logger.SetLogLevel(LogEngine::Levels::llDebug);
    logger.GetSink(LOGGER_NAME)->SetLogLevel(LogEngine::Levels::llDebug);
    LogEngine::PatternLayout* lay = dynamic_cast<LogEngine::PatternLayout*>(logger.GetSink(LOGGER_NAME)->GetLayout());
    lay->SetAllPatterns("%LOGLEVEL% %DATETIME% #%THREAD% : %MSG%");

    logger.InfoFmt("{} called", FUNC);
    logger.InfoFmt("{} TC IniFile: {}", FUNC, dps->DefaultIniName);
    //logger.InfoFmt("{} TC Plugin Interface Version: {}.{}", FUNC, std::to_string(dps->PluginInterfaceVersionHi), std::to_string(dps->PluginInterfaceVersionLow));
    logger.InfoFmt("{} {} IniFileName: {}", FUNC, PLUGIN_NAME, convert_string<char>(pluginIni));
}

void __stdcall SetChangeVolProc(int hArcData, tChangeVolProc pChangeVolProc)
{
    LOG_INFO(FUNC + " called");
    int index = IsArcDataCorrect(hArcData) ? hArcData : 0;
    ArchiveList[index].ChangeVolProc = pChangeVolProc;
}

void __stdcall SetChangeVolProcW(int hArcData, tChangeVolProcW pChangeVolProcW)
{
    LOG_INFO(FUNC + " called");
    int index = IsArcDataCorrect(hArcData) ? hArcData : 0;
    ArchiveList[index].ChangeVolProcW = pChangeVolProcW;
}

void __stdcall SetProcessDataProc(int hArcData, tProcessDataProc pProcessDataProc)
{
    LOG_INFO(FUNC + " called");
    int index = IsArcDataCorrect(hArcData) ? hArcData : 0;    
    ArchiveList[index].ProcessDataProc = pProcessDataProc;
}

void __stdcall SetProcessDataProcW(int hArcData, tProcessDataProcW pProcessDataProcW)
{
   LOG_INFO(FUNC + " called");

    int index = IsArcDataCorrect(hArcData) ? hArcData : 0;
    ArchiveList[index].ProcessDataProcW = pProcessDataProcW;
}
