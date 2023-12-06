#pragma once

#include <fstream>
#include <vector>
#include "ArchiveHeader.h"

//using namespace std;


class RangeCompressor
{
public:
	void CompressFiles(const vector_string_t& files);
	void CompressFile(std::ofstream* fout, FileRecord& fr);
	void CompressFileBlock(std::ofstream* fout, FileRecord& fr);
	void SaveBlock(std::ostringstream& fblock, std::ofstream* fout, uint32_t uBlockSize, int32_t lineNum);
	void LoadBlock(std::istringstream& fb, std::ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum);

	void UncompressFiles(std::ifstream* fin);
	void UncompressFile(std::ifstream* fin, FileRecord& fr);
	void UncompressFileBlock(std::ifstream* fin, FileRecord& fr);

	void PrintCompressionStart();
	void PrintUncompressionStart(FileRecord& fr);
	void PrintFileCompressionDone(FileRecord& fr);

	//void EncodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
	//void DecodeFile(FILE* DecodedFile, FILE* EncodedFile, uint64_t fSize);
};

