#pragma once

#include "framework.h"
#include "Parameters.h"
#include "ArchiveHeader.h"

string_t BooolToStr(bool Value); // function for convert bool Value to string '0' or '1'
bool StrToBoool(const string_t& Value); // function to convert string value to the bool ('1','yes','true'=true; all the others=false) 

#define PARAM_BUFSIZE 20

class PluginParameters : public Parameters
{
public:
    string_t LOGFILENAME;

    // it is assumed that InifileName already initialized with proper value from PackSetDefaultParams  
// Note! Logger is not initialized yet, do not call it from this function
    void LoadFromIni(const string_t& IniFile)
    {
        char_t value[PARAM_BUFSIZE];

        BLOCK_SIZE = GetPrivateProfileInt(CONFIG_SECTION, CONFIG_PARAM_BLOCKSIZE, BLOCK_SIZE, IniFile.c_str());
        THREADS = GetPrivateProfileInt(CONFIG_SECTION, CONFIG_PARAM_THREADS, THREADS, IniFile.c_str());
        GetPrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_MODEL, ModelTypeCode[(uint)MODEL_TYPE].c_str(), value, PARAM_BUFSIZE, IniFile.c_str());
        MODEL_TYPE = (ModelType)ModelIdByName(value);
        GetPrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_CODER, CoderNames[(uint)CODER_TYPE].c_str(), value, PARAM_BUFSIZE, IniFile.c_str());
        CODER_TYPE = (CoderType)CoderIdByName(value);
        GetPrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_STREAMMODE, BooolToStr(!BLOCK_MODE).c_str(), value, PARAM_BUFSIZE, IniFile.c_str());
        BLOCK_MODE = !StrToBoool(value);

        LOGFILENAME.resize(MAX_PATH);
        GetPrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_LOGFILENAME, LOGGER_FILE_NAME, LOGFILENAME.data(), MAX_PATH, IniFile.c_str());
    }

    void SaveToIni(const string_t& IniFile)
    {
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_BLOCKSIZE, std::to_wstring(BLOCK_SIZE).c_str(), IniFile.c_str());
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_MODEL, ModelTypeCode[(uint)MODEL_TYPE].c_str(), IniFile.c_str());
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_CODER, CoderNames[(uint)CODER_TYPE].c_str(), IniFile.c_str());
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_THREADS, std::to_wstring(THREADS).c_str(), IniFile.c_str());
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_STREAMMODE, BooolToStr(!BLOCK_MODE).c_str(), IniFile.c_str());
        WritePrivateProfileString(CONFIG_SECTION, CONFIG_PARAM_LOGFILENAME, LOGFILENAME.c_str(), IniFile.c_str());
    }

    ModelType ModelIdByName(wchar_t* modelName)
    {
        for (uint i = 0; i < sizeof(ModelTypeCode) / sizeof(ModelTypeCode[0]); i++)
        {
            if (ModelTypeCode[i] == modelName)
                return (ModelType)i;
        }
        return (ModelType)0;
    }

    CoderType CoderIdByName(wchar_t* coderName)
    {
        for (uint i = 0; i < sizeof(CoderNames) / sizeof(CoderNames[0]); i++)
        {
            if (CoderNames[i] == coderName)
                return (CoderType)i;
        }
        return (CoderType)0;
    }
};



void WCHARtoChar(char* dest, wchar_t* src); // assuming that dest buffer is large enough
void CharToWCHAR(wchar_t* dest, const char* src); // assuming that dest buffer is large enough
uint MakeTCTime(time_t tt);
char_t* ExtractFileNameW(char_t* path);
string_t ExtractFileDirW(const char_t* FileName);
uint32_t ParseBlockSize(std::string s); // s is by value intentionally


void StringListFromDeleteList(char* DeleteList, vect_string_t& StringList);
void StringListFromDeleteList(wchar_t* DeleteList, vect_string_t& StringList);