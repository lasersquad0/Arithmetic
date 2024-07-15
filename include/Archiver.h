#pragma once

#include <fstream>
#include <vector>
#include "BasicModel.h"
#include "Callback.h"
#include "ArchiveHeader.h"
#include "ArchiverInterface.hpp"


class __declspec(delphiclass) Archiver : public TArchiverInterface
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

	bool BypassFile(std::ifstream* fin, const FileRecord& fr);
	bool CopyFileData(std::ifstream* fin, std::ofstream* fout, const FileRecord& fr);

public:
	void AddCallback(ICallback* cb);
	void RemoveCallback(ICallback* cb);

	void CompressFiles(const vector_string_t& files, std::string ArchiveFileName, const Parameters& params); // ArchiveFileName intentionally passed "by value" here
	void CompressFile(const std::string& FileName, std::string ArchiveFileName, const Parameters& params); // ArchiveFileName intentionally passed "by value" here
	
	void UncompressFiles(std::ifstream* fin, const Parameters& params);
	
	void ExtractFiles(const std::string& ArchiveFile, const vector_string_t& FilesToExtract, const std::string& ExtractDir, const Parameters& params = Parameters());
	void ExtractFile(const std::string& ArchiveFile, const std::string& FileToExtract, const std::string& ExtractDir, const Parameters& params = Parameters());

	void RemoveFile(const std::string& ArchiveFile, const std::string& FileToDelete);
	void RemoveFiles(const std::string& ArchiveFile, const vector_string_t& flist);

		// overrided methods
	void __fastcall CompressFiles(const System::UnicodeString Files, const System::UnicodeString ArchiveFileName/*, Parameters params*/) override;
	void __fastcall CompressFile(const System::UnicodeString FileName, const System::UnicodeString ArchiveFileName/*, Parameters params*/) override;
	void __fastcall ExtractFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToExtract, const System::UnicodeString ExtractDir/*, Parameters params = Parameters()*/) override;
	void __fastcall RemoveFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToDelete) override;


	//void EncodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
	//void DecodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
};

