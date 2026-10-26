
#include <filesystem>
#include <algorithm>

//#include "fpaq0.h"
//#include "RangeCoder.h"
//#include "BasicModel.h"
#include "Context.h"
#include "utils/include/Ticks.h"
#include "CommonFunctions.h"
//#include "libsais64.h"
#include "MTF.h"
#include "Factories.h"
#include "Archiver.h"
#include "LogEngine2/LogEngine.h"
#include "thread_pool.h"

using namespace std;

#define ARC_EXT U(".ar")

#define SHOW_PROGRESS_AFTER 1000
#define SHOWP (fr.fileSize > SHOW_PROGRESS_AFTER)
#define TICK_COMPRESS_FILE "compressfile"
#define TICK_UNCOMPRESS_FILE "uncompressfile"

const uint FIELD_W = 19;
const uint FILL_LENGTH = 30; // number of '-' syms in line '---...---'
const char FILL_SYM = '-';

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
// ArchiveFileName passed by value intentionally
int Archiver::CompressFile(string_t archiveFileName, const string_t& fileName, const Parameters& params)
{
	vect_string_t files;

	files.push_back(fileName);
	return CompressFiles(archiveFileName, files, params);
}

// ArchiveFileName passed by value intentionally
int Archiver::CompressFiles(string_t archiveFileName, const vect_string_t& files, const Parameters& params)
{
	//ConsoleProgressCallback ccb;
	//cbmanager.AddCallback(&ccb);

	GET_LOGGER();

	if (archiveFileName.substr(archiveFileName.size() - 3, 3) != ARC_EXT) archiveFileName += ARC_EXT;

	PrintCompressionStart(params, archiveFileName, files);
	//LOG_INFO(std::string(30, '-'));

	ofstream fout;
	fout.open(archiveFileName, ios::out | ios::binary/*, _SH_DENYWR*/); // third parameter is not supported by C++ standard
	if (fout.fail())
		throw file_error("Cannot open file '" + wtos(archiveFileName) + "' for writing. Check file permissions.");

	ArchiveHeader hd;
	vect_fr_t& frs = hd.FillFileRecs(files, params);
	hd.SaveHeader(&fout);

	int result = CALLBACK_OK;
	bool aborted = false;
	for (size_t i = 0; i < frs.size(); i++)
	{
		IModel* model = ModelCoderFactory::GetModelAndCoder(frs[i]);

		FileRecord& fr = frs[i];
		ifstream fin;
		fin.open(std::filesystem::path(fr.origFilename), ios::in | ios::binary);
		if (fin.fail())
			throw file_error("Cannot open file '" + wtos(fr.origFilename) + "' for reading.");

		if (params.BLOCK_MODE)
		{
			if(params.THREADS > 1)
				result = CompressFileBlockInThread(&fout, &fin, fr, model, params);
			else
				result = CompressFileBlock(&fout, &fin, fr, model);
		}
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
		std::filesystem::remove(archiveFileName); // remove partially created archive file
		LOG_INFO("User has cancelled compression process. Aborting.");
	}
	else
	{
		hd.UpdateHeaders(archiveFileName);
		LOG_INFO("All files are compressed.");
	}

	return result;
}

int Archiver::CompressFile(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
	GET_LOGGER();

	logger.InfoFmt("{:<{}}: '{}'", "Compressing file", FIELD_W, wtos(fr.origFilename));
	Ticks::Start(TICK_COMPRESS_FILE);

   // IModel* model = ModelsFactory::GetModel();
   // CallBack cb;

	uint64_t delta = fr.fileSize / 100ull;
	uint64_t threshold = 0;
	uint64_t cntRead = 0;

	/*ifstream fin(fr.origFilename, ios::in | ios::binary);
	if (fin.fail())
	{
		throw file_error("Cannot open file '" + fr.origFilename + "' for reading.");
	}*/

	bool showprogress = SHOWP;
	if (showprogress) cbmanager.Start();

	model->BeginEncode(fout, fin);
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
			}
		}

		model->EncodeSymbol(ctx.getCtx(), sm);
		ctx.add(sm);
	}

	model->StopEncode();
	if (showprogress) cbmanager.Finish();

	fr.compressedSize = model->GetBytesPassed();

	Ticks::Finish(TICK_COMPRESS_FILE);

	if (RESULT == CALLBACK_ABORT)
		PrintFileCompressionAborted(fr);
	else
		PrintFileCompressionDone(fr);

	//logger.InfoFmt("{:16} {}", "Spent time:", MillisecToStr(tick));
	//LOG_INFO3("{:16} {}", "Spent time:", millisecToStr(tick));
	//LOG_INFO("------------------------------");

	return RESULT;
}

