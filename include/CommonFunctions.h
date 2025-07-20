#pragma once

#include <tchar.h>
#include <string>
#include <filesystem>
//#include <iostream>
#include <vector>
#include <sstream>

// this is to remove defines min, max in windows headers because they conflict with std::min std::max 
#define NOMINMAX
#include <windows.h>
#include "ARIExceptions.h"

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

#define GET_LOGGER() auto& logger = Global::GetLogger()
#define LOG_INFO(_) logger.Info(_)
#define LOG_INFO1(_1,_2) logger.InfoFmt(_1, _2)
#define LOG_INFO2(_1,_2,_3) logger.InfoFmt(_1, _2, _3)
#define LOG_WARN(_) logger.Warn(_)
#define LOG_ERROR(_) logger.Error(_)
#define LOG_DEBUG(_) logger.Debug(_)
#define LOG_DEBUG1(_1,_2) logger.DebugFmt(_1, _2)
#define LOG_DEBUG2(_1,_2,_3) logger.DebugFmt(_1, _2, _3)

enum class CoderType { NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, BITARITHMETIC, FPAQARITHMETIC };
enum class ModelType { UNKNOWN, O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ };

//TODO think how can we report here error in general way for non-console applications
#define LOG_CHECK_ERROR(_msg) std::cout << (_msg)
//#define LOG_CHECK_ERROR(_msg) logger.Error(_msg)
#define MAKE_LOG_TSTR(_msg) string_t(_T(__FILE__)).append(_T(" : ")).append(_msg)
#define MAKE_LOG_STR(_msg) std::string(__FILE__).append(" : ").append(__func__).append(" : ").append(std::to_string(__LINE__)).append(" - ").append(_msg)
#define HR_CHECK(_hr) {if(FAILED(_hr)) LOG_CHECK_ERROR( MAKE_LOG_STR(std::system_category().message(_hr)));}
#define BOOL_CHECK(_res) {if(!(_res)) LOG_CHECK_ERROR( MAKE_LOG_STR(std::system_category().message(GetLastError())));}


struct MyGroupSeparator : std::numpunct<char_t>
{
    char_t do_thousands_sep() const override { return ' '; } // thousands separator
	std::string do_grouping() const override { return "\3"; } // groupping by 3
};

void SetImbue(std::ostream& stream);

uint32_t ParseBlockSize(string_t s);
ModelType ParseModelType(string_t s);
CoderType ParseCoderType(string_t ct);
void PrintWindowsErrorMessage(const TCHAR* lpszFunction);
bool SaveLogConfigFile(int resID, const std::string& fileName);

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


// short version via filesystem::path
/*inline std::string wtos(const std::filesystem::path& str)
{
    return str.string();
}

// short version via filesystem::path
inline std::wstring stow(const std::filesystem::path& str)
{
    return str.wstring();
}*/

template<class WSTRING>
std::string wtos(const WSTRING& wstr)
{
    // make sure that STRING is one of instantiations of std::wstring
    static_assert(std::is_base_of<std::basic_string<typename WSTRING::value_type, typename WSTRING::traits_type>, WSTRING>::value);
    static_assert(std::is_same_v<typename WSTRING::value_type, wchar_t>);

    if (wstr.size() == 0) return "";

    int len = WideCharToMultiByte(CP_UTF8, 0 /*WC_NO_BEST_FIT_CHARS*/, wstr.data(), (int)wstr.length(), nullptr, 0, nullptr, nullptr);

    if (len == 0)
		throw convert_error(std::string("Error in WideCharToMultiByte 1. Error code: ") + std::to_string(GetLastError()));

    std::string dest;
    dest.resize(len);
    int err = WideCharToMultiByte(CP_UTF8, 0 /*WC_NO_BEST_FIT_CHARS*/, wstr.data(), (int)wstr.size(), dest.data(), len, nullptr, nullptr);

	if (!err)
		throw convert_error(std::string("Error in WideCharToMultiByte 2. Error code: ") + std::to_string(GetLastError()));

    return dest;
}

template<class STRING>
std::wstring stow(const STRING& str)
{
    // make sure that STRING is one of instantiations of std::wstring
    static_assert(std::is_base_of<std::basic_string<typename STRING::value_type, typename STRING::traits_type>, STRING>::value);
    static_assert(std::is_same_v<typename STRING::value_type, char>);

	if (str.size() == 0) return L"";

    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0); // Get the required buffer size
    if (bufferSize == 0)
		throw convert_error(std::string("Error in WideCharToMultiByte 1. Error code: ") + std::to_string(GetLastError()));

	std::wstring wstr(bufferSize, 0);

	int result = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr.data(), bufferSize);
	if (result == 0)
        throw convert_error(std::string("Error in WideCharToMultiByte 2. Error code: ") + std::to_string(GetLastError()));

    wstr.resize(bufferSize - 1);
    return wstr;
}

