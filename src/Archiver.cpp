
#include <filesystem>
#if defined (__BORLANDC__)
#include <share.h>
#endif
#include "fpaq0.h"
#include "RangeCoder.h"
//#include "Exceptions.h"
#include "BasicModel.h"
#include "Context.h"
#include "Ticks.h"
#include "CommonFunctions.h"
#include "libsais64.h"
#include "MTF.h"
#include "Factories.h"
#include "Archiver.h"


using namespace std;

#define ARC_EXT ".ar"

#if defined (__BORLANDC__)
#define _SH_DENYRW SH_DENYRW
#define _SH_DENYWR SH_DENYWR
#define _SH_DENYRD SH_DENYRD
#define _SH_DENYNO SH_DENYNO
#endif


void Archiver::AddCallback(ICallback* cb)
{
    cbmanager.AddCallback(cb);
}

void Archiver::RemoveCallback(ICallback* cb)
{
    cbmanager.RemoveCallback(cb);
}

void Archiver::CompressFile(string FileName, string ArchiveFileName, Parameters params)
{
    vector_string_t files;

    files.push_back(FileName);
    CompressFiles(files, ArchiveFileName, params);
}

void Archiver::CompressFiles(const vector_string_t& files, string ArchiveFileName, Parameters params)
{
    ConsoleCallback ccb;
    cbmanager.AddCallback(&ccb);

    if (ArchiveFileName.substr(ArchiveFileName.size() - 3, 3) != ARC_EXT) ArchiveFileName += ARC_EXT;

#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
    logger.info("Creating archive '%s'.", ArchiveFileName.c_str());
    PrintCompressionStart(params);
    logger.info("Adding %d file%s to archive.", files.size() - 1, files.size() > 2 ? "s" : "");
    logger.info("------------------------------"); 
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger(); 
    logger.LogFmt(LogEngine::Levels::llInfo, "Creating archive '%s'.", ArchiveFileName.c_str());
    PrintCompressionStart(params);
    logger.LogFmt(LogEngine::Levels::llInfo, "Adding %d file%s to archive.", files.size() - 1, files.size() > 2 ? "s" : "");
    logger.Info("------------------------------");
#else
    LogEngine::Logger& logger = Global::GetLogger();
    logger.LogFmt(LogEngine::Levels::llInfo, "Creating archive '{}'.", ArchiveFileName.c_str());
    PrintCompressionStart(params);
    logger.LogFmt(LogEngine::Levels::llInfo, "Adding {} file{} to archive.", files.size() - 1, files.size() > 2 ? "s" : "");
    logger.Info("------------------------------");

#endif

    ofstream fout;
    fout.open(ArchiveFileName, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + ArchiveFileName + "' for writing. Check file permissions.");

    ArchiveHeader hd;
    vector_fr_t& frs = hd.fillFileRecs(files, params);
    hd.saveHeader(&fout);

    bool aborted = false;
    for (size_t i = 0; i < frs.size(); i++)
    {
        IModel* model = ModelCoderFactory::GetModelAndCoder(frs[i]);

        FileRecord& fr = frs[i];
        ifstream fin(fr.origFilename, ios::in | ios::binary);
        if (fin.fail()) throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
       
        int result;
        if (params.BLOCK_MODE)
            result = CompressFileBlock(&fout, &fin, fr, model);
        else 
            result = CompressFile(&fout, &fin, fr, model);
        
        if(result == CALLBACK_ABORT)
        {
            aborted = true;
            break;
        }

        fin.close();
    }

    fout.flush();
    fout.close();

    cbmanager.RemoveCallback(&ccb);

    if (aborted)
    {
        std::filesystem::remove(ArchiveFileName); 
#ifdef LOG4CPP
        logger.info("User has cancelled compression process. Aborting.");
#else
        logger.Info("User has cancelled compression process. Aborting.");
#endif
    }
    else
    {
        hd.updateHeaders(ArchiveFileName);
#ifdef LOG4CPP
        logger.info("All files are compressed.");
#else
        logger.Info("All files are compressed.");
#endif
    }
}

