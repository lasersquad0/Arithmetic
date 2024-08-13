
#include <filesystem>
#if defined (__BORLANDC__)
#include <share.h>
#endif
#include "fpaq0.h"
#include "RangeCoder.h"
#include "BasicModel.h"
#include "Context.h"
#include "Ticks.h"
#include "CommonFunctions.h"
#include "libsais64.h"
#include "MTF.h"
#include "Factories.h"
#include "Archiver.h"

using namespace std;

#define ARC_EXT _T(".ar")

#define SHOW_PROGRESS_AFTER 1000
#define SHOWP (fr.fileSize > SHOW_PROGRESS_AFTER)


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

// if ArchiveFileName does not have '.ar' extension it will be added
// if ArchiveFileName exists - it will be overwritten
// if ArchiveFileName does not exist it will be created
int Archiver::CompressFile(string_t ArchiveFileName, const string_t& FileName, const Parameters& params)
{
	vect_string_t files;

	files.push_back(FileName);
	return CompressFiles(ArchiveFileName, files, params);
}

int Archiver::CompressFiles(string_t ArchiveFileName, const vect_string_t& files, const Parameters& params)
{
	//ConsoleProgressCallback ccb;
	//cbmanager.AddCallback(&ccb);

	GET_LOGGER();

	if (ArchiveFileName.substr(ArchiveFileName.size() - 3, 3) != ARC_EXT) ArchiveFileName += ARC_EXT;

#ifdef LOG4CPP
	logger.info("Creating archive '%s'.", ArchiveFileName.c_str());
	PrintCompressionStart(params);
	logger.info("Adding %d file%s to archive.", files.size() - 1, files.size() > 2 ? "s" : "");
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Creating archive '%s'.", convert_string<char>(ArchiveFileName).c_str());
	PrintCompressionStart(params);
	logger.LogFmt(LogEngine::Levels::llInfo, "Adding %d file%s to archive.", files.size(), files.size() > 1 ? "s" : "");
#else
	logger.InfoFmt("Creating archive '{}'.", convert_string<char>(ArchiveFileName));
	PrintCompressionStart(params);
	logger.InfoFmt("Adding {} file{} to archive.", files.size(), files.size() > 1 ? "s" : "");
#endif
	LOG_INFO("------------------------------");

	ofstream fout;
	fout.open(ArchiveFileName, ios::out | ios::binary, _SH_DENYWR);
	if (fout.fail())
		throw file_error("Cannot open file '" + convert_string<char>(ArchiveFileName) + "' for writing. Check file permissions.");

	ArchiveHeader hd;
	vect_fr_t& frs = hd.FillFileRecs(files, params);
	hd.SaveHeader(&fout);

	int result = CALLBACK_OK;
	bool aborted = false;
	for (size_t i = 0; i < frs.size(); i++)
	{
		IModel* model = ModelCoderFactory::GetModelAndCoder(frs[i]);

		FileRecord& fr = frs[i];
		ifstream fin(fr.origFilename, ios::in | ios::binary);
		if (fin.fail())
			throw file_error("Cannot open file '" + convert_string<char>(fr.origFilename) + "' for reading.");

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

	//cbmanager.RemoveCallback(&ccb);

	if (aborted)
	{
		std::filesystem::remove(ArchiveFileName); // remove partially created archive file
		LOG_INFO("User has cancelled compression process. Aborting.");
	}
	else
	{
		hd.updateHeaders(ArchiveFileName);
		LOG_INFO("All files are compressed.");
	}

	return result;
}


int Archiver::CompressFile(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
	GET_LOGGER();
#ifdef LOG4CPP
	logger.info("Compressing file '%s'.", fr.origFilename.c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '%s'.", convert_string<char>(fr.origFilename).c_str());
#else
	logger.InfoFmt("Compressing file '{}'.", convert_string<char>(fr.origFilename));
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

	bool showprogress = SHOWP;
	if (showprogress) cbmanager.Start();

	model->BeginEncode(fout);
	int RESULT = CALLBACK_OK;
	Context ctx;

	while(true)
	{
		uchar sm = (uchar)fin->get();
		if (fin->eof()) break;

		cntRead++;

		if (showprogress && (cntRead > threshold))
		{
			threshold += delta;
			if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold/fr.fileSize))) // user has aborted compression process
			{
				RESULT = CALLBACK_ABORT;
				break;
				//return CALLBACK_ABORT;
			}
		}

		model->EncodeSymbol(ctx.getCtx(), sm);
		ctx.add(sm);
	}

	model->StopEncode();
	if (showprogress) cbmanager.Finish();

	fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO makes sense do it better and more convenient

	if (RESULT == CALLBACK_ABORT)
		PrintFileCompressionAborted(fr);
	else
		PrintFileCompressionDone(fr);

	auto tick = Ticks::Finish(fr.origFilename);
