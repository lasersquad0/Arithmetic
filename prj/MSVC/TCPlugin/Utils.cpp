
#include <Windows.h>
#include <time.h>
#include "Utils.h"


// assuming that dest buffer is large enough
void WCHARtoChar(char* dest, wchar_t* src)
{
    //size_t len;
    WideCharToMultiByte(CP_ACP, 0, src, -1, dest, MAX_PATH, nullptr, nullptr);
    //wcstombs_s(&len, dest, MAX_PATH, src, wcslen(src));
    //if (len > 0u) dest[len] = '\0';
}

// assuming that dest buffer is large enough
void CharToWCHAR(wchar_t* dest, const char* src)
{
    // size_t len;
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dest, MAX_PATH);
    // mbstowcs_s(&len, dest, MAX_PATH, src, strlen(src));
    // if (len > 0u) dest[len] = '\0';
}

uint MakeTCTime(time_t tt)
{
    struct tm fttm;
    localtime_s(&fttm, &tt);

    unsigned short date, time;
    time = fttm.tm_hour << 11 | fttm.tm_min << 5 | fttm.tm_sec;
    date = (fttm.tm_year - 80) << 9 | (fttm.tm_mon + 1) << 5 | fttm.tm_mday;
    return 65536 * date + time;
}


// extracts filename from path with filename
char_t* ExtractFileNameW(char_t* path)
{
    size_t i = wcslen(path);
    while (i >= 0)
    {
        if ((path[i] == L'\\') || (path[i] == L'/'))
            break;
        i--;
    }

    return path + i + 1;
}

// extracts file dir from path with filename
string_t ExtractFileDirW(const char_t* FileName)
{
    if (FileName == nullptr) return _T("");
    if (FileName[0] == '\0') return _T("");

    size_t i = wcslen(FileName);

    do
    {
        i--;
        if ((FileName[i] == '\\') || (FileName[i] == '/'))
        {
            i++;
            break;
        }
    } while (i > 0);

    return string_t(FileName).substr(0, i);
}


char mytoupper(int c); // to eliminate compile warning "warning C4244: '=': conversion from 'int' to 'char', possible loss of data"


uint32_t ParseBlockSize(std::string s) // s is by value intentionally
{
    int factor = 1;
    transform(s.begin(), s.end(), s.begin(), mytoupper); // to uppercase

    if (s.find_last_of('K') != std::string::npos)
        //if (s.lastIndexOf("K") > 0)
    {
        s = s.substr(0, s.length() - 1);
        factor = 1024;
    }
    else if (s.find_last_of('M') != std::string::npos)
    {
        s = s.substr(0, s.length() - 1);
        factor = 1024 * 1024;
    }

    return stoi(s) * factor;
}

// function for convert bool Value to string
string_t BooolToStr(bool Value)
{
    return Value ? _T("1") : _T("0");
}

// function to convert string value to the bool ('1','yes','true'=true; all the others=false)
bool StrToBoool(const string_t& Value)
{
    return EqualNCase(Value, string_t(_T("1"))) || EqualNCase(Value, string_t(_T("yes"))) || EqualNCase(Value, string_t(_T("true")));
}

void StringListFromDeleteList(char* DeleteList, vect_string_t& StringList)
{
    //LOG_INFO("StringListFromDeleteList");
    char* start = DeleteList;
    while (true)
    {
        if (start[0] == '\0') break;
        StringList.push_back(convert_string<char_t>(start));
        //LOG_INFO(start);
        start += strlen(start) + 1;
    }
}

void StringListFromDeleteList(wchar_t* DeleteList, vect_string_t& StringList)
{
    //LOG_INFO("StringListFromDeleteList");
    wchar_t* start = DeleteList;
    while (true)
    {
        if (start[0] == '\0') break;
        StringList.push_back(start);
        //LOG_INFO(convert_string<char>(start));
        start += wcslen(start) + 1;
    }
}