#define SHOWP (fr.fileSize > SHOW_PROGRESS_AFTER)

int Archiver::CompressFile(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
    logger.info("Compressing file '%s'.", fr.origFilename.c_str());
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger(); 
    logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '%s'.", fr.origFilename.c_str());
#else
    LogEngine::Logger& logger = Global::GetLogger();
    logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '{}'.", fr.origFilename.c_str());
#endif
    
    Ticks::Start(fr.origFilename);

   // IModel* model = ModelsFactory::GetModel();
   // CallBack cb;

    uint64_t delta = fr.fileSize / 100;
    uint64_t threshold = 0;
    uint64_t cntRead = 0;

    /*ifstream fin(fr.origFilename, ios::in | ios::binary);
    if (fin.fail())
    {
        throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
    }*/

    if (SHOWP) cbmanager.Start();

    model->BeginEncode(fout);

    Context ctx;
    bool showprogress = SHOWP;
    while(true)
    {
        uchar sm = (uchar)fin->get();
        if (fin->eof()) break;
        

        cntRead++;

        if (showprogress && (cntRead > threshold))
        {
            threshold += delta;
            if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold/fr.fileSize))) // user has aborted compression process
                return CALLBACK_ABORT;
        }

        model->EncodeSymbol(ctx.getCtx(), sm);
        ctx.add(sm);
    }

    model->StopEncode();
    if (SHOWP) cbmanager.Finish();

    fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO makes sense do it better and more convenient

    //fin.close();
    PrintFileCompressionDone(fr);
    auto tick = Ticks::Finish(fr.origFilename);
#ifdef LOG4CPP
    logger.info("%-16s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.info("------------------------------");
#elif defined(__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#else 
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {}", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#endif

    return CALLBACK_OK;
}

int Archiver::CompressFileBlock(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
    logger.info("Compressing file '%s'.", fr.origFilename.c_str());
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger();
    logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '%s'.", fr.origFilename.c_str());
#else
    LogEngine::Logger& logger = Global::GetLogger();
    logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '{}'.", fr.origFilename.c_str());

#endif

    Ticks::Start(fr.origFilename);

   // IModel* model = ModelsFactory::GetModel();
  //  CallBack cb;

    uint32_t numBlocks = 1 + (uint32_t)(fr.fileSize / fr.blockSize);
    uint32_t delta =  numBlocks / 100 + 1;
    uint32_t threshold = 0;
    uint32_t cntBlocks = 0;
    
    //ifstream fin(fr.origFilename, ios::in | ios::binary);
    //if (fin.fail())
    //{
    //    throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
    //}

    if (SHOWP) cbmanager.Start();

    ostringstream fblock;
    model->BeginEncode(&fblock);

    Context ctx;
    bool showprogress = SHOWP;
    uint32_t i = 0;
    int64_t lineNum = -1;
    uint8_t* buf    = new uint8_t[fr.blockSize];
    uint8_t* bwtbuf = new uint8_t[fr.blockSize];
    int64_t* temp   = new int64_t[fr.blockSize + 1];
    //int64_t freq[256];
    MTF mtf;

    cbmanager.Progress(0);

    while (true)
    {    
        fin->read((char*)buf, fr.blockSize);
        streamsize cntRead = fin->gcount();

        if (cntRead == 0) break; // for the case when (filesize % fr.blockSize)==0

        //string fn = "bufencode";
        //SaveToFile(fn + to_string(cntBlocks), (char*)buf, cntRead);

        lineNum = libsais64_bwt(buf, bwtbuf, temp, cntRead, 0, nullptr);
#ifdef LOG4CPP
        if (lineNum < 0) logger.error("[CompressFileBlock] BWT function returned error code.");
#else
        if (lineNum < 0) logger.Error("[CompressFileBlock] BWT function returned error code.");
#endif

        mtf.Encode(bwtbuf, buf, cntRead);

        //fn = "bwtbufencode";
        //SaveToFile(fn + to_string(cntBlocks), (char*)bwtbuf, cntRead);

        model->GetCoder().StartBlockEncode();
        
        for (i = 0; i < cntRead; i++)
        {
            model->EncodeSymbol(ctx.getCtx(), (uchar)buf[i]);
            ctx.add((uchar)buf[i]);
        }

        model->GetCoder().FinishBlockEncode(); // save 'last 4 bytes' into block.

        SaveBlock(fblock, fout, i, (uint32_t)lineNum);

        cntBlocks++;
        if (showprogress && (cntBlocks > threshold))
        {
            threshold += delta;
            if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold / numBlocks)))
                return CALLBACK_ABORT; // TODO immediate return causes memory leak
        }

        if (fin->eof()) break;
    }

    model->StopEncode();
    if (SHOWP) cbmanager.Finish();

    delete [] buf;
    delete [] bwtbuf;
    delete [] temp;
  
    fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO makes sense do it better and more convenient
    fr.blockCount = cntBlocks;

    //fin.close();
    PrintFileCompressionDone(fr);
    auto tick = Ticks::Finish(fr.origFilename);
