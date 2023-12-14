
#include <filesystem>
#include "fpaq0.h"
#include "RangeCoder.h"
#include "Exceptions.h"
#include "ModelOrder1.h"
#include "ModelOrder2.h"
#include "ModelOrder3.h"
#include "Context.h"
#include "Ticks.h"
#include "Functions.h"
#include "RangeCompressor.h"
#include "libsais64.h"
#include "MTF.h"
#include "ModelFactory.h"


using namespace std;

#define ARC_EXT ".ar"

void RangeCompressor::CompressFiles(const vector_string_t& files)
{
    string arcFile = files[0]; // first item is archive file name
    if (arcFile.substr(arcFile.size() - 3, 3) != ARC_EXT) arcFile += ARC_EXT;

    log4cpp::Category& logger = Parameters::logger;

    logger.info("Creating archive '%s'.", arcFile.c_str());
    PrintCompressionStart();

    logger.info("Adding %d file%s to archive.", files.size() - 1, files.size()>2?"s":"");

    ofstream fout;
    fout.open(arcFile, ios::out | ios::binary);
    if (!fout)
        throw file_error("Cannot open file '" + arcFile + "' for writing. Check file permissions.");
    
    ArchiveHeader hd;
    vector_fr_t& frs = hd.fillFileRecs(files);
    hd.saveHeader(&fout);

    for (size_t i = 0; i < frs.size(); i++)
    {
        if(Parameters::BLOCK_MODE)
            CompressFileBlock(&fout, frs[i]);
        else
            CompressFile(&fout, frs[i]);
    }

    fout.flush();
    fout.close();

    hd.updateHeaders(arcFile);

    logger.info("All files are compressed.");
}

#define SHOWP (fr.fileSize > SHOW_PROGRESS_AFTER)

void RangeCompressor::CompressFile(ofstream* fout, FileRecord& fr)
{
    log4cpp::Category& logger = Parameters::logger;
    logger.info("Compressing file '%s'.", fr.origFilename.c_str());
    Ticks::Start(fr.origFilename);

    IModel* model = ModelsFactory::GetModel();
    CallBack cb;

    uint64_t delta = fr.fileSize / 100;
    uint64_t threshold = 0;
    uint64_t cntRead = 0;

    ifstream fin(fr.origFilename, ios::in | ios::binary);
    if (fin.fail())
    {
        throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
    }

    if (SHOWP) cb.start();

    model->BeginEncode(fout);

    Context ctx;
    uchar sm;
    bool showprogress = SHOWP;
    while(true)
    {
        sm = (uchar)fin.get();
        if (fin.eof()) break;
        ctx.add(sm);

        cntRead++;

        if (showprogress && (cntRead > threshold))
        {
            threshold += delta;
            cb.progress((100ull * threshold/fr.fileSize));
        }

        model->EncodeSymbol(ctx.getCtx());
    }

    model->StopEncode();
    if (SHOWP) cb.finish();

    fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO некрасиво надо бы переделать

    fin.close();
    PrintFileCompressionDone(fr);
    auto tick = Ticks::Finish(fr.origFilename);
    logger.info("Spent time: %s", millisecToStr(tick).c_str());
}

