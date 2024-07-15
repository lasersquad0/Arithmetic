
#include <ctime>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <locale>
#if defined (__BORLANDC__)
#include <share.h>
#include <sys/stat.h>
#endif
#include "Parameters.h"
#include "CommonFunctions.h"

using namespace std;


void SetImbue(ostream& stream)
{
	stream.imbue(locale(stream.getloc(), new MyGroupSeparator()));
}

char mytoupper(int c) // to eliminate compile warning "warning C4244: '=': conversion from 'int' to 'char', possible loss of data"
{
	return (char)toupper(c);
}

void trimAndUpper(string& str) // TODO what if we have \t in the beginning (end) of string?
{
	// remove any leading and traling spaces, just in case.
	size_t strBegin = str.find_first_not_of(' ');
	size_t strEnd = str.find_last_not_of(' ');
	str.erase(strEnd + 1, str.size() - strEnd);
	str.erase(0, strBegin);

	// to uppercase
	transform(str.begin(), str.end(), str.begin(), ::mytoupper);
}

uint32_t ParseBlockSize(string s)
{
	int factor = 1;
	transform(s.begin(), s.end(), s.begin(), ::mytoupper); // to uppercase

	if (s.find_last_of('K') != string::npos)
		//if (s.lastIndexOf("K") > 0)
	{
		s = s.substr(0, s.length() - 1);
		factor = 1024;
	}
	else if (s.find_last_of('M') != string::npos)
	{
		s = s.substr(0, s.length() - 1);
		factor = 1024 * 1024;
	}

	return stoi(s) * factor;
}

ModelType ParseModelType(string mt)
{
	trimAndUpper(mt);

	size_t len = end(Parameters::ModelTypeCode) - begin(Parameters::ModelTypeCode);
	for (size_t i = 0; i < len; i++)
	{
		if (Parameters::ModelTypeCode[i] == mt)
			return (ModelType)i;
	}

	throw invalid_argument("Specified model type is not recognised.");

}

CoderType ParseCoderType(string ct)
{
	trimAndUpper(ct);

	size_t len = end(Parameters::CoderNames) - begin(Parameters::CoderNames);
	for (size_t i = 0; i < len; i++)
	{
		if (Parameters::CoderNames[i] == ct)
			return (CoderType)i;
	}

	throw invalid_argument("Specified Coder type is not recognised.");

}

// converts native datetime value into string
string DateTimeToString(time_t t)
{
	struct tm ttm;
	localtime_s(&ttm, &t);
	char ss[50];
	strftime(ss, 50, "%F %T", &ttm);

	return ss;
}

/*
void SaveToFile(string fileName, char* buf, unsigned int len)
{
	string dir = "ttt\\";
	_mkdir(dir.c_str());

	FILE* f;
	dir.append(fileName);
	errno_t err = fopen_s(&f, dir.c_str(), "w");
	if (err != 0) 
	{
		cout << " ***** cannot open file for writing *****" << endl;
		return;
	}

	_setmode(_fileno(f), _O_BINARY);
	size_t wrtn = fwrite(buf, sizeof(char), len, f);
	assert(wrtn == len);

	fflush(f);
	fclose(f);
}

void SaveTo(string fileName, uint8_t* buf, int len)
{
	int hf = 0;
	errno_t res = 0;
	res = _sopen_s(&hf, fileName.c_str(), O_WRONLY | O_CREAT | O_BINARY, SH_DENYWR, S_IREAD | S_IWRITE);
	int wrtn = _write(hf, buf, len);
	assert(wrtn == len);

	if (wrtn == -1)
	{
		cout << "Cannot write to file '" + fileName + "'! May be disk full?" << endl;
		//throw exception(s.c_str());
	}

	_close(hf);

}

void LoadFromFile(string fileName, char* buf, unsigned int len)
{
	FILE* f;
	errno_t err = fopen_s(&f, fileName.c_str(), "r");

	_setmode(_fileno(f), _O_BINARY);
	if (err != 0)
		cout << " ***** cannot open file for writing *****" << endl;
	size_t wrtn = fread(buf, sizeof(char), len, f);
	fclose(f);
}
  */


