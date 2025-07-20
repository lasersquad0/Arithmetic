#pragma once

#include <fstream>
#include <stack>

#include "CommonFunctions.h"
#include "BasicModel.h"
#include "Callback.h"
#include "ArchiveHeader.h"  // for vector_string_t
#include "thread_pool.h"
#include "libsais64.h"
#include "MTF.h"

struct BWTTask : public MT::Task 
{
	inline static uint32_t TaskIDSeq = 0;
	uint32_t BlockSize; 
	uint32_t ReadSize; // number of bytes actually read into Buf, may be less than blockSize
	uint8_t* Buf;
	uint8_t* BWTBuf;
	int64_t* TempBuf; // needed by libsais64_bwt() call
	int64_t LineNum = -1;
	MTF mtf;

	BWTTask(uint32_t blockSize) : Task("BWTTask#" + std::to_string(getNextID()))
	{
		BlockSize = blockSize;
		Buf     = new uint8_t[BlockSize];
		BWTBuf  = new uint8_t[BlockSize];
		TempBuf = new int64_t[BlockSize + 1]; //TODO too large array, consider using 32bit version of libsais
	};
	
	BWTTask(const BWTTask& a) = delete; // copy constructor

	/*BWTTask(const BWTTask& a) : Task(a) // copy constructor
	{
		BlockSize = a.BlockSize;
		ReadSize = a.ReadSize;
		Buf = a.Buf;
		BWTBuf = a.BWTBuf;
		TempBuf = a.TempBuf;
		LineNum = a.LineNum;
		mtf = a.mtf;
	}*/

	BWTTask& operator=(const BWTTask& a) = delete;
	
	~BWTTask()
	{
		delete[] Buf;
		delete[] BWTBuf;
		delete[] TempBuf;
	}

	uint32_t getNextID() const { return TaskIDSeq++; }

	void one_thread_method() override 
	{
		GET_LOGGER();
		logger.DebugFmt("{} task has started", description.c_str());
		LineNum = libsais64_bwt(Buf, BWTBuf, TempBuf, ReadSize, 0, nullptr);

		if (LineNum < 0)
		{
			LOG_ERROR("[BWTTask] libsais64_bwt finished with error."); //TODO shall we exit in case of lineNum<0 ?
		}

		mtf.Encode(BWTBuf, Buf, ReadSize);

		logger.DebugFmt("{} task has finished", description.c_str());
	}
};


/*
class BWTTaskPool
{
	static uint32_t BlockSize;
	static std::stack<BWTTask> pool;

	BWTTaskPool(uint32_t capacity, uint32_t blockSize)
	{
		BlockSize = blockSize;

		// create initial BWTTasks
		for (size_t i = 0; i < capacity; i++)
		{
			pool.emplace(blockSize); //TODO may be std::to_string(i) is too slow, replace by int value instead of string?
		}
	}

	void Init(uint32_t size, uint32_t blockSize)
	{
		BlockSize = blockSize;

		// create initial BWTTasks
		for (size_t i = 0; i < size; i++)
		{
			pool.emplace(blockSize); //TODO may be std::to_string(i) is too slow, replace by int value instead of string?
		}
	}

	void push(BWTTask& task)
	{
		pool.push(task);
	}

	BWTTask pop()
	{
		if (pool.empty())
		{
			return BWTTask(BlockSize);
		}
		else
		{
			auto& tmpTask = pool.top();
			pool.pop();
			return tmpTask;
		}
	}
};
*/

class Archiver
{
private:
	// takes effect on how many tasks can be added into the thread pool
	const uint32_t MAX_TASKS_MEMORY_USAGE = 2'000'000'000; // 1G. Limit memory usage by all BWTTasks
	// pool will not contain more than MAX_TASKS tasks.
	uint32_t MAX_TASKS;// = std::max(MAX_TASKS_MEMORY_USAGE / (2 * blockSize + 8 * blockSize), 2u); // buf + bwtbuf + tmpbuf

	CallbackManager cbmanager;

	bool LoadBlocksToBWTTasks(std::ifstream* fin, MT::ThreadPool* ppool, uint32_t blockSize) const;

	void SaveBlock(std::ostringstream& fblock, std::ofstream* fout, uint32_t uBlockSize, int32_t lineNum);
	void LoadBlock(std::istringstream& fb, std::ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum);

	/** Return values for next 4 methods:
	* 0 - user aborted the process. need to stop/abort current operation
	* 1 and any value except 0 - everything was ok, move on to the next file
	*/
	int CompressFile(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
	int CompressFileBlock(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
	int CompressFileBlockInThread(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model, const Parameters& params);
	int UncompressFile(std::ifstream* fin, std::ofstream* fout, FileRecord& fr, IModel* model);
	int UncompressFileBlock(std::ifstream* fin, std::ofstream* fout, FileRecord& fr, IModel* model);

	void PrintCompressionStart(const Parameters& params);
	void PrintUncompressionStart(const FileRecord& fr, const Parameters& params);
	void PrintFileCompressionDone(const FileRecord& fr);
	void PrintFileCompressionAborted(const FileRecord& fr);

	bool BypassFile(std::ifstream* fin, const FileRecord& fr);
	bool CopyFileData(std::ifstream* fin, std::ofstream* fout, const FileRecord& fr);

public:
	void AddCallback(ICallback* cb);
	void RemoveCallback(ICallback* cb);

	int CompressFiles(string_t ArchiveFileName, const vect_string_t& files, const Parameters& params); // ArchiveFileName intentionally passed "by value" here 
	int CompressFile(string_t ArchiveFileName, const string_t& FileName, const Parameters& params); // ArchiveFileName intentionally passed "by value" here

	// Extracts (uncompresses) ALL files from archive into params.OUTPUT_DIR directory
	int UncompressFiles(const string_t& ArchiveFileName, const Parameters& params);

	int ExtractFiles(const string_t& ArchiveFileName, const vect_string_t& FilesToExtract, const Parameters& params, bool justTest = false);
	int ExtractFiles(const string_t& ArchiveFileName, const vect_string_t& FilesToExtract, const string_t& ExtractDir, bool justTest = false);
	int ExtractFile(const string_t& ArchiveFileName, const string_t& FileToExtract, const Parameters& params, bool justTest = false);
	int ExtractFile(const string_t& ArchiveFileName, const string_t& FileToExtract, const string_t& ExtractDir, bool justTest = false);

	void RemoveFiles(const string_t& ArchiveFileName, const vect_string_t& flist);
	void RemoveFile(const string_t& ArchiveFileName, const string_t& FileToDelete);
};




