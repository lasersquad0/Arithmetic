#pragma once

#include <string>
#ifdef LOG4CPP
#include <log4cpp/Category.hh>
#else
#include "LogEngine.h"
#endif
#include "ParametersInterface.hpp"

enum class CoderType { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, AARITHMETIC, AARITHMETIC32, AARITHMETIC64, ABITARITHMETIC };

enum class ModelType { UNKNOWN, O0, O1, O2, O3, MIXO3, FO1, BITO1 };

class __declspec(delphiclass) Parameters : public TParametersInterface
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
private:
	uint32_t __fastcall GetThreads() override { return THREADS; }
	void __fastcall SetThreads(uint32_t thrds) override {THREADS = thrds; }
};

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
