
#include <ctime>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>
#include <sstream>
#include "functions.h"


// converts native datetime value into AString
std::string DateTimeToStr(time_t& t)
{
	struct tm ttm;
	localtime_s(&ttm, &t);
	char ss[50];
	std::strftime(ss, 50, "%F %T", &ttm);

	return ss;
}

void SetImbue(std::ostream& stream)
{
	stream.imbue(std::locale(stream.getloc(), new MyGroupSeparator()));
}

void SaveToFile(std::string fileName, char* buf, unsigned int len)
{
	std::string dir = "ttt\\";
	_mkdir(dir.c_str());

	FILE* f;
	dir.append(fileName);
	errno_t err = fopen_s(&f, dir.c_str(), "w");
	if (err != 0) 
	{
		std::cout << " ***** cannot open file for writing *****" << std::endl;
		return;
	}

	_setmode(_fileno(f), _O_BINARY);
	size_t wrtn = fwrite(buf, sizeof(char), len, f);
	assert(wrtn == len);

	fflush(f);
	fclose(f);
}

void SaveTo(std::string fileName, uint8_t* buf, int len)
{
	int hf = 0;
	errno_t res = 0;
	res = _sopen_s(&hf, fileName.c_str(), O_WRONLY | O_CREAT | O_BINARY, _SH_DENYWR, _S_IREAD | _S_IWRITE);
	int wrtn = _write(hf, buf, len);
	assert(wrtn == len);

	if (wrtn == -1)
	{
		std::cout << "Cannot write to file '" + fileName + "'! May be disk full?";
		//throw exception(s.c_str());
	}

	_close(hf);

}

void LoadFromFile(std::string fileName, char* buf, unsigned int len)
{
	FILE* f;
	errno_t err = fopen_s(&f, fileName.c_str(), "r");

	_setmode(_fileno(f), _O_BINARY);
	if (err != 0)
		std::cout << " ***** cannot open file for writing *****" << std::endl;
	size_t wrtn = fread(buf, sizeof(char), len, f);
	fclose(f);
}