#ifdef LOG4CPP
    logger.info("%-16s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.info("------------------------------");
#elif defined(__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#else
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {}", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#endif
    return CALLBACK_OK;
}

void Archiver::SaveBlock(ostringstream& fb, ofstream* fout, uint32_t uBlockSize, int32_t lineNum)
{
    string s = fb.str();
  
    uint32_t cBlockSize = (uint32_t)s.size();
    char flags = 0;

    fout->write((char*)&cBlockSize, sizeof(uint32_t)); // compressed size
    fout->write((char*)&uBlockSize, sizeof(uint32_t)); // uncompressed size
    fout->write((char*)&lineNum,    sizeof(int32_t));
    fout->write((char*)&flags,      sizeof(char));

    fout->write(s.data(), cBlockSize);
    fb.str(""); //clear output block buffer

    //string fn = "compressedblocksaved" + to_string(lineNum);
    //SaveToFile(fn, s.data(), len);
}

void Archiver::UncompressFiles(ifstream* fin, Parameters params)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif

    ConsoleCallback ccb;
    cbmanager.AddCallback(&ccb);

    ArchiveHeader ah;
    auto files = ah.loadHeader(fin);

    bool aborted = false;
    for (size_t i = 0; i < files.size(); i++)
    {
        IModel* model = ModelCoderFactory::GetModelAndCoder(files[i]);

        string realFileName = params.OUTPUT_DIR + "\\" + files[i].fileName;
        ofstream fout(realFileName, ios::out | ios::binary, _SH_DENYWR);
        if (fout.fail())
            throw file_error("Cannot open file '" + realFileName + "' for writing.");

        PrintUncompressionStart(files[i], params);

        int result;
        if (files[i].blockCount > 0)
            result = UncompressFileBlock(fin, &fout, files[i], model);
        else
            result = UncompressFile(fin, &fout, files[i], model);

        fout.close();

        if (result == CALLBACK_ABORT)
        {
            std::filesystem::remove(realFileName); // remove partially uncompressed file
            aborted = true;
            break;
        }
    }

    cbmanager.RemoveCallback(&ccb);

    if (aborted)
    {
#ifdef LOG4CPP
        logger.info("User has cancelled uncompression process. Aborting.");
#else
        logger.Info("User has cancelled uncompression process. Aborting.");
#endif
    }
}

int Archiver::UncompressFile(ifstream* fin, ofstream* fout, FileRecord& fr, IModel* model)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif

    Ticks::Start(fr.fileName);

    //CallBack cb;
    
    uint64_t delta = fr.fileSize / 100;
    uint64_t threshold = 0;

  /*  string realFileName = ParametersG::OUTPUT_DIR + "\\" + fr.fileName;
    ofstream fout(realFileName, ios::out | ios::binary);
    if(fout.fail())
        throw file_error("Cannot open file '" + realFileName + "' for writing.");*/

    if (SHOWP) cbmanager.Start();
    model->BeginDecode(fin);

    uchar symbol;
    Context ctx;
    uint64_t decodedBytes = 0;

    try
    {
        while (decodedBytes < fr.fileSize)
        {
            if (SHOWP && decodedBytes > threshold)
            {
                threshold += delta;
                if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold / fr.fileSize)))
                    return CALLBACK_ABORT;
            }

            symbol = model->DecodeSymbol(ctx.getCtx()); // TODO what to do when we extract first few symbols and we do not have context yet??
            ctx.add(symbol);
            decodedBytes++;
            fout->put(symbol);
        }
    }
    catch (...)
    {
        throw;
    }

    model->StopDecode();
    if (SHOWP) cbmanager.Finish();
  
   // fout.close();

