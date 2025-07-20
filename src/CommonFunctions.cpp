
#include <ctime>
#include <iostream>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <locale>

#if defined (__BORLANDC__)
#include <share.h>
#include <sys/stat.h>
#endif
#include "strsafe.h"
#include <direct.h>
#include <io.h>
#include <fcntl.h>

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

// intentionally left std::string
void TrimAndUpper(string_t& str) // TODO what if we have \t in the beginning (end) of string?
{
	// remove any leading and traling spaces, just in case.
	size_t strBegin = str.find_first_not_of(' ');
	size_t strEnd = str.find_last_not_of(' ');
	str.erase(strEnd + 1, str.size() - strEnd);
	str.erase(0, strBegin);

	// to uppercase
	transform(str.begin(), str.end(), str.begin(), ::mytoupper);
}

string_t GetErrorMessageText(ulong lastError, const string_t& errorPlace)
{
	const uint32_t BUF_SIZE = 2048; // should be enough for all error messages
	string_t buf, buf2;
	buf.resize(BUF_SIZE);
	buf2.resize(BUF_SIZE);

	BOOL_CHECK(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)buf.data(), BUF_SIZE, nullptr));

	//return std::format(_T("%s failed with error code %d as follows:\n%s"), errorPlace, lastError, buf);
	HR_CHECK(StringCchPrintf(buf2.data(), buf2.size(), TEXT("%s failed with error code %d as follows:\n%s"), errorPlace.c_str(), lastError, buf.data()));
	return buf2;
}

//TODO replace by function GetErrorMessageText above ??
void PrintWindowsErrorMessage(const TCHAR* lpszFunction)
{
	LPVOID lpMsgBuf;
	DWORD err = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	//wcout << (TCHAR*)lpMsgBuf << endl;

	LPVOID lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

	StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), lpszFunction, err, lpMsgBuf);

	auto& logger = Global::GetLogger();
	LOG_ERROR(convert_string<char>((LPCTSTR)lpDisplayBuf));
	//wcout << (TCHAR*)lpDisplayBuf << endl;

	LocalFree(lpDisplayBuf);
}

uint32_t ParseBlockSize(string_t s) // intentionally by value
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

ModelType ParseModelType(string_t mt) // intentionally by value
{
	TrimAndUpper(mt);

	size_t len = end(Parameters::ModelTypeCode) - begin(Parameters::ModelTypeCode);
	for (size_t i = 0; i < len; i++)
	{
		if (Parameters::ModelTypeCode[i] == mt)
			return (ModelType)i;
	}

	throw invalid_argument("Specified model type is not recognised.");
}

CoderType ParseCoderType(string_t ct)  
{
	TrimAndUpper(ct);

	size_t len = end(Parameters::CoderNames) - begin(Parameters::CoderNames);
	for (size_t i = 0; i < len; i++)
	{
		if (Parameters::CoderNames[i] == ct)
			return (CoderType)i;
	}

	throw invalid_argument("Specified Coder type is not recognised.");
}

//TODO implementation of this function is not finished.
void CheckParametersValidity(Parameters& params)
{
	struct Pair
	{
		CoderType coder;
		ModelType model;
	};

	const Pair InvalidPairs[] = { {CoderType::HUFFMAN, ModelType::O1}, {CoderType::FPAQARITHMETIC, ModelType::O1FPAQ} };

	if (params.BLOCK_MODE && params.MODEL_TYPE == ModelType::O0FIX)
		throw std::invalid_argument("You have Block mode turned on. Model 'O0FIX' is not compatible with Block mode. Either select stream (non-block) mode or choo another model");
}

bool SaveLogConfigFile(int resID, const std::string& fileName)
{
	HRSRC resInfo = FindResource(nullptr, MAKEINTRESOURCE(resID/*IDR_RT_RCDATA1*/), L"RT_RCDATA");

	if (resInfo == nullptr)
	{
		PrintWindowsErrorMessage(_T("FindResource"));
		return false;
	}

	HGLOBAL resBytes = LoadResource(nullptr, resInfo);

	if (resBytes == nullptr)
	{
		PrintWindowsErrorMessage(_T("LoadResource"));
		return false;
	}

	LPVOID lockBytes = LockResource(resBytes);

	if (lockBytes == nullptr)
	{
		PrintWindowsErrorMessage(_T("LockResource"));
		return false;
	}

	DWORD size = SizeofResource(nullptr, resInfo);

	ofstream out;
	out.open(fileName, ios::binary);
	out.write((char*)lockBytes, size);
	out.close();

	return true;
}

// converts native datetime value into string_t
string_t DateTimeToString(time_t t)
{
	struct tm ttm;
	localtime_s(&ttm, &t);
	char_t ss[100];

#if defined(UNICODE) || defined(_UNICODE)
	wcsftime(ss, 100, _T("%F %T"), &ttm);
#else
	strftime(ss, 100, _T("%F %T"), &ttm);
#endif

	return ss;
}

// converts native datetime value into std::string
std::string DateTimeToStringA(time_t t)
{
	struct tm ttm;
	localtime_s(&ttm, &t);
	char ss[100];

	strftime(ss, 100, "%F %T", &ttm);
	return ss;
}

std::string toOEM(const string_t& str)
{
	std::string tmp;
	tmp.resize(str.size());
	CharToOemBuffW(str.data(), tmp.data(), (DWORD)str.size());
	return tmp;
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


