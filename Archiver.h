#pragma once

#include <fstream>
#include <vector>
#include "BasicModel.h"
#include "Callback.h"
#include "ArchiveHeader.h"

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

	void PrintCompressionStart(Parameters params);
	void PrintUncompressionStart(FileRecord& fr, Parameters params);
	void PrintFileCompressionDone(FileRecord& fr);

	bool BypassFile(std::ifstream* fin, FileRecord& fr);
	bool CopyFileData(std::ifstream* fin, std::ofstream* fout, FileRecord& fr);

public:
	void AddCallback(ICallback* cb);
	void RemoveCallback(ICallback* cb);

	void CompressFiles(const vector_string_t& files, std::string ArchiveFileName, Parameters params);
	void CompressFile(std::string FileName, std::string ArchiveFileName, Parameters params);
	
	void UncompressFiles(std::ifstream* fin, Parameters params);
	
	void ExtractFiles(std::string ArchiveFile, vector_string_t FilesToExtract, std::string ExtractDir, Parameters params = Parameters());
	void ExtractFile(std::string ArchiveFile, std::string FileToExtract, std::string ExtractDir, Parameters params = Parameters());

	void RemoveFile(std::string ArchiveFile, std::string FileToDelete);
	void RemoveFiles(std::string ArchiveFile, vector_string_t& flist);

	
	//void EncodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
	//void DecodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
};