#ifdef LOG4CPP
    logger.info("Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.info("%-19s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.info("------------------------------");
#elif defined(__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#else
    logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '{}'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#endif
    return CALLBACK_OK;
}

void Archiver::LoadBlock(istringstream& fb, ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum)
{
    char flags = 0;

    fin->read((char*)&cBlockSize, sizeof(uint32_t)); // compressed size
    fin->read((char*)&uBlockSize, sizeof(uint32_t)); // uncompressed size
    fin->read((char*)&lineNum, sizeof(int32_t));
    fin->read((char*)&flags, sizeof(char));

    static string s;// (buf, cBlockSize);// TODO make s static? to avoid buffer realocating each time
    s.reserve(uBlockSize);
    s.resize(cBlockSize);

    //char* buf = new char[cBlockSize];
    fin->read(s.data(), cBlockSize);

    fb.str(s);
    fb.clear();

    //string fn = "compressedblockloaded" + to_string(lineNum);
    //SaveToFile(fn, s.data(), cBlockSize);
}

int Archiver::UncompressFileBlock(ifstream* fin, ofstream *fout, FileRecord& fr, IModel* model)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif

    Ticks::Start(fr.fileName);

    //CallBack cb;

    uint32_t numBlocks = 1 + (uint32_t)(fr.fileSize / fr.blockSize);
    uint32_t delta = numBlocks / 100 + 1;
    uint64_t threshold = 0;

    //string realFileName = ParametersG::OUTPUT_DIR + "\\" + fr.fileName;
    //ofstream fout(realFileName, ios::out | ios::binary);
    //if (fout.fail())
    //    throw file_error("Cannot open file '" + realFileName + "' for writing.");

    bool showprogress = SHOWP;
    if (SHOWP) cbmanager.Start();

    uint8_t* buf    = new uint8_t[fr.blockSize];
    uint8_t* bwtbuf = new uint8_t[fr.blockSize];
    int64_t* temp   = new int64_t[fr.blockSize + 1];
    //int64_t freq[256];

    istringstream fblock;
    int32_t lineNum = -1;
    uint32_t uBlockSize = 0;
    uint32_t cBlockSize = 0;
    uint32_t cntBlocks = 0;

    LoadBlock(fblock, fin, uBlockSize, cBlockSize, lineNum);
    model->BeginDecode(&fblock);
    
    uchar symbol;
    Context ctx;
    MTF mtf;
    uint64_t decodedBytes = 0;
    
    uint32_t i = 0;
    
    cbmanager.Progress(0);

    while (decodedBytes < fr.fileSize)
    {     
        if (decodedBytes > 0) // first block is loaded already, bypass loading first block here 
        {
            LoadBlock(fblock, fin, uBlockSize, cBlockSize, lineNum);
            model->GetCoder().StartBlockDecode();
        }
        
        try
        {
            for (i = 0; i < uBlockSize; i++)
            {
                symbol = model->DecodeSymbol(ctx.getCtx()); // TODO what to do when we extract first few symbols and we do not have context yet??
                ctx.add(symbol);
                decodedBytes++;
                //fout.put(symbol);
                bwtbuf[i] = symbol;
            }
        }
        catch (runtime_error&)
        {
            throw;
        }

        model->GetCoder().FinishBlockDecode();

        //uint32or64 compressedBytes = rc.GetBytesPassed();
        cntBlocks++;
        if (showprogress && cntBlocks > threshold)
        {
            threshold += delta;
            if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold / numBlocks)))
                return CALLBACK_ABORT; //TODO causes memory leak
        }

        //string fn("bwtbufdecode");
        //SaveToFile(fn + to_string(decodedBytes), (char*)bwtbuf, uBlockSize);

        mtf.Decode(bwtbuf, buf, uBlockSize);

        int64_t result = libsais64_unbwt(buf, bwtbuf, temp, uBlockSize, nullptr, lineNum);
