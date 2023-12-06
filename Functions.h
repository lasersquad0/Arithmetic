#pragma once

#include <string>
#include <iostream>

struct MyGroupSeparator : std::numpunct<char>
{
	char do_thousands_sep() const override { return ' '; } // ����������� �����
	std::string do_grouping() const override { return "\3"; } // ����������� �� 3
};


// converts native datetime value into AString
std::string DateTimeToStr(time_t& t);

void SaveToFile(std::string fileName, char* buf, unsigned int len);
void LoadFromFile(std::string fileName, char* buf, unsigned int len);
void SaveTo(std::string fileName, uint8_t* buf, int len);

void SetImbue(std::ostream& stream);

template<typename IntType>
std::string toStringSep(IntType v) // assumes that cout.imbue already called with all required settings
{
	std::stringstream ss;
	ss.imbue(std::locale(ss.getloc(), new MyGroupSeparator()));

	//SetImbue(&ss);
	ss << v;  // printing to string stream with formating
	return ss.str();
}