bool Archiver::LoadBlocksToBWTTasks(ifstream* fin, MT::ThreadPool* ppool, uint32_t blockSize) const
{
	size_t tasksInQueue = ppool->task_queue_size();
	if (tasksInQueue >= MAX_TASKS) return true;
	uint32_t blocksLoaded = 0;
	
	GET_LOGGER();
	logger.Debug("[LoadBlocksToBWTTasks] Loading blocks ...");

	while (tasksInQueue < MAX_TASKS)
	{
		auto task = std::make_shared<BWTTask>(blockSize);
		fin->read((char*)task->Buf, blockSize);
		task->ReadSize = (uint32_t)fin->gcount();
		
		// no sense to add task with zero data size
		if(task->ReadSize > 0)
			ppool->add_task(task);

		blocksLoaded++;

		if (task->ReadSize < blockSize) return false; // no more data blocks in a file
		
		tasksInQueue++;
	}

	logger.DebugFmt("[LoadBlocksToBWTTasks] Blocks loaded: {}", blocksLoaded);
	//logger.DebugFmt("[LoadBlocksToBWTTasks] NEW blocks created: " + newBlocks);
	//logger.DebugFmt("[LoadBlocksToBWTTasks] REUSED blocks: " + reusedBlocks);

	return true; 
}

int Archiver::CompressFileBlockInThread(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model, const Parameters& params)
{
	GET_LOGGER();

	logger.InfoFmt("Compressing file '{}'.", wtos(fr.origFilename));

	// pool will not contain more than MAX_TASKS tasks.
	MAX_TASKS = std::max(MAX_TASKS_MEMORY_USAGE / (2 * fr.blockSize + 8 * fr.blockSize), 2u); // buf + bwtbuf + tmpbuf

	Ticks::Start(TICK_COMPRESS_FILE);

	// IModel* model = ModelsFactory::GetModel();
   //  CallBack cb;

	uint32_t numBlocks = 1 + (uint32_t)(fr.fileSize / fr.blockSize);
	uint32_t delta = numBlocks / 100 + 1;
	uint32_t threshold = 0;
	uint32_t cntBlocks = 0;

	bool showprogress = SHOWP;
	if (showprogress) cbmanager.Start();

	ostringstream fblock;
	model->BeginEncode(&fblock, fin); // for O0FIX model: nullptr tells to the model - do NOT calculate Freqs on BeginEncode

	Context ctx;
	uint32_t i = 0;
	if (showprogress) cbmanager.Progress(0);
	int RESULT = CALLBACK_OK;

	MT::ThreadPool pool(params.THREADS); //TODO how to get number of processor cores?
	pool.set_logger_flag(false);
	pool.start(); // remove the pool from a pause, allowing streams to take on the tasks on the fly

	logger.DebugFmt("MAX_TASKS = {}", MAX_TASKS);

	bool moreBlocks = LoadBlocksToBWTTasks(fin, &pool, fr.blockSize);
	MT::task_id blockID = 1; // because pool.last_task_id starts from 1 too

	uint32_t sleepTime = 100; // millisec
	//if (fr.blockSize <= 1 << 16) sleepTime = 5;
	//if (fr.blockSize > 1 << 22) sleepTime = 500;

	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));

		size_t tasksInQueue = pool.task_queue_size();

		if (moreBlocks && (tasksInQueue < (MAX_TASKS/2)) ) // load more blocks only after half of blocks have been BWTed
			moreBlocks = LoadBlocksToBWTTasks(fin, &pool, fr.blockSize);
		
		// pop_result returns nullptr if result is not ready yet.
		auto task = pool.pop_result<BWTTask>(blockID);
		if (task)
		{
			logger.DebugFmt("Block #{} started compressing", blockID);

			model->BeginBlockEncode(task->Buf, task->ReadSize);

			for (i = 0; i < task->ReadSize; i++)
			{
				model->EncodeSymbol(ctx.getCtx(), (uchar)task->Buf[i]);
				ctx.add((uchar)task->Buf[i]);
			}

			model->StopBlockEncode(); // save 'last 4 bytes' into block.

			logger.DebugFmt("Block #{} finsihed compressing", blockID);

			SaveBlock(fblock, fout, i, (uint32_t)task->LineNum); //TODO shall we use task->ReadSize instead of i ?

			logger.DebugFmt("Current task count in a queue:{}, completed: {}", pool.task_queue_size(), pool.tasks_completed());

			cntBlocks++;
			if (showprogress && (cntBlocks > threshold))
			{
				threshold += delta;
				if (CALLBACK_ABORT == cbmanager.Progress((100ull * threshold / numBlocks)))
				{
					RESULT = CALLBACK_ABORT;
					break;
					//return CALLBACK_ABORT; //TODO immediate return causes memory leak
				}
			}

			blockID++;
		}
		else
		{
			//logger.DebugFmt("[CompressFileBlockInThread] Task #{} is not ready yet, waiting.", blockID);
		}

		// check that all tasks are executed and removed from completed_tasks
		if ((pool.tasks_completed() == 0) && (pool.task_queue_size() == 0) && !pool.is_active()) break; 
		
	}

	model->StopEncode();
	if (showprogress) cbmanager.Finish();

	fr.compressedSize = model->GetBytesPassed(); //TODO makes sense do it better and more convenient
	fr.blockCount = cntBlocks;

	Ticks::Finish(TICK_COMPRESS_FILE);
	
	if (RESULT == CALLBACK_ABORT)
		PrintFileCompressionAborted(fr);
	else
		PrintFileCompressionDone(fr);

	//logger.InfoFmt("{:16} {}", "Spent time:", MillisecToStr(tick));
	//LOG_INFO("------------------------------");
	return RESULT;
}

