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

	void PrintCompressionStart(Parameters& params);
	void PrintUncompressionStart(const FileRecord& fr, Parameters& params);
	void PrintFileCompressionDone(const FileRecord& fr);
	void PrintFileCompressionAborted(const FileRecord& fr);

	bool BypassFile(std::ifstream* fin, const FileRecord& fr);
	bool CopyFileData(std::ifstream* fin, std::ofstream* fout, const FileRecord& fr);

public:
	void AddCallback(ICallback* cb);
	void RemoveCallback(ICallback* cb);

	void CompressFiles(std::string ArchiveFileName, const vector_string_t& files, Parameters& params); // ArchiveFileName intentionally passed "by value" here
 //	void CompressFilesW(const vector_wstring_t& files, std::wstring ArchiveFileName, Parameters& params);

	void CompressFile(std::string ArchiveFileName, const std::string& FileName, Parameters& params); // ArchiveFileName intentionally passed "by value" here
	//void CompressFileW(const std::wstring& FileName, std::wstring ArchiveFileName, Parameters& params);

	// Extracts (uncompresses) ALL files from archive into params.OUTPUT_DIR directory
	void UncompressFiles(const std::string& ArchiveFileName, Parameters& params);

	void ExtractFiles(const std::string& ArchiveFileName, const vector_string_t& FilesToExtract, Parameters& params);
	void ExtractFiles(const std::string& ArchiveFileName, const vector_string_t& FilesToExtract, const std::string& ExtractDir);
	void ExtractFile(const std::string& ArchiveFileName, const std::string& FileToExtract, Parameters& params);
	void ExtractFile(const std::string& ArchiveFileName, const std::string& FileToExtract, const std::string& ExtractDir);

	void RemoveFiles(const std::string& ArchiveFileName, const vector_string_t& flist);
	void RemoveFile(const std::string& ArchiveFileName, const std::string& FileToDelete);
};




