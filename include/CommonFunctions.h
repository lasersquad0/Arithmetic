#pragma once

#include <tchar.h>
#include <string>
#include <filesystem>
//#include <iostream>
#include <vector>
#include <sstream>
//#include "Parameters.h"


typedef unsigned char uchar;
typedef unsigned int uint;

#if defined(UNICODE) || defined(_UNICODE)

typedef std::wstring string_t;
typedef std::wstringstream stringstream_t;
typedef std::wstring::value_type char_t;
#define to_string_t std::to_wstring

#else

typedef std::string string_t;
typedef std::stringstream stringstream_t;
typedef std::string::value_type char_t;
#define to_string_t std::to_string

#endif

#ifdef LOG4CPP

#define GET_LOGGER() log4cpp::Category& logger = Global::GetLogger()
#define LOG_INFO(_) logger.info(_)
#define LOG_INFO2(_1,_2) logger.info(_1,_2)
#define LOG_WARN(_) logger.warn(_)
#define LOG_ERROR(_) logger.error(_)

#else // LogEngine

#define GET_LOGGER() LogEngine::Logger& logger = Global::GetLogger()
#define LOG_INFO(_) logger.Info(_)
#define LOG_INFO2(_1,_2) logger.LogFmt(LogEngine::Levels::llInfo, _1, _2)
#define LOG_INFO3(_1,_2,_3) logger.LogFmt(LogEngine::Levels::llInfo, _1, _2, _3)
#define LOG_WARN(_) logger.Warn(_)
#define LOG_ERROR(_) logger.Error(_)
#define LOG_DEBUG(_) logger.Debug(_)
#define LOG_DEBUG2(_1,_2) logger.LogFmt(LogEngine::Levels::llDebug, _1, _2)
#define LOG_DEBUG3(_1,_2,_3) logger.LogFmt(LogEngine::Levels::llDebug, _1, _2, _3)
#endif


enum class CoderType { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, BITARITHMETIC, FPAQARITHMETIC };
enum class ModelType { UNKNOWN, O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ };

struct MyGroupSeparator : std::numpunct<char_t>
{
    char_t do_thousands_sep() const override { return ' '; } // thousands separator
	std::string do_grouping() const override { return "\3"; } // groupping by 3
};

void SetImbue(std::ostream& stream);

uint32_t ParseBlockSize(string_t s);
ModelType ParseModelType(string_t s);
CoderType ParseCoderType(string_t ct);

// converts native datetime value into std::string or string_t
string_t DateTimeToString(time_t t);
std::string DateTimeToStringA(time_t t);

std::string toOEM(const string_t& str);

//void SaveToFile(std::string fileName, char* buf, uint len);
//void LoadFromFile(std::string fileName, char* buf, uint len);
//void SaveTo(std::string fileName, uint8_t* buf, int len);

/// converts any integer type into a string with group separator applied.
/// group separator is defined by MyGroupSeparator class
/// string_t can be either std::string or std::wstring
template<typename IntType>
string_t toStringSep(IntType v)
{
	stringstream_t ss;
	ss.imbue(std::locale(ss.getloc(), new MyGroupSeparator()));

	//SetImbue(&ss);
	ss << v;  // printing to string stream with formating
	return ss.str();
}

/// converts any integer type into a string with group separator applied.
/// group separator is defined by MyGroupSeparator class
template<typename IntType>
std::string toStringSepA(IntType v)
{
    struct GroupSeparatorA : std::numpunct<char>
    {
        char do_thousands_sep() const override { return ' '; } // thousands separator
        std::string do_grouping() const override { return "\3"; } // groupping by 3
    };

    std::stringstream ss;
    ss.imbue(std::locale(ss.getloc(), new GroupSeparatorA()));

    //SetImbue(&ss);
    ss << v;  // printing to string stream with formating
    return ss.str();
}

// split string into array of strings using Delim as delimiter
template<class STRING>
void StringToArray(const STRING& str, std::vector<STRING>& arr, const typename STRING::value_type Delim = '\n')
{
	// make sure that STRING is one of instantiations of std::string
    static_assert(std::is_base_of<std::basic_string<typename STRING::value_type, typename STRING::traits_type>, STRING>::value);

    uint i = 0;
    size_t len = str.length();
    STRING s;
    s.reserve(len);

    while (i < len)
    {
        s.clear();
        while (i < len)
        {
            if (str[i] == Delim)
            {
                i++;
                break;
            }
            s += str[i++];
        }

        if (s.length() > 0)
            arr.push_back(s);
    }
}

// splits string to array of strings using Delim as delimiter
template<class STRING>
void StringToArrayAccum(const STRING& str, std::vector<STRING>& arr, const typename STRING::value_type Delim = '\n')
{
    // make sure that STRING is one of instantiations of std::string
    static_assert(std::is_base_of<std::basic_string<typename STRING::value_type, typename STRING::traits_type>, STRING>::value);

    uint i = 0;
    size_t len = str.length();
    STRING s;
    s.reserve(len);

    while (i < len)
    {
        if (str[i] == Delim)
            if (s.length() > 0) arr.push_back(s);
        s += str[i++];

    }

    if (s.length() > 0)
        arr.push_back(s);
}

template<typename T>
constexpr std::basic_string<T> convert_string(const std::filesystem::path& str)
{
    if constexpr(std::is_same_v<T, char>)
    {
        return str.string();
    }
    else if (std::is_same_v<T, wchar_t>)
    {
        return str.wstring();
    }
    /*else if (std::is_same_v<T, char16_t>) {
        return str.u16string();
    }
    else if (std::is_same_v<T, char32_t>) {
        return str.u32string();
    } */
}

