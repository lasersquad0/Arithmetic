#pragma once

//#include <tchar.h>
#include <strsafe.h>
#include <string>
#include <filesystem>
#include <vector>
#include <sstream>

// this is to remove defines min, max in windows headers because they conflict with std::min std::max 
#define NOMINMAX

#include <windows.h>
#include "utils/include/string_utils.h"
#include "LogEngine2/LogEngine.h" // for uchar, uint, ulong definitions
#include "ARIExceptions.h"


#define GET_LOGGER() auto& logger = Global::GetLogger()
#define LOG_INFO(_) logger.Info(_)
#define LOG_INFO1(_1,_2) logger.InfoFmt(_1, _2)
#define LOG_INFO2(_1,_2,_3) logger.InfoFmt(_1, _2, _3)
#define LOG_WARN(_) logger.Warn(_)
#define LOG_ERROR(_) logger.Error(_)
#define LOG_DEBUG(_) logger.Debug(_)
#define LOG_DEBUG1(_1,_2) logger.DebugFmt(_1, _2)
#define LOG_DEBUG2(_1,_2,_3) logger.DebugFmt(_1, _2, _3)

enum class CoderType: uint { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, BITARITHMETIC, FPAQARITHMETIC, CodersCount };
enum class ModelType: uint { UNKNOWN, O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ, ModelsCount };

//TODO think how can we report here error in general way for non-console applications
#define LOG_CHECK_ERROR(_msg) std::cout << (_msg)
//#define LOG_CHECK_ERROR(_msg) logger.Error(_msg)
#define MAKE_LOG_TSTR(_msg) string_t(U(__FILE__)).append(U(" : ")).append(_msg)
#define MAKE_LOG_STR(_msg) std::string(__FILE__).append(" : ").append(__func__).append(" : ").append(std::to_string(__LINE__)).append(" - ").append(_msg)
#define HR_CHECK(_hr) {if(FAILED(_hr)) LOG_CHECK_ERROR( MAKE_LOG_STR(std::system_category().message(_hr)));}
#define BOOL_CHECK(_res) {if(!(_res)) LOG_CHECK_ERROR( MAKE_LOG_STR(std::system_category().message(GetLastError())));}

//void CheckParametersValidity(Parameters& params);
uint64_t ParseBlockSize(const string_t& s);
ModelType ParseModelType(string_t s);
CoderType ParseCoderType(string_t ct);
void PrintWindowsErrorMessage(const TCHAR* lpszFunction);
bool SaveLogConfigFile(int resID, const std::string& fileName);

// converts native datetime value into std::string or string_t
string_t DateTimeToString(time_t t);
std::string DateTimeToStringA(time_t t);

//std::string toOEM(const string_t& str);

//void SaveToFile(std::string fileName, char* buf, uint len);
//void LoadFromFile(std::string fileName, char* buf, uint len);
//void SaveTo(std::string fileName, uint8_t* buf, int len);