#ifdef LOG4CPP
        if (result != 0) logger.error("[UncompressFileBlock] unBWT function returned error code.");
#else
        if (result != 0) logger.Error("[UncompressFileBlock] unBWT function returned error code.");
#endif

        fout->write((char*)bwtbuf, uBlockSize);
    }

    model->StopDecode();
    if (SHOWP) cbmanager.Finish();
  
    delete[] buf;
    delete[] bwtbuf;
    delete[] temp;
#ifdef LOG4CPP
    logger.info("Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.info("%-19s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.info("------------------------------");
#elif defined (__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#else
    logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '{}'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Spent time:", millisecToStr(tick).c_str());
    logger.Info("------------------------------");
#endif
    return CALLBACK_OK;
}


void Archiver::ExtractFiles(string ArchiveFile, vector_string_t FilesToExtract, string ExtractDir, Parameters params)
{
    for (auto fl: FilesToExtract)
    {
        ExtractFile(ArchiveFile, fl, ExtractDir, params);
    }
}

void Archiver::ExtractFile(string ArchiveFile, string FileToExtract, string ExtractDir, Parameters params)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif

    ifstream fin(ArchiveFile, ios::in | ios::binary);
    if (!fin)
        throw file_error("Cannot open archive file '" + ArchiveFile + "' for reading. Exiting.");
       
    ArchiveHeader ah;
    auto files = ah.loadHeader(&fin);

    FileRecord fr;
    bool found = false;
    for (size_t i = 0; i < files.size(); i++)
    {
        if (files[i].fileName == FileToExtract)
        {
            found = true;
            fr = files[i];
            break;
        }
    }

    if (!found)
        throw file_error("File '" + FileToExtract + "' absent in archive. Cannot extract.");
    
    for (size_t i = 0; i < files.size(); i++)
    {
        if (files[i].fileName == FileToExtract)
        {
            break;
        }
        else
        {
            BypassFile(&fin, files[i]);
        }
    }

    string realFileName = ExtractDir + "\\" + FileToExtract;
    
    ofstream fout(realFileName, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + realFileName + "' for writing.");

    PrintUncompressionStart(fr, params);

    IModel* model = ModelCoderFactory::GetModelAndCoder(fr);

    ConsoleCallback ccb;
    cbmanager.AddCallback(&ccb);

    int result;
    if (fr.blockCount > 0)
        result = UncompressFileBlock(&fin, &fout, fr, model);
    else
        result = UncompressFile(&fin, &fout, fr, model);
    
    fout.close();
    cbmanager.RemoveCallback(&ccb);


    if (result == CALLBACK_ABORT)
    {
        filesystem::remove(realFileName); // deleting partially extracted file
#ifdef LOG4CPP
        logger.info("User has cancelled file extracting process. Aborting.");
#else
        logger.Info("User has cancelled file extracting process. Aborting.");
#endif
    }
}

bool Archiver::BypassFile(ifstream* fin, FileRecord& fr)
{
    if (fr.blockCount == 0)
    {
        fin->ignore(fr.compressedSize);
    }
    else
    {
        for (uint32_t j = 0; j < fr.blockCount; j++)
        {
            uint32_t cBlockSize;
            uint32_t uBlockSize;
            uint32_t bwtLineNum;
            fin->read((char*)&cBlockSize, sizeof(uint32_t));
            fin->read((char*)&uBlockSize, sizeof(uint32_t));
            fin->read((char*)&bwtLineNum, sizeof(uint32_t));

            char bflags = (char)fin->get();

            assert(bflags == 0);
            assert(uBlockSize < (1<<29));
            assert(cBlockSize < (1 << 29));

            fin->ignore(cBlockSize);

            if (fin->fail()) 
                return false;
        }
    }

    return !fin->fail();
}