#ifdef LOG4CPP
	logger.info("%-16s %s", "Spent time:", millisecToStr(tick).c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Spent time:", millisecToStr(tick).c_str());
#else
	logger.InfoFmt("{:16} {}", "Spent time:", millisecToStr(tick));
#endif
	//LOG_INFO3("{:16} {}", "Spent time:", millisecToStr(tick));
	LOG_INFO("------------------------------");

	return RESULT;
}

// assuming that dest buffer is large enough
void WCHARtoChar(char* dest, const wchar_t* src)
{
	//size_t len;
	int res = WideCharToMultiByte(CP_ACP, 0, src, -1, dest, MAX_PATH, nullptr, nullptr);
	assert(res != 0);
	//wcstombs_s(&len, dest, MAX_PATH, src, wcslen(src));
	//if (len > 0u) dest[len] = '\0';
}


int Archiver::CompressFileBlock(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
	GET_LOGGER();

#ifdef LOG4CPP
	logger.info("Compressing file '%s'.", fr.origFilename.c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Compressing file '%s'.", convert_string<char>(fr.origFilename).c_str());
#else
	//std::string charbuf;
	//charbuf.resize(MAX_PATH);
	//WCHARtoChar(charbuf.data(), fr.origFilename.c_str());
// sometimes convert_string<char> generates an exception during conversation, code above - is not.
// neither of it works properly with, for instance, russian symbols in file name.
	std::string oemStr;
	oemStr.resize(fr.origFilename.size());
	//std::string str2 = convert_string<char>(fr.origFilename);
	//CharToOemA(str2.data(), oemStr.data());
	CharToOemW(fr.origFilename.data(), oemStr.data());
	logger.InfoFmt("Compressing file '{}'.", oemStr);
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

	bool showprogress = SHOWP;

	if (showprogress) cbmanager.Start();

	ostringstream fblock;
	model->BeginEncode(&fblock);

	Context ctx;
	uint32_t i = 0;
	int64_t lineNum = -1;
	uint8_t* buf    = new uint8_t[fr.blockSize];
	uint8_t* bwtbuf = new uint8_t[fr.blockSize];
	int64_t* temp   = new int64_t[fr.blockSize + 1];
	//int64_t freq[256];
	MTF mtf;

	if (showprogress) cbmanager.Progress(0);
	int RESULT = CALLBACK_OK;
	while (true)
	{
		fin->read((char*)buf, fr.blockSize);
		streamsize cntRead = fin->gcount();

		if (cntRead == 0) break; // for the case when (filesize % fr.blockSize)==0

		//string fn = "bufencode";
		//SaveToFile(fn + to_string(cntBlocks), (char*)buf, cntRead);

		lineNum = libsais64_bwt(buf, bwtbuf, temp, cntRead, 0, nullptr);

		if (lineNum < 0) LOG_ERROR("[CompressFileBlock] BWT function returned error code.");

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
			{
				RESULT = CALLBACK_ABORT;
				break;
				//return CALLBACK_ABORT; // TODO immediate return causes memory leak
			}
		}

		if (fin->eof()) break;
	}

	model->StopEncode();
	if (showprogress) cbmanager.Finish();

	delete [] buf;
	delete [] bwtbuf;
	delete [] temp;

	fr.compressedSize = model->GetCoder().GetBytesPassed(); // TODO makes sense do it better and more convenient
	fr.blockCount = cntBlocks;

	if (RESULT == CALLBACK_ABORT)
		PrintFileCompressionAborted(fr);
	else
		PrintFileCompressionDone(fr);

	auto tick = Ticks::Finish(fr.origFilename);
#ifdef LOG4CPP
	logger.info("%-16s %s", "Spent time:", millisecToStr(tick).c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Spent time:", millisecToStr(tick).c_str());
#else
	logger.InfoFmt("{:16} {}", "Spent time:", millisecToStr(tick));
#endif
	LOG_INFO("------------------------------");
	return RESULT;
}


int Archiver::UncompressFiles(const string_t& ArchiveFileName, const Parameters& params)
{
	GET_LOGGER();

	ifstream fin(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file " + convert_string<char>(ArchiveFileName) + "for reading. Exiting.");

	//ConsoleProgressCallback ccb;
	//cbmanager.AddCallback(&ccb);

	ArchiveHeader ah;
	auto files = ah.LoadHeader(&fin);

	int RESULT = CALLBACK_OK;
	for (size_t i = 0; i < files.size(); i++)
	{
		IModel* model = ModelCoderFactory::GetModelAndCoder(files[i]);
		//string_t temp = params.OUTPUT_DIR;
		string_t realFileName = params.OUTPUT_DIR + _T("\\") + files[i].fileName;
		ofstream fout(realFileName, ios::out | ios::binary, _SH_DENYWR);
		if (fout.fail())
			throw file_error("Cannot open file '" + convert_string<char>(realFileName) + "' for writing.");

		PrintUncompressionStart(files[i], params);

		if (files[i].blockCount > 0)
			RESULT = UncompressFileBlock(&fin, &fout, files[i], model);
		else
			RESULT = UncompressFile(&fin, &fout, files[i], model);

		fout.close();

		if (RESULT == CALLBACK_ABORT)
		{
			std::filesystem::remove(realFileName); // remove partially uncompressed file
			break;
		}
	}

	//cbmanager.RemoveCallback(&ccb);

	if (RESULT == CALLBACK_ABORT)
	{
		LOG_INFO("User has cancelled uncompression process. Aborting.");
	}

	return RESULT;
}

int Archiver::UncompressFile(ifstream* fin, ofstream* fout, FileRecord& fr, IModel* model)
{
	GET_LOGGER();

	Ticks::Start(fr.fileName);

	//CallBack cb;

	uint64_t delta = fr.fileSize / 100;
	uint64_t threshold = 0;

  /*  string realFileName = ParametersG::OUTPUT_DIR + "\\" + fr.fileName;
	ofstream fout(realFileName, ios::out | ios::binary);
	if(fout.fail())
		throw file_error("Cannot open file '" + realFileName + "' for writing.");*/

	bool showprogress = SHOWP;
	if (showprogress) cbmanager.Start();
	model->BeginDecode(fin);

	uchar symbol;
	Context ctx;
	uint64_t decodedBytes = 0;

	try
	{
		while (decodedBytes < fr.fileSize)
		{
			if (showprogress && decodedBytes > threshold)
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
	if (showprogress) cbmanager.Finish();

   // fout.close();

#ifdef LOG4CPP
	logger.info("Extracting done '%s'.", fr.fileName.c_str());
	auto tick = Ticks::Finish(fr.fileName);
	logger.info("%-19s %s", "Spent time:", millisecToStr(tick).c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '%s'.", fr.fileName.c_str());
	auto tick = Ticks::Finish(fr.fileName);
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Spent time:", millisecToStr(tick).c_str());
#else
	logger.InfoFmt("Extracting done '{}'.", convert_string<char>(fr.fileName));
	auto tick = Ticks::Finish(fr.fileName);
	logger.InfoFmt("{:19} {}", "Spent time:", convert_string<char>(millisecToStr(tick)));
#endif
	LOG_INFO("------------------------------");
	return CALLBACK_OK;
}


int Archiver::UncompressFileBlock(ifstream* fin, ofstream *fout, FileRecord& fr, IModel* model)
{
	GET_LOGGER();
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
	if (showprogress) cbmanager.Start();

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

	if (showprogress) cbmanager.Progress(0);

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

		if (result != 0) LOG_ERROR("[UncompressFileBlock] unBWT function returned error code.");

		fout->write((char*)bwtbuf, uBlockSize);
	}

	model->StopDecode();
	if (showprogress) cbmanager.Finish();

	delete[] buf;
	delete[] bwtbuf;
	delete[] temp;
#ifdef LOG4CPP
	logger.info("Extracting done '%s'.", fr.fileName.c_str());
	auto tick = Ticks::Finish(fr.fileName);
	logger.info("%-19s %s", "Spent time:", millisecToStr(tick).c_str());
#elif defined (__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Extracting done '%s'.", convert_string<char>(fr.fileName).c_str());
	auto tick = Ticks::Finish(fr.fileName);
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Spent time:", convert_string<char>(millisecToStr(tick)).c_str());
#else
	logger.InfoFmt("Extracting done '{}'.", convert_string<char>(fr.fileName));
	auto tick = Ticks::Finish(fr.fileName);
	logger.InfoFmt("{:19} {}", "Spent time:", convert_string<char>(millisecToStr(tick)));
#endif
	LOG_INFO("------------------------------");

	return CALLBACK_OK;
}

// ArchiveFileName should be fully qualified file name with extension (.ar)
// Archive extension will NOT be added automatically in contrast to CompressFile(s) methods
int Archiver::ExtractFiles(const string_t& ArchiveFileName, const vect_string_t& FilesToExtract, const string_t& ExtractDir, bool justTest)
{
	Parameters params;
	params.OUTPUT_DIR = ExtractDir;
	return ExtractFiles(ArchiveFileName, FilesToExtract, params, justTest);
}

int Archiver::ExtractFiles(const string_t& ArchiveFileName, const vect_string_t& FilesToExtract, const Parameters& params, bool justTest)
{
	int result = CALLBACK_OK;
	for (auto fl: FilesToExtract)
	{
		result = ExtractFile(ArchiveFileName, fl, params, justTest);
		if (result == CALLBACK_ABORT) break;
	}

	return result;
}

int Archiver::ExtractFile(const string_t& ArchiveFileName, const string_t& FileToExtract, const string_t& ExtractDir, bool justTest)
{
	Parameters params;
	params.OUTPUT_DIR = ExtractDir;
	return ExtractFile(ArchiveFileName, FileToExtract, params, justTest);
}

int Archiver::ExtractFile(const string_t& ArchiveFileName, const string_t& FileToExtract, const Parameters& params, bool justTest)
{
	GET_LOGGER();

	ifstream fin(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file '" + convert_string<char>(ArchiveFileName) + "' for reading. Exiting.");

	ArchiveHeader ah;
	auto files = ah.LoadHeader(&fin);

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
		throw file_error("File '" + convert_string<char>(FileToExtract) + "' absent in archive. Cannot extract.");

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

	ofstream fout;
	string_t realFileName = params.OUTPUT_DIR + _T("\\") + FileToExtract;

	if (justTest)
	{
		//fout.setstate(std::ios_base::badbit);
		// here is a trick.
		// fout is not opened while archive testing, nothing is written there when we write
	}
	else
	{
		fout.open(realFileName, ios::out | ios::binary, _SH_DENYWR);
		if (fout.fail())
			throw file_error("Cannot open file '" + convert_string<char>(realFileName) + "' for writing.");

		PrintUncompressionStart(fr, params);
	}

	IModel* model = ModelCoderFactory::GetModelAndCoder(fr);

	//ConsoleProgressCallback ccb;
	//cbmanager.AddCallback(&ccb);

	int result;
	if (fr.blockCount > 0)
		result = UncompressFileBlock(&fin, &fout, fr, model);
	else
		result = UncompressFile(&fin, &fout, fr, model);

	fout.close();
	//cbmanager.RemoveCallback(&ccb);

	if (result == CALLBACK_ABORT)
	{
		// no need to remove any files during archive testing
		if (!justTest) filesystem::remove(realFileName); // deleting partially extracted file
		LOG_INFO("User has cancelled file extracting process. Aborting.");
	}

	return result;
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

void Archiver::SaveBlock(ostringstream& fb, ofstream* fout, uint32_t uBlockSize, int32_t lineNum)
{
	string s = fb.str();

	uint32_t cBlockSize = (uint32_t)s.size();
	char flags = 0;

	fout->write((char*)&cBlockSize, sizeof(uint32_t)); // compressed size
	fout->write((char*)&uBlockSize, sizeof(uint32_t)); // uncompressed size
	fout->write((char*)&lineNum, sizeof(int32_t));
	fout->write((char*)&flags, sizeof(char));

	fout->write(s.data(), cBlockSize);
	fb.str(""); //clear output block buffer

	//string fn = "compressedblocksaved" + to_string(lineNum);
	//SaveToFile(fn, s.data(), len);
}

bool Archiver::BypassFile(ifstream* fin, const FileRecord& fr)
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

bool Archiver::CopyFileData(ifstream* fin, ofstream* fout, const FileRecord& fr)
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
        uint32_t bufSize = INIT_BUFSIZE;

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

// ArchiveFileName should be fully qualified file name with extension (.ar)
// Archive extension will NOT be added automatically in contrast to CompressFile(s) methods
void Archiver::RemoveFile(const string_t& ArchiveFileName, const string_t& FileToDelete)
{
	GET_LOGGER();

    ifstream fin(ArchiveFileName, ios::in | ios::binary);
    if (fin.fail())
		throw file_error("Cannot open archive file '" + convert_string<char>(ArchiveFileName) + "' for reading. Exiting.");

	ArchiveHeader ah;
	vect_fr_t& files = ah.LoadHeader(&fin);

	vect_fr_t filesOLD = files;

	if (!ah.FileInList(FileToDelete)) //incorrect file name
	{
		throw invalid_argument("File '" + convert_string<char>(FileToDelete) + "' cannot be found in archive '" + convert_string<char>(ArchiveFileName) + "'.");
	}
#ifdef LOG4CPP
	logger.info("Deleting file '%s' from archive '%s'", FileToDelete.c_str(), ArchiveFile.c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Deleting file '%s' from archive '%s'", convert_string<char>(FileToDelete).c_str(), convert_string<char>(ArchiveFileName).c_str());
#else
    logger.InfoFmt("Deleting file '{}' from archive '{}'", convert_string<char>(FileToDelete), convert_string<char>(ArchiveFileName));
#endif

	ah.RemoveFileFromList(FileToDelete);

    // TODO: add handling of removing last files from archive.

    string_t newArchiveFile = ArchiveFileName + _T(".tmp");
    ofstream fout(newArchiveFile, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + convert_string<char>(newArchiveFile) + "' for writing.");

    ah.SaveHeader(&fout); // saving header without deleted file in the list


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
		std::remove(convert_string<char>(newArchiveFile).c_str());
		throw file_error("Error reading archive file '" + convert_string<char>(ArchiveFileName) + "'.");
	}

	if (fout.fail())
	{
		std::remove(convert_string<char>(newArchiveFile).c_str());
		throw file_error("Error writing archive file '" + convert_string<char>(newArchiveFile) + "'.");
	}

	std::remove(convert_string<char>(ArchiveFileName).c_str());
	std::rename(convert_string<char>(newArchiveFile).c_str(), convert_string<char>(ArchiveFileName).c_str());

	LOG_INFO("Deleted successfully");
}

// ArchiveFileName should be fully qualified file name with extension (.ar)
// Archive extension will NOT be added automatically in contrast to CompressFile(s) methods
void Archiver::RemoveFiles(const string_t& ArchiveFileName, const vect_string_t& flist)
{
    if (flist.size() == 0) return;

	GET_LOGGER();

    ifstream fin(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file '" + convert_string<char>(ArchiveFileName) + "' for reading. Exiting.");

    ArchiveHeader ah;
    vect_fr_t& files = ah.LoadHeader(&fin);

    vect_fr_t filesOLD = files;

    //if (!ah.FileInList(FileToDelete)) //incorrect file name
    //{
    //    throw invalid_argument("File '" + FileToDelete + "' cannot be found in archive '" + ArchiveFile + "'.");
    //}
#ifdef LOG4CPP
    logger.info("Deleting files from archive '%s'", ArchiveFile.c_str());
#elif defined(__BORLANDC__)
	logger.LogFmt(LogEngine::Levels::llInfo, "Deleting files from archive '%s'", convert_string<char>(ArchiveFileName).c_str());
#else
	logger.InfoFmt("Deleting files from archive '{}'", convert_string<char>(ArchiveFileName));
#endif

    ah.RemoveFilesFromList(flist);

    // TODO add handling of removing last file from archive.

    string_t newArchiveFile = ArchiveFileName + _T(".tmp");
    ofstream fout(newArchiveFile, ios::out | ios::binary, _SH_DENYWR);
    if (fout.fail())
        throw file_error("Cannot open file '" + convert_string<char>(newArchiveFile) + "' for writing.");

    ah.SaveHeader(&fout); // saving header without deleted file in the list


    //for (size_t i = 0; i < filesOLD.size(); i++)
    for (vect_fr_t::iterator iter = filesOLD.begin(); iter != filesOLD.end(); iter++)
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
		std::remove(convert_string<char>(newArchiveFile).c_str());
		throw file_error("Error reading archive file '" + convert_string<char>(ArchiveFileName) + "'.");
	}

	if (fout.fail())
	{
		fout.close();
		std::remove(convert_string<char>(newArchiveFile).c_str());
		throw file_error("Error writing archive file '" + convert_string<char>(newArchiveFile) + "'.");
	}

	fin.close();
	fout.close();

	std::remove(convert_string<char>(ArchiveFileName).c_str());
	std::rename(convert_string<char>(newArchiveFile).c_str(), convert_string<char>(ArchiveFileName).c_str());

    LOG_INFO("Deleted successfully");
}

void Archiver::PrintCompressionStart(const Parameters& params)
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
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s.", "Compression coder:", convert_string<char>(Parameters::CoderNames[(uint8_t)params.CODER_TYPE]).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Use model type:", convert_string<char>(Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE]).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Block mode:", params.BLOCK_MODE ? "YES" : "NO");
	if (params.BLOCK_MODE)
		logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s bytes", "Block size:", toStringSepA(params.BLOCK_SIZE).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %u", "Threads count:", params.THREADS);
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
#else
	LogEngine::Logger& logger = Global::GetLogger();
	//logger.info("Using Arithmetic Range compressor.");
	logger.InfoFmt("{:19} {}.", "Compression coder:", convert_string<char>(Parameters::CoderNames[(uint8_t)params.CODER_TYPE]));
	logger.InfoFmt("{:19} {}", "Use model type:", convert_string<char>(Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE]));
	logger.InfoFmt("{:19} {}", "Block mode:", params.BLOCK_MODE ? "YES" : "NO");
	if (params.BLOCK_MODE)
		logger.InfoFmt("{:19} {} bytes", "Block size:", toStringSepA(params.BLOCK_SIZE));
	logger.InfoFmt("{:19} {}", "Threads count:", params.THREADS);
	logger.InfoFmt("{:19} {}", "Verbose:", params.VERBOSE ? "true" : "false");
#endif

}

void Archiver::PrintUncompressionStart(const FileRecord& fr, const Parameters& params)
{
#ifdef LOG4CPP
    log4cpp::Category& logger = Global::GetLogger();

    //logger.info("Using Arithmetic Range compressor.");
	logger.info("%-19s %ls", "Extracting file:", fr.fileName.c_str());
	logger.info("%-19s %ls", "Extracting to:", params.OUTPUT_DIR.c_str());
	logger.info("%-19s %s", "Compression coder:", Parameters::CoderNames[fr.alg].c_str());
	logger.info("%-19s %s", "Model type:", Parameters::ModelTypeCode[fr.modelOrder].c_str());
	logger.info("%-19s %s", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
	logger.info("%-19s %ls bytes", "Block size:", toStringSep(fr.blockSize).c_str());
	logger.info("%-19s %u", "Threads count:", params.THREADS);
	logger.info("%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
	//logger.info("------------------------------");
#elif defined(__BORLANDC__)
    LogEngine::Logger& logger = Global::GetLogger();
    //logger.info("Using Arithmetic Range compressor.");

	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Extracting file:", convert_string<char>(fr.fileName).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Extracting to:", convert_string<char>(params.OUTPUT_DIR).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Compression coder:", convert_string<char>(Parameters::CoderNames[fr.alg]).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Model type:", convert_string<char>(Parameters::ModelTypeCode[fr.modelOrder]).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s bytes", "Block size:", toStringSepA(fr.blockSize).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %u", "Threads count:", params.THREADS);
	logger.LogFmt(LogEngine::Levels::llInfo, "%-19s %s", "Verbose:", params.VERBOSE ? "true" : "false");
	//logger.info("------------------------------");
#else
	LogEngine::Logger& logger = Global::GetLogger();
	//logger.info("Using Arithmetic Range compressor.");

	logger.InfoFmt("{:19} {}", "Extracting file:", convert_string<char>(fr.fileName));
	logger.InfoFmt("{:19} {}", "Extracting to:", convert_string<char>(params.OUTPUT_DIR));
	logger.InfoFmt("{:19} {}", "Compression coder:", convert_string<char>(Parameters::CoderNames[fr.alg]));
	logger.InfoFmt("{:19} {}", "Model type:", convert_string<char>(Parameters::ModelTypeCode[fr.modelOrder]));
	logger.InfoFmt("{:19} {}", "Block mode:", fr.blockCount > 0 ? "YES" : "NO");
	logger.InfoFmt("{:19} {} bytes", "Block size:", toStringSepA(fr.blockSize));
	logger.InfoFmt("{:19} {}", "Threads count:", params.THREADS);
	logger.InfoFmt("{:19} {}", "Verbose:", params.VERBOSE ? "true" : "false");
	//logger.info("------------------------------");
#endif // LOG4CPP
}

void Archiver::PrintFileCompressionDone(const FileRecord& fr)
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
	std::string oemFilename;
	oemFilename.resize(fr.origFilename.size());
	CharToOemBuffW(fr.origFilename.data(), oemFilename.data(), fr.origFilename.size());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s '%s'", "File name:", oemFilename.c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s bytes", "File size:", toStringSepA(fr.fileSize).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s bytes", "Compressed size:", toStringSepA(fr.compressedSize).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %.2f", "Ratio:", (float)fr.fileSize / (float)fr.compressedSize);
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s", "Blocks:", toStringSepA(fr.blockCount).c_str());
#else
	LogEngine::Logger& logger = Global::GetLogger();
	logger.Info("Done compression.");
	logger.InfoFmt("{:16} '{}'", "File name:", toOEM(fr.origFilename));
	logger.InfoFmt("{:16} {} bytes", "File size:", toStringSepA(fr.fileSize));
	logger.InfoFmt("{:16} {} bytes", "Compressed size:", toStringSepA(fr.compressedSize));
	logger.InfoFmt("{:16} {}", "Ratio:", (float)fr.fileSize / (float)fr.compressedSize);
	logger.InfoFmt("{:16} {}", "Blocks:", toStringSepA(fr.blockCount));
#endif // LOG4CPP

}

void Archiver::PrintFileCompressionAborted(const FileRecord& fr)
{
#ifdef LOG4CPP

#elif defined(__BORLANDC__)
	LogEngine::Logger& logger = Global::GetLogger();
	logger.Warn("Compression aborted by user.");
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s '%s'", "File name:", convert_string<char>(fr.origFilename).c_str());
	logger.LogFmt(LogEngine::Levels::llInfo, "%-16s %s bytes", "File size:", toStringSepA(fr.fileSize).c_str());
#else
	LogEngine::Logger& logger = Global::GetLogger();
	logger.Info("Compression aborted by user.");
	logger.InfoFmt("{:16} '{}'", "File name:", toOEM(fr.origFilename));
	logger.InfoFmt("{:16} {} bytes", "File size:", toStringSepA(fr.fileSize));
#endif // LOG4CPP

}