void RangeCompressor::CompressFileBlock(ofstream* fout, FileRecord& fr)
{
    log4cpp::Category& logger = Parameters::logger;
    logger.info("Compressing file '%s'.", fr.origFilename.c_str());
    Ticks::Start(fr.origFilename);

    IModel* model = ModelsFactory::GetModel();
    CallBack cb;

    uint32_t numBlocks = 1 + (uint32_t)(fr.fileSize / fr.blockSize);
    uint32_t delta =  numBlocks / 100 + 1;
    uint32_t threshold = 0;
    uint32_t cntBlocks = 0;
    
    ifstream fin(fr.origFilename, ios::in | ios::binary);
    if (fin.fail())
    {
        throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
    }

    if (SHOWP) cb.start();

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

    cb.progress(0);

    while (true)
    {    
        fin.read((char*)buf, fr.blockSize);
        streamsize cntRead = fin.gcount();

        if (cntRead == 0) break; // for the case when (filesize % fr.blockSize)==0

        //string fn = "bufencode";
        //SaveToFile(fn + to_string(cntBlocks), (char*)buf, cntRead);

        lineNum = libsais64_bwt(buf, bwtbuf, temp, cntRead, 0, nullptr);
        if (lineNum < 0) logger.error("[CompressFileBlock] BWT function returned error code.");

        mtf.Encode(bwtbuf, buf, cntRead);

        //fn = "bwtbufencode";
        //SaveToFile(fn + to_string(cntBlocks), (char*)bwtbuf, cntRead);

        model->GetCoder().StartBlockEncode();
        
        for (i = 0; i < cntRead; i++)
        {
            ctx.add((uchar)buf[i]);
            model->EncodeSymbol(ctx.getCtx());
        }

        model->GetCoder().FinishBlockEncode(); // save 'last 4 bytes' into block.

        SaveBlock(fblock, fout, i, (uint32_t)lineNum);

        cntBlocks++;
        if (showprogress && (cntBlocks > threshold))
        {
            threshold += delta;
            cb.progress((100ull * threshold / numBlocks));
        }

        if (fin.eof()) break;

    }

    model->StopEncode();
    if (SHOWP) cb.finish();

    delete [] buf;
    delete [] bwtbuf;
    delete [] temp;
  
    fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO некрасиво надо бы переделать
    fr.blockCount = cntBlocks;

    fin.close();
    PrintFileCompressionDone(fr);
    auto tick = Ticks::Finish(fr.origFilename);
    logger.info("Spent time: %s", millisecToStr(tick).c_str());
}

void RangeCompressor::SaveBlock(std::ostringstream& fb, std::ofstream* fout, uint32_t uBlockSize, int32_t lineNum)
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

void RangeCompressor::UncompressFiles(ifstream* fin)
{
    ArchiveHeader ah;
    auto files = ah.loadHeader(fin);

    for (size_t i = 0; i < files.size(); i++)
    {
        if (files[i].blockCount > 0)
            UncompressFileBlock(fin, files[i]);
        else
            UncompressFile(fin, files[i]);
    }
}

void RangeCompressor::UncompressFile(ifstream* fin, FileRecord& fr)
{
    PrintUncompressionStart(fr);

    log4cpp::Category& logger = Parameters::logger;
    Ticks::Start(fr.fileName);

    IModel* model = ModelsFactory::GetModel((ModelType)fr.modelOrder, (CoderType)fr.alg);
    CallBack cb;
    
    uint64_t delta = fr.fileSize / 100;
    uint64_t threshold = 0;

    string realFileName = Parameters::OUTPUT_DIR + "\\" + fr.fileName;
    ofstream fout(realFileName, ios::out | ios::binary);
    if(fout.fail())
        throw file_error("Cannot open file '" + realFileName + "' for writing.");

    if (SHOWP) cb.start();
    model->BeginDecode(fin);

    uchar symbol;
    Context ctx;
    uint64_t decodedBytes = 0;

    while(decodedBytes < fr.fileSize)
    {
        if (SHOWP && decodedBytes > threshold)
        {
            threshold += delta;
            cb.progress((100ull * threshold / fr.fileSize));
        }

        symbol = model->DecodeSymbol(ctx.getCtx()); // TODO как быть когда достаем первые символы и еще нету контекста ??
        ctx.add(symbol); 
        decodedBytes++;
        fout.put(symbol);
    }

    model->StopDecode();
    if (SHOWP) cb.finish();
  
    fout.close();
    logger.info("Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.info("Spent time: %s", millisecToStr(tick).c_str());
}

void RangeCompressor::LoadBlock(std::istringstream& fb, std::ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum)
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

void RangeCompressor::UncompressFileBlock(ifstream* fin, FileRecord& fr)
{
    PrintUncompressionStart(fr);

    log4cpp::Category& logger = Parameters::logger;
    Ticks::Start(fr.fileName);

    IModel* model = ModelsFactory::GetModel((ModelType)fr.modelOrder, (CoderType)fr.alg);
    CallBack cb;

    uint32_t numBlocks = 1 + (uint32_t)(fr.fileSize / fr.blockSize);
    uint32_t delta = numBlocks / 100 + 1;
    uint64_t threshold = 0;

    string realFileName = Parameters::OUTPUT_DIR + "\\" + fr.fileName;
    ofstream fout(realFileName, ios::out | ios::binary);
    if (fout.fail())
        throw file_error("Cannot open file '" + realFileName + "' for writing.");

    bool showprogress = SHOWP;
    if (SHOWP) cb.start();

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
    
    cb.progress(0);

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
                symbol = model->DecodeSymbol(ctx.getCtx()); // TODO как быть когда достаем первые символы и еще нету контекста ??
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
            cb.progress((100ull * threshold / numBlocks));
        }

        //string fn("bwtbufdecode");
        //SaveToFile(fn + to_string(decodedBytes), (char*)bwtbuf, uBlockSize);

        mtf.Decode(bwtbuf, buf, uBlockSize);

        int64_t result = libsais64_unbwt(buf, bwtbuf, temp, uBlockSize, nullptr, lineNum);
        if(result != 0) logger.error("[UncompressFileBlock] unBWT function returned error code.");

        fout.write((char*)bwtbuf, uBlockSize);
    }

    model->StopDecode();
    if (SHOWP) cb.finish();
  
    delete[] buf;
    delete[] bwtbuf;
    delete[] temp;

    logger.info("Extracting done '%s'.", fr.fileName.c_str());
    auto tick = Ticks::Finish(fr.fileName);
    logger.info("Spent time: %s", millisecToStr(tick).c_str());
}