bool Archiver::CopyFileData(ifstream* fin, ofstream* fout, FileRecord& fr)
{
    if (fr.blockCount == 0)
    {
        unique_ptr<char[]> buffer {std::make_unique<char[]>(fr.compressedSize)};

        //char* buf = new char[fr.compressedSize]; // TODO may be use smaller buffer to copy file data
        fin->read(buffer.get(), fr.compressedSize);
        fout->write(buffer.get(), fr.compressedSize);
        //delete[] buf;
    }
    else
    {
        const uint32_t INIT_BUFSIZE = 1 << 16; // 64K
        unique_ptr<char[]> buffer{ std::make_unique<char[]>(INIT_BUFSIZE) };
        //char* buf = new char[INIT_BUFSIZE];
        uint32_t bufSize = INIT_BUFSIZE
            ;
        for (uint32_t j = 0; j < fr.blockCount; j++)
        {
            uint32_t cBlockSize;
            uint32_t uBlockSize;
            uint32_t bwtLineNum;
            fin->read((char*)&cBlockSize, sizeof(uint32_t));
            fin->read((char*)&uBlockSize, sizeof(uint32_t));
            fin->read((char*)&bwtLineNum, sizeof(uint32_t));

            char bflags = (char)fin->get();

            assert(bflags == 0);
            assert(uBlockSize < (1 << 29));
            assert(cBlockSize < (1 << 29));

            if (cBlockSize > bufSize)
            {
                //delete[] buf;
                buffer.reset(new char[cBlockSize]);
                bufSize = cBlockSize;
            }

            fin->read(buffer.get(), cBlockSize);

            fout->write((char*)&cBlockSize, sizeof(uint32_t)); // compressed size
            fout->write((char*)&uBlockSize, sizeof(uint32_t)); // uncompressed size
            fout->write((char*)&bwtLineNum, sizeof(int32_t));
            fout->write((char*)&bflags, sizeof(char));

            fout->write(buffer.get(), cBlockSize);
            if (fout->fail()) 
                return false;
        }

        //delete[] buf;
        return true;
    }

    return !fin->fail() && !fout->fail();
}

void Archiver::RemoveFile(string ArchiveFile, string FileToDelete)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif
    
    ifstream fin(ArchiveFile, ios::in | ios::binary);
    if (!fin)
        throw file_error("Cannot open archive file '" + ArchiveFile + "' for reading. Exiting.");

    ArchiveHeader ah;
    vector_fr_t& files = ah.loadHeader(&fin);

    vector_fr_t filesOLD = files;

    if (!ah.FileInList(FileToDelete)) //incorrect file name
    {
        throw invalid_argument("File '" + FileToDelete + "' cannot be found in archive '" + ArchiveFile + "'.");
    }
#ifdef LOG4CPP
    logger.info("Deleting file '%s' from archive '%s'", FileToDelete.c_str(), ArchiveFile.c_str());
#elif defined(__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "Deleting file '%s' from archive '%s'", FileToDelete.c_str(), ArchiveFile.c_str());
#else
    logger.LogFmt(LogEngine::Levels::llInfo, "Deleting file '{}' from archive '{}'", FileToDelete.c_str(), ArchiveFile.c_str());
#endif

    ah.RemoveFileFromList(FileToDelete);

    // TODO add handling of removing last files from archive.
    
    string newArchiveFile = ArchiveFile + ".tmp";
    ofstream fout(newArchiveFile, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + newArchiveFile + "' for writing.");

    ah.saveHeader(&fout); // saving header without deleted file in the list


    for (size_t i = 0; i < filesOLD.size(); i++)
    {
        if (filesOLD[i].fileName == FileToDelete)
        {
            BypassFile(&fin, filesOLD[i]);
        }
        else
        {
            CopyFileData(&fin, &fout, filesOLD[i]);
        }
    }

    fin.close();
    fout.close();

    if (fin.fail())
    {
        std::remove(newArchiveFile.c_str());
        throw file_error("Error reading archive file '" + ArchiveFile + "'.");
    }

    if (fout.fail())
    {
        std::remove(newArchiveFile.c_str());
        throw file_error("Error writing archive file '" + newArchiveFile + "'.");
    }

    std::remove(ArchiveFile.c_str());
    std::rename(newArchiveFile.c_str(), ArchiveFile.c_str());