int Archiver::CompressFileBlock(ofstream* fout, ifstream* fin, FileRecord& fr, IModel* model)
{
	GET_LOGGER();

 	logger.InfoFmt("Compressing file '{}'.", wtos(fr.origFilename));
	Ticks::Start(TICK_COMPRESS_FILE);

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
	model->BeginEncode(&fblock, fin); // for O0FIX model: nullptr tells to the model - do NOT calculate Freqs on BeginEncode

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
		uint32_t cntRead = (uint32_t)fin->gcount();

		if (cntRead == 0) break; // for the case when (filesize % fr.blockSize)==0 i.e. nothing read

		//string fn = "bufencode";
		//SaveToFile(fn + to_string(cntBlocks), (char*)buf, cntRead);

		lineNum = libsais64_bwt(buf, bwtbuf, temp, cntRead, 0, nullptr);

		if (lineNum < 0) LOG_ERROR("[CompressFileBlock] BWT function returned error code.");

		mtf.Encode(bwtbuf, buf, cntRead);

		//fn = "bwtbufencode";
		//SaveToFile(fn + to_string(cntBlocks), (char*)bwtbuf, cntRead);

		model->BeginBlockEncode(buf, fr.blockSize);

		for (i = 0; i < cntRead; i++)
		{
			model->EncodeSymbol(ctx.getCtx(), (uchar)buf[i]);
			ctx.add((uchar)buf[i]);
		}

		model->StopBlockEncode(); // save 'last 4 bytes' into block.

		SaveBlock(fblock, fout, i, (uint32_t)lineNum);

		cntBlocks++;
		if (showprogress && (cntBlocks > threshold))
		{
			threshold += delta;
			if(CALLBACK_ABORT == cbmanager.Progress((100ull * threshold / numBlocks)))
			{
				RESULT = CALLBACK_ABORT;
				break;
				//return CALLBACK_ABORT; //TODO immediate return causes memory leak
			}
		}

		if (fin->eof()) break;
	}

	model->StopEncode();
	if (showprogress) cbmanager.Finish();

	delete [] buf;
	delete [] bwtbuf;
	delete [] temp;

	fr.compressedSize = model->GetBytesPassed(); //TODO makes sense do it better and more convenient
	fr.blockCount = cntBlocks;

	Ticks::Finish(TICK_COMPRESS_FILE);

	if (RESULT == CALLBACK_ABORT)
		PrintFileCompressionAborted(fr);
	else
		PrintFileCompressionDone(fr);
	
	//LOG_INFO("------------------------------");
	return RESULT;
}


