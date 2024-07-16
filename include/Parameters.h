#pragma once

#include <string>
#ifdef LOG4CPP
#include <log4cpp/Category.hh>
#else
#include "LogEngine.h"
#endif

#if defined (EXPORT_FOR_DELPHI)
#include "ParametersInterface.hpp"

typedef TCoderType CoderType;
typedef TModelType ModelType;
//using ModelType = TModelType;

class __declspec(delphiclass) Parameters : public TParametersInterface
{
private:
	uint32_t FTHREADS = 1;
	UnicodeString FOUTPUT_DIR = ".\\";
	uint32_t FBLOCK_SIZE = 1 << 16;
	bool FBLOCK_MODE = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	bool FVERBOSE = false;
	TModelType FMODEL_TYPE = TModelType::O2;
	TCoderType FCODER_TYPE = TCoderType::AARITHMETIC;
public:
	std::string OUTPUT_DIR = ".\\";
	static const inline std::string CoderNames[] = { "NONE", "HUF", "AHUF", "RLE", "ARI", "ARI32", "ARI64", "AARI", "AARI32", "AARI64", "BITARI" };
	static const inline std::string ModelTypeCode[] = { "UNKNOWN", "O0", "O1", "O2", "O3", "MIXO3", "FO1", "BITO1" };
private:
	uint32_t __fastcall GetThreads() override { return FTHREADS; }
	void __fastcall SetThreads(uint32_t thrds) override {FTHREADS = thrds; }
	TModelType __fastcall GetModelType() override { return FMODEL_TYPE; }
	void __fastcall SetModelType(TModelType mtype) override {FMODEL_TYPE = mtype; }
	TCoderType __fastcall GetCoderType() override { return FCODER_TYPE; }
	void __fastcall SetCoderType(TCoderType ctype) override {FCODER_TYPE = ctype; }
	bool __fastcall GetVerbose() override { return FVERBOSE; }
	void __fastcall SetVerbose(bool vrbs) override {FVERBOSE = vrbs; }
	bool __fastcall GetBlockMode() override { return FBLOCK_MODE; }
	void __fastcall SetBlockMode(bool bmode) override {FBLOCK_MODE = bmode; }
	uint32_t __fastcall GetBlockSize() override { return FBLOCK_SIZE; }
	void __fastcall SetBlockSize(uint32_t bsize) override {FBLOCK_SIZE = bsize; }
	UnicodeString __fastcall GetOutputDir() override { return FOUTPUT_DIR; }
	void __fastcall SetOutputDir(UnicodeString odir) override {FOUTPUT_DIR = odir; }
};

#else

enum class CoderType { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, AARITHMETIC, AARITHMETIC32, AARITHMETIC64, ABITARITHMETIC };
enum class ModelType { UNKNOWN, O0, O1, O2, O3, MIXO3, FO1, BITO1 };

class Parameters
{
public:
	uint32_t THREADS = 1;
	std::string OUTPUT_DIR = ".\\";
	uint32_t BLOCK_SIZE = 1 << 16;
	bool BLOCK_MODE = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	bool VERBOSE = false;
	ModelType MODEL_TYPE = ModelType::O2;
	CoderType CODER_TYPE = CoderType::AARITHMETIC;
	static const inline std::string CoderNames[] = { "NONE", "HUF", "AHUF", "RLE", "ARI", "ARI32", "ARI64", "AARI", "AARI32", "AARI64", "BITARI" };
	static const inline std::string ModelTypeCode[] = { "UNKNOWN", "O0", "O1", "O2", "O3", "MIXO3", "FO1", "BITO1" };

	uint32_t GetThreads() { return THREADS; }
	void SetThreads(uint32_t thrds) { THREADS = thrds; }
	ModelType GetModelType() { return MODEL_TYPE; }
	void SetModelType(ModelType mtype) { MODEL_TYPE = mtype; }
	CoderType GetCoderType() { return CODER_TYPE; }
	void SetCoderType(CoderType ctype) { CODER_TYPE = ctype; }
	bool GetVerbose() { return VERBOSE; }
	void SetVerbose(bool vrbs) { VERBOSE = vrbs; }
	bool GetBlockMode() { return BLOCK_MODE; }
	void SetBlockMode(bool bmode) { BLOCK_MODE = bmode; }
	uint32_t GetBlockSize() { return BLOCK_SIZE; }
	void SetBlockSize(uint32_t bsize) { BLOCK_SIZE = bsize; }
};

#endif

class Global
{
public:
	static inline std::string APP_NAME = "SOME APP"; // initialized by argv[0] in function main
#ifdef LOG4CPP
	static log4cpp::Category& GetLogger()
	{
		return log4cpp::Category::getInstance("Arithmetic");
	}
#else
	static LogEngine::Logger& GetLogger()
	{
		return LogEngine::GetLogger("Arithmetic");
	}

#endif
};

/*
class ParametersG
{
public:
	static inline std::string LOGGER_NAME = "Arithmetic";
	static inline uint32_t THREADS = 1;
	static std::string OUTPUT_DIR;
	static inline uint32_t BLOCK_SIZE = 1 << 16;
	static inline bool BLOCK_MODE = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	static inline std::string APP_NAME = "ttttt"; // initialized by argv[0] in function main
	static log4cpp::Category& logger;
	static inline bool VERBOSE = false;
	static inline ModelType MODEL_TYPE = ModelType::O3;
	static inline CoderType CODER_TYPE = CoderType::AARITHMETIC;
	static const inline std::string CoderNames[] = { "NONE", "HUF", "AHUF", "RLE", "ARI", "ARI32", "ARI64", "AARI", "AARI32", "AARI64", "BITARI" };
	static const inline std::string ModelTypeCode[] = { "UNKNOWN", "O1", "O2", "O3", "O4", "FO1", "BITO1" };
};
*/