#ifdef LOG4CPP
    logger.info("Deleted successfully");
#else
    logger.Info("Deleted successfully");
#endif
}

void Archiver::RemoveFiles(string ArchiveFile, vector_string_t& flist)
{
    if (flist.size() == 0) return;

#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
#else
    LogEngine::Logger& logger = Global::GetLogger();
#endif

    ifstream fin(ArchiveFile, ios::in | ios::binary);
    if (!fin)
        throw file_error("Cannot open archive file '" + ArchiveFile + "' for reading. Exiting.");

    ArchiveHeader ah;
    vector_fr_t& files = ah.loadHeader(&fin);

    vector_fr_t filesOLD = files;

    //if (!ah.FileInList(FileToDelete)) //incorrect file name
    //{
    //    throw invalid_argument("File '" + FileToDelete + "' cannot be found in archive '" + ArchiveFile + "'.");
    //}
#ifdef LOG4CPP
    logger.info("Deleting files from archive '%s'", ArchiveFile.c_str());
#elif defined(__BORLANDC__)
    logger.LogFmt(LogEngine::Levels::llInfo, "Deleting files from archive '%s'", ArchiveFile.c_str());
#else
    logger.LogFmt(LogEngine::Levels::llInfo, "Deleting files from archive '{}'", ArchiveFile.c_str());
#endif
    
    ah.RemoveFilesFromList(flist);

    // TODO add handling of removing last file from archive.

    string newArchiveFile = ArchiveFile + ".tmp";
    ofstream fout(newArchiveFile, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + newArchiveFile + "' for writing.");

    ah.saveHeader(&fout); // saving header without deleted file in the list


    //for (size_t i = 0; i < filesOLD.size(); i++)
    for (vector_fr_t::iterator iter = filesOLD.begin(); iter != filesOLD.end(); iter++)
    {
        auto result = std::find(flist.begin(), flist.end(), iter->fileName);

        if (result == flist.end()) // file not found, copy its data into new archive
            CopyFileData(&fin, &fout, *iter);
        else
            BypassFile(&fin, *iter);
    }

    if (fin.fail())
    {
        fin.close();
        std::remove(newArchiveFile.c_str());
        throw file_error("Error reading archive file '" + ArchiveFile + "'.");
    }

    if (fout.fail())
    {
        fout.close();
        std::remove(newArchiveFile.c_str());
        throw file_error("Error writing archive file '" + newArchiveFile + "'.");
    }

    fin.close();
    fout.close();
    
    std::remove(ArchiveFile.c_str());
    std::rename(newArchiveFile.c_str(), ArchiveFile.c_str());

#ifdef LOG4CPP
    logger.info("Deleted successfully");
#else
    logger.Info("Deleted successfully");
#endif
    
}

void Archiver::PrintCompressionStart(Parameters params)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();
    //logger.info("Using Arithmetic Range compressor.");
    logger.info("%-19s %s.", "Compression coder:", Parameters::CoderNames[(uint8_t)params.CODER_TYPE].c_str());
    logger.info("%-19s %s", "Use model type:", Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE].c_str());
    logger.info("%-19s %s", "Block mode:", params.BLOCK_MODE ? "YES" : "NO");
    if (params.BLOCK_MODE)
        logger.info("%-19s %s bytes", "Block size:", toStringSep(params.BLOCK_SIZE).c_str());
    logger.info("%-19s %u", "Threads count:", params.THREADS);
    logger.info("%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger();
    //logger.Info("Using Arithmetic Range compressor.");
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s.", "Compression coder:", Parameters::CoderNames[(uint8_t)params.CODER_TYPE].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Use model type:", Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Block mode:", params.BLOCK_MODE ? "YES" : "NO");
    if (params.BLOCK_MODE)
        logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s bytes", "Block size:", toStringSep(params.BLOCK_SIZE).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %u", "Threads count:", params.THREADS);
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
#else
    LogEngine::Logger& logger = Global::GetLogger();
    //logger.info("Using Arithmetic Range compressor.");
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}.", "Compression coder:", Parameters::CoderNames[(uint8_t)params.CODER_TYPE].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Use model type:", Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Block mode:", params.BLOCK_MODE ? "YES" : "NO");
    if (params.BLOCK_MODE)
        logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {} bytes", "Block size:", toStringSep(params.BLOCK_SIZE).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Threads count:", params.THREADS);
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Verbose:", params.VERBOSE ? "true" : "false");
#endif

}

