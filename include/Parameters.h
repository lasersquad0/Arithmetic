#pragma once

#include <string>
#include "CommonFunctions.h"


#ifdef EXPORT_FOR_DELPHI
#include "ParametersInterface.hpp"

//typedef TCoderType CoderType;
//typedef TModelType ModelType;

class __declspec(delphiclass) DelphiParameters : public TParametersInterface
{
private:
	uint32_t FTHREADS = 1;
	uint64_t FBLOCK_SIZE = 1 << 16;
	UnicodeString FOUTPUT_DIR = ".\\";
	bool FBLOCK_MODE = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	bool FVERBOSE = false;
	TModelType FMODEL_TYPE = TModelType::O2;
	TCoderType FCODER_TYPE = TCoderType::AARITHMETIC;
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
	uint64_t __fastcall GetBlockSize() override { return FBLOCK_SIZE; }
	void __fastcall SetBlockSize(uint64_t bsize) override {FBLOCK_SIZE = bsize; }
	UnicodeString __fastcall GetOutputDir() override { return FOUTPUT_DIR; }
	void __fastcall SetOutputDir(UnicodeString odir) override {FOUTPUT_DIR = odir; }
};
#endif

class Parameters
{
public:
	uint32_t THREADS = 1;
	string_t OUTPUT_DIR = _T(".\\");
	uint64_t BLOCK_SIZE = 1 << 16;
	bool BLOCK_MODE = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	bool VERBOSE = false;
	ModelType MODEL_TYPE = ModelType::O2;
	CoderType CODER_TYPE = CoderType::ARITHMETIC;
	static const inline string_t CoderNames[(uint)CoderType::CodersCount] = { _T("NONE"),_T("HUF"),_T("AHUF"),_T("RLE"),_T("ARI"),_T("ARI32"),_T("ARI64"),_T("BITARI"), _T("ARIFPAQ") };
	static const inline bool IsCoder64bit[(uint)CoderType::CodersCount]   = {   false,      true,      true,     true,     false,      false,      true,        true,         true};
	static const inline string_t ModelTypeCode[(uint)ModelType::ModelsCount]={ _T("UNKNOWN"),_T("O0"),_T("O1"),_T("O2"),_T("O3"),_T("O0FIX"),_T("O0SORT"),_T("O0PAIR"),_T("O3MIX"),_T("O1FPAQ") };


	uint32_t GetThreads() const { return THREADS; }
	void SetThreads(uint32_t thrds) { THREADS = thrds; }
	ModelType GetModelType() const { return MODEL_TYPE; }
	void SetModelType(ModelType mtype) { MODEL_TYPE = mtype; }
	CoderType GetCoderType() const { return CODER_TYPE; }
	void SetCoderType(CoderType ctype) { CODER_TYPE = ctype; }
	bool GetVerbose() const { return VERBOSE; }
	void SetVerbose(bool vrbs) { VERBOSE = vrbs; }
	bool GetBlockMode() const { return BLOCK_MODE; }
	void SetBlockMode(bool bmode) { BLOCK_MODE = bmode; }
	uint64_t GetBlockSize() const { return BLOCK_SIZE; } 
	void SetBlockSize(uint64_t bsize) { BLOCK_SIZE = bsize; }

#ifdef EXPORT_FOR_DELPHI
 	Parameters (){} // explicit default contructor needed because it is removed by compiler when constructor from TParametersInterface exists

	Parameters(TParametersInterface* other) // initialization from Delphi class
	{
		THREADS =  other->THREADS;
		BLOCK_MODE = other->BLOCK_MODE;
		BLOCK_SIZE = other->BLOCK_SIZE;
		VERBOSE = other->VERBOSE;
		MODEL_TYPE = (ModelType)other->MODEL_TYPE;
		CODER_TYPE = (CoderType)other->CODER_TYPE;
		OUTPUT_DIR = other->OUTPUT_DIR.w_str();
	}

#endif
};

namespace Global
{
    // this name used in PrintUsage function as for now
	inline string_t APP_NAME = _T("SOME APP"); // initialized by argv[0] in function main
	const inline std::string LOGGER_NAME = "Arithmetic";
	const inline std::string LOGFILE_NAME = "ArithmeticLog.log";
	const inline std::string LFGFILE_NAME = "ArithmeticLog.lfg";
	inline LogEngine::Logger& GetLogger()
	{
		return LogEngine::GetLogger(LOGGER_NAME);
	}

	struct MyGroupSeparator : std::numpunct<char>
	{
		char do_thousands_sep() const override { return '\''; } // thousands separator
		std::string do_grouping() const override { return "\3"; }   // group by 3
	};

	// used in std::cout and in std::format
	inline const std::locale& GetLoc()
	{
		static std::locale LLocale(std::cout.getloc(), new MyGroupSeparator());
		return LLocale;
	}
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
