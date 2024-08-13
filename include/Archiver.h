#pragma once

#include <fstream>
//#include <vector>
#include "BasicModel.h"
#include "Callback.h"
#include "ArchiveHeader.h"  // for vector_string_t

class Archiver
{
private:
	CallbackManager cbmanager;

	void SaveBlock(std::ostringstream& fblock, std::ofstream* fout, uint32_t uBlockSize, int32_t lineNum);
	void LoadBlock(std::istringstream& fb, std::ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum);

	/** Return values for next 4 methods:
	* 0 - user aborted the process. need to stop/abort current operation
	* 1 and any value except 0 - everything was ok, move on to the next file
	*/
	int CompressFile(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
	int CompressFileBlock(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
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