void Archiver::PrintUncompressionStart(FileRecord& fr, Parameters params)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();

    //logger.info("Using Arithmetic Range compressor.");
    logger.info("%-19s %s", "Extracting file:", fr.fileName.c_str());
    logger.info("%-19s %s", "Extracting to:", params.OUTPUT_DIR.c_str());
    logger.info("%-19s %s", "Compression coder:", Parameters::CoderNames[fr.alg].c_str());
    logger.info("%-19s %s", "Model type:", Parameters::ModelTypeCode[fr.modelOrder].c_str());
    logger.info("%-19s %s", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
    logger.info("%-19s %s bytes", "Block size:", toStringSep(fr.blockSize).c_str());
    logger.info("%-19s %u", "Threads count:", params.THREADS);
    logger.info("%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
    //logger.info("------------------------------");
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger();
    //logger.info("Using Arithmetic Range compressor.");

    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Extracting file:", fr.fileName.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Extracting to:", params.OUTPUT_DIR.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Compression coder:", Parameters::CoderNames[fr.alg].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Model type:", Parameters::ModelTypeCode[fr.modelOrder].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s bytes", "Block size:", toStringSep(fr.blockSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %u", "Threads count:", params.THREADS);
    logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
    //logger.info("------------------------------");
#else
    LogEngine::Logger& logger = Global::GetLogger();
    //logger.info("Using Arithmetic Range compressor.");

    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Extracting file:", fr.fileName.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Extracting to:", params.OUTPUT_DIR.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Compression coder:", Parameters::CoderNames[fr.alg].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Model type:", Parameters::ModelTypeCode[fr.modelOrder].c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {} bytes", "Block size:", toStringSep(fr.blockSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Threads count:", params.THREADS);
    logger.LogFmt(LogEngine::Levels::llInfo, "{:19} {}", "Verbose:", params.VERBOSE ? "true" : "false");
    //logger.info("------------------------------");
#endif // LOG4CPP
}

void Archiver::PrintFileCompressionDone(FileRecord& fr)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();

    logger.info("Done compression.");
    logger.info("%-16s '%s'", "File name:", fr.origFilename.c_str());
    logger.info("%-16s %s bytes", "File size:", toStringSep(fr.fileSize).c_str());
    logger.info("%-16s %s bytes", "Compressed size:", toStringSep(fr.compressedSize).c_str());
    logger.info("%-16s %.2f", "Ratio:", (float)fr.fileSize / (float)fr.compressedSize);
    logger.info("%-16s %s", "Blocks:", toStringSep(fr.blockCount).c_str());
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger();
    logger.Info("Done compression.");
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s '%s'", "File name:", fr.origFilename.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s bytes", "File size:", toStringSep(fr.fileSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s bytes", "Compressed size:", toStringSep(fr.compressedSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %.2f", "Ratio:", (float)fr.fileSize / (float)fr.compressedSize);
    logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Blocks:", toStringSep(fr.blockCount).c_str());
#else
    LogEngine::Logger& logger = Global::GetLogger();
    logger.Info("Done compression.");
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} '{}'", "File name:", fr.origFilename.c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {} bytes", "File size:", toStringSep(fr.fileSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {} bytes", "Compressed size:", toStringSep(fr.compressedSize).c_str());
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {}", "Ratio:", (float)fr.fileSize / (float)fr.compressedSize);
    logger.LogFmt(LogEngine::Levels::llInfo, "{:16} {}", "Blocks:", toStringSep(fr.blockCount).c_str());
#endif // LOG4CPP

}