int Archiver::UncompressFiles(const string_t& ArchiveFileName, const Parameters& params)
{
	GET_LOGGER();

	ifstream fin;
	fin.open(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file " + wtos(ArchiveFileName) + "for reading. Exiting.");

	//ConsoleProgressCallback ccb;
	//cbmanager.AddCallback(&ccb);

	ArchiveHeader ah;
	auto files = ah.LoadHeader(&fin);

	int RESULT = CALLBACK_OK;
	for (size_t i = 0; i < files.size(); i++)
	{
		IModel* model = ModelCoderFactory::GetModelAndCoder(files[i]);
		//string_t temp = params.OUTPUT_DIR;
		string_t realFileName = params.OUTPUT_DIR + U("\\") + convert_string<char_t>(files[i].fileName);
		ofstream fout;
		fout.open(realFileName, ios::out | ios::binary/*, _SH_DENYWR*/); // third parameter is not supported by C++ standard
		if (fout.fail())
			throw file_error("Cannot open file '" + wtos(realFileName) + "' for writing.");

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

	Ticks::Start(TICK_UNCOMPRESS_FILE);

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

			symbol = model->DecodeSymbol(ctx.getCtx()); //TODO what to do when we extract first few symbols and we do not have context yet??
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

	auto tick = Ticks::Finish(TICK_UNCOMPRESS_FILE);

	logger.InfoFmt("{:<{}}:'{}'", "Extracting status", FIELD_W, "Successful");
	logger.InfoFmt("{:<{}}: {}", "File extracting time", FIELD_W, MillisecToStr(tick));
	LOG_INFO(std::string(FILL_LENGTH, FILL_SYM));

	return CALLBACK_OK;
}


int Archiver::UncompressFileBlock(ifstream* fin, ofstream *fout, FileRecord& fr, IModel* model)
{
	GET_LOGGER();
	Ticks::Start(TICK_UNCOMPRESS_FILE);

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
			model->BeginBlockDecode();
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

		model->StopBlockDecode();

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

	auto tick = Ticks::Finish(TICK_UNCOMPRESS_FILE);
	logger.InfoFmt("{:<{}}: {}", "Extracting status", FIELD_W, "Successfull");
	logger.InfoFmt("{:<{}}: {}", "File extracting time", FIELD_W, MillisecToStr(tick));
	LOG_INFO(std::string(FILL_LENGTH, FILL_SYM));

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

	ifstream fin;
	fin.open(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file '" + wtos(ArchiveFileName) + "' for reading. Exiting.");

	ArchiveHeader ah;
	auto& files = ah.LoadHeader(&fin);

	FileRecord fr;
	bool found = false;
	for (size_t i = 0; i < files.size(); i++)
	{
		if (files[i].fileName == convert_string<wchar_t>(FileToExtract))
		{
			found = true;
			fr = files[i];
			break;
		}
	}

	if (!found)
		throw file_error("File '" + wtos(FileToExtract) + "' absent in archive. Cannot extract.");

	for (size_t i = 0; i < files.size(); i++)
	{
		if (files[i].fileName == convert_string<wchar_t>(FileToExtract))
			break;
		else
			BypassFile(&fin, files[i]);
	}

	ofstream fout;
	string_t realFileName = params.OUTPUT_DIR + U("\\") + FileToExtract;

	if (justTest)
	{
		//fout.setstate(std::ios_base::badbit);
		// here is a trick.
		// fout is not opened while archive testing, nothing is written there when we write
	}
	else
	{
		fout.open(realFileName, ios::out | ios::binary/*, _SH_DENYWR*/); // third parameter is not supported by C++ standard
		if (fout.fail())
			throw file_error("Cannot open file '" + wtos(realFileName) + "' for writing.");

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

	static string s;// (buf, cBlockSize); //TODO make s static? to avoid buffer realocating each time
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

	ifstream fin;
	fin.open(ArchiveFileName, ios::in | ios::binary);
    if (fin.fail())
		throw file_error("Cannot open archive file '" + wtos(ArchiveFileName) + "' for reading. Exiting.");

	ArchiveHeader ah;
	vect_fr_t& files = ah.LoadHeader(&fin);

	vect_fr_t filesOLD = files;

	if (!ah.FileInList(FileToDelete)) //incorrect file name
	{
		throw invalid_argument("File '" + wtos(FileToDelete) + "' cannot be found in archive '" + wtos(ArchiveFileName) + "'.");
	}

    logger.InfoFmt("Deleting file '{}' from archive '{}'", wtos(FileToDelete), wtos(ArchiveFileName));

	ah.RemoveFileFromList(FileToDelete);

	//TODO: add handling of removing last files from archive.

    string_t newArchiveFile = ArchiveFileName + U(".tmp");
	ofstream fout;
	fout.open(newArchiveFile, ios::out | ios::binary/*, _SH_DENYWR*/); // third parameter is not supported by C++ standard
    if (fout.fail())
        throw file_error("Cannot open file '" + wtos(newArchiveFile) + "' for writing.");

    ah.SaveHeader(&fout); // saving header without deleted file in the list


    for (size_t i = 0; i < filesOLD.size(); i++)
    {
        if (filesOLD[i].fileName == convert_string<wchar_t>(FileToDelete))
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
		std::filesystem::remove(newArchiveFile);
		throw file_error("Error reading archive file '" + wtos(ArchiveFileName) + "'.");
	}

	if (fout.fail())
	{
		std::filesystem::remove(newArchiveFile);
		throw file_error("Error writing archive file '" + wtos(newArchiveFile) + "'.");
	}

	std::filesystem::remove(ArchiveFileName);
	std::filesystem::rename(newArchiveFile, ArchiveFileName);

	LOG_INFO("Deleted successfully");
}

// ArchiveFileName should be fully qualified file name with extension (.ar)
// Archive extension will NOT be added automatically in contrast to CompressFile(s) methods
void Archiver::RemoveFiles(const string_t& ArchiveFileName, const vect_string_t& flist)
{
    if (flist.size() == 0) return;

	GET_LOGGER();

	ifstream fin;
	fin.open(ArchiveFileName, ios::in | ios::binary);
	if (fin.fail())
		throw file_error("Cannot open archive file '" + wtos(ArchiveFileName) + "' for reading. Exiting.");

    ArchiveHeader ah;
    vect_fr_t& files = ah.LoadHeader(&fin);

    vect_fr_t filesOLD = files;

    //if (!ah.FileInList(FileToDelete)) //incorrect file name
    //{
    //    throw invalid_argument("File '" + FileToDelete + "' cannot be found in archive '" + ArchiveFile + "'.");
    //}

	logger.InfoFmt("Deleting files from archive '{}'", wtos(ArchiveFileName));

    ah.RemoveFilesFromList(flist);

    //TODO add handling of removing last file from archive.

    string_t newArchiveFile = ArchiveFileName + U(".tmp");
	ofstream fout;
	fout.open(newArchiveFile, ios::out | ios::binary/*, _SH_DENYWR*/); // third parameter is not supported by C++ standard
    if (fout.fail())
        throw file_error("Cannot open file '" + wtos(newArchiveFile) + "' for writing.");

    ah.SaveHeader(&fout); // saving header without deleted file in the list

    for (auto iter = filesOLD.begin(); iter != filesOLD.end(); iter++)
    {
        auto result = std::find(flist.begin(), flist.end(), convert_string<char_t>(iter->fileName));

        if (result == flist.end()) // file not found, copy its data into new archive
            CopyFileData(&fin, &fout, *iter);
        else
            BypassFile(&fin, *iter);
    }

    if (fin.fail())
    {
        fin.close();
		std::filesystem::remove(newArchiveFile);
		throw file_error("Error reading archive file '" + wtos(ArchiveFileName) + "'.");
	}

	if (fout.fail())
	{
		fout.close();
		std::filesystem::remove(newArchiveFile);
		throw file_error("Error writing archive file '" + wtos(newArchiveFile) + "'.");
	}

	fin.close();
	fout.close();

	std::filesystem::remove(ArchiveFileName);
	std::filesystem::rename(newArchiveFile, ArchiveFileName);

    LOG_INFO("Deleted successfully");
}

void Archiver::PrintCompressionStart(const Parameters& params, const string_t& archiveFileName, const vect_string_t& files)
{
	GET_LOGGER();

	logger.InfoFmt("{:<{}}:'{}'", "Creating archive", FIELD_W, /*toOEM*/wtos(archiveFileName));
	logger.InfoFmt("{:<{}}: {}", "Compression coder", FIELD_W, wtos(Parameters::CoderNames[(uint8_t)params.CODER_TYPE]));
	logger.InfoFmt("{:<{}}: {}", "Use model type", FIELD_W, wtos(Parameters::ModelTypeCode[(uint8_t)params.MODEL_TYPE]));
	logger.InfoFmt("{:<{}}: {}", "Block mode", FIELD_W, params.BLOCK_MODE ? "YES" : "NO");
	if (params.BLOCK_MODE)
		logger.InfoFmt("{:<{}}: {:L} bytes", "Block size", FIELD_W, params.BLOCK_SIZE);
	logger.InfoFmt("{:<{}}: {}", "Threads count", FIELD_W, params.THREADS);
	logger.InfoFmt("{:<{}}: {}", "Verbose", FIELD_W, params.VERBOSE ? "true" : "false");
	logger.InfoFmt("Adding {} file{} to archive.", files.size(), files.size() > 1 ? "s" : "");
	logger.Info(std::string(FILL_LENGTH, FILL_SYM));
}

void Archiver::PrintUncompressionStart(const FileRecord& fr, const Parameters& params)
{
	GET_LOGGER();

	logger.InfoFmt("{:<{}}: {}", "Extracting to", FIELD_W, wtos(params.OUTPUT_DIR));
	logger.InfoFmt("{:<{}}: {}", "Compression coder", FIELD_W, wtos(Parameters::CoderNames[fr.alg]));
	logger.InfoFmt("{:<{}}: {}", "Model type", FIELD_W, wtos(Parameters::ModelTypeCode[fr.modelOrder]));
	logger.InfoFmt("{:<{}}: {}", "Block mode", FIELD_W, fr.blockCount > 0 ? "YES" : "NO");
	logger.InfoFmt("{:<{}}: {:L} bytes", "Block size", FIELD_W, fr.blockSize);
	logger.InfoFmt("{:<{}}: {}", "Threads count", FIELD_W, params.THREADS);
	logger.InfoFmt("{:<{}}: {}", "Verbose", FIELD_W, params.VERBOSE ? "true" : "false");
	logger.InfoFmt("{:<{}}:'{}'", "Extracting file", FIELD_W, wtos(fr.fileName)); //toOEM(fr.fileName));
	//logger.Info(std::string(FILL_LENGTH, FILL_SYM));
}

void Archiver::PrintFileCompressionDone(const FileRecord& fr)
{
	GET_LOGGER();

	logger.InfoFmt("{:<{}}: {}", "Compression status", FIELD_W, "Successfull");
	logger.InfoFmt("{:<{}}: '{}'", "File name", FIELD_W, wtos(fr.origFilename)); //toOEM(fr.origFilename));
	logger.InfoFmt("{:<{}}: {:L} bytes", "File size", FIELD_W, fr.fileSize);
	logger.InfoFmt("{:<{}}: {:L} bytes", "Compressed size", FIELD_W, fr.compressedSize);
	logger.InfoFmt("{:<{}}: {:5.3f}", "Ratio", FIELD_W, (float)fr.fileSize / (float)fr.compressedSize);
	logger.InfoFmt("{:<{}}: {:L}", "Blocks", FIELD_W, fr.blockCount);
	logger.InfoFmt("{:<{}}: {}", "Spent time", FIELD_W, MillisecToStr(Ticks::GetTick("compressfile")));
	logger.Info(std::string(FILL_LENGTH, FILL_SYM));
}

void Archiver::PrintFileCompressionAborted(const FileRecord& fr)
{
	GET_LOGGER();

	logger.WarnFmt("{:<{}}: {}", "Compression status", FIELD_W, "ABORTED BY USER");
	logger.InfoFmt("{:<{}}: '{}'", "File name", FIELD_W, wtos(fr.origFilename)); //toOEM(fr.origFilename));
	logger.InfoFmt("{:<{}}: {:L} bytes", "File size", FIELD_W, fr.fileSize);
	logger.InfoFmt("{:<{}}: {}", "Spent time", FIELD_W, MillisecToStr(Ticks::GetTick("compressfile")));
	logger.Info(std::string(FILL_LENGTH, FILL_SYM));
}