void RangeCompressor::PrintCompressionStart()
{
    log4cpp::Category& logger = Parameters::logger;

    logger.info("Using Arithmetic Range compressor.");
    logger.info("%-19s %s.", "Compression method", Parameters::CoderNames[(uint8_t)Parameters::CODER_TYPE].c_str());
    logger.info("%-19s %s", "Use model type", Parameters::ModelTypeCode[(uint8_t)Parameters::MODEL_TYPE].c_str());
    logger.info("%-19s %s", "Block mode", Parameters::BLOCK_MODE? "YES": "NO");
    if(Parameters::BLOCK_MODE)
        logger.info("%-19s %s bytes", "Block size", toStringSep(Parameters::BLOCK_SIZE).c_str());
    logger.info("%-19s %u", "Threads count", Parameters::THREADS);
    logger.info("%-19s %s", "Verbose", Parameters::VERBOSE? "true": "false");
    logger.info("------------------------------");
}

void RangeCompressor::PrintUncompressionStart(FileRecord& fr)
{
    log4cpp::Category& logger = Parameters::logger;

    //logger.info("Using Arithmetic Range compressor.");
    logger.info("%-21s %s", "Extracting file", fr.fileName.c_str());
    logger.info("%-21s %s", "Extracting to folder", Parameters::OUTPUT_DIR.c_str());
    logger.info("%-21s %s", "Compression method", Parameters::CoderNames[fr.alg].c_str());
    logger.info("%-21s %s", "Model type", Parameters::ModelTypeCode[fr.modelOrder].c_str());
    logger.info("%-21s %s", "Block mode", fr.blockCount > 0 ? "YES" : "NO");
    logger.info("%-21s %s bytes", "Block size", toStringSep(fr.blockSize).c_str());
    logger.info("%-21s %u", "Threads count", Parameters::THREADS);
    logger.info("%-21s %s", "Verbose", Parameters::VERBOSE ? "true" : "false");
    logger.info("------------------------------");
}

void RangeCompressor::PrintFileCompressionDone(FileRecord& fr)
{
    log4cpp::Category& logger = Parameters::logger;

    logger.info("Done compression.");
    logger.info("%-18s '%s'.", "File name", fr.origFilename.c_str());
    logger.info("%-18s %s bytes.", "File size", toStringSep(fr.fileSize).c_str());
    logger.info("%-18s %s bytes.", "Compressed size", toStringSep(fr.compressedSize).c_str());
    logger.info("%-18s %.2f.", "Ratio", (float)fr.fileSize / (float)fr.compressedSize);
    logger.info("%-18s %s", "Blocks", toStringSep(fr.blockCount).c_str());
    logger.info("------------------------------");
}
