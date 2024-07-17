#pragma once

#include <string>
#include <iostream>
#include "Parameters.h"


typedef unsigned char uchar;
typedef unsigned int uint;

struct MyGroupSeparator : std::numpunct<char>
{
	char do_thousands_sep() const override { return ' '; } // thousands separator
	std::string do_grouping() const override { return "\3"; } // groupping by 3
};

void SetImbue(std::ostream& stream);

uint32_t ParseBlockSize(std::string s);
ModelType ParseModelType(std::string s);
CoderType ParseCoderType(std::string ct);

// converts native datetime value into std::string
std::string DateTimeToString(time_t t);

void SaveToFile(std::string fileName, char* buf, unsigned int len);
void LoadFromFile(std::string fileName, char* buf, unsigned int len);
void SaveTo(std::string fileName, uint8_t* buf, int len);

template<typename IntType>
std::string toStringSep(IntType v) 
{
	std::stringstream ss;
	ss.imbue(std::locale(ss.getloc(), new MyGroupSeparator()));

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

