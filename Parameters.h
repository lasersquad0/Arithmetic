#pragma once

#include <string>
#include <log4cpp/Category.hh>


enum class ComprAlgs { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, AARITHMETIC, AARITHMETIC32, AARITHMETIC64, ABITARITHMETIC };


class Parameters
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
	static inline uint8_t MODEL_ORDER = 3;
	static inline uint8_t COMPRESSION_ALG = (uint8_t)ComprAlgs::AARITHMETIC;

	static uint32_t parseNumber(std::string s);
	static uint8_t parseModelOrder(std::string s);

};

