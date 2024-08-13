#pragma once

#include "framework.h"
#include "wcxhead.h"
#include "CommonFunctions.h"
#include "Callback.h"
#include "LogEngine.h"

class TCCallback : public ICallback
{
private:
    std::string FFilename;
    tProcessDataProc FProgressDataProc;
public:
    TCCallback(const std::string& FileName, tProcessDataProc progressProc)
    {
        LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);
        LOG_INFO("[TCCallback] constructor");
        if(progressProc == nullptr)
            LOG_ERROR("[TCCallback] progressPros IS NULL!!!");

        FFilename = FileName;
        FProgressDataProc = progressProc;
    }

    void start() override {}
    void finish() override {}
    int progress(uint64_t progress) override
    {
        LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);
        LOG_DEBUG2("[TCCallback] progress: {}%", progress);

        // is first parameter of FProgressDataProc is null then Progress TC window shows from and two paths correctly.
        // is first parameter of FProgressDataProc is name of the being copied file then Progress TC window shows just this filename that look bad.
        if (CALLBACK_ABORT == FProgressDataProc(nullptr/*(char*)FFilename.c_str()*/, -(int)progress))
        {
            LOG_INFO2("[TCCallback] User cancelled operation at {}%.", progress);
            return CALLBACK_ABORT;
        }
        return CALLBACK_OK;
    }

};

class TCCallbackW : public ICallback
{
private:
    string_t FFilename;
    tProcessDataProcW FProgressDataProc;
public:
    TCCallbackW(const string_t& FileName, tProcessDataProcW progressProc)
    {
        LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);
        LOG_INFO("[TCCallbackW] constructor");
        if (progressProc == nullptr)
            LOG_ERROR("[TCCallbackW] progressPros IS NULL!!!");

        FFilename = FileName;
        FProgressDataProc = progressProc;
    }

    void start() override {}
    void finish() override {}
    int progress(uint64_t progress) override
    {
        LogEngine::Logger& logger = LogEngine::GetLogger(LOGGER_NAME);
       
        // is first parameter of FProgressDataProc is null then Progress TC window shows from and two paths correctly.
        // is first parameter of FProgressDataProc is name of the being copied file then Progress TC window shows just this filename that look bad.
        int res = FProgressDataProc((char_t*)FFilename.c_str(), -(int)progress);
        if (CALLBACK_ABORT == res)
        {
            LOG_INFO2("[TCCallbackW] User cancelled operation at {}%.", progress);
            return CALLBACK_ABORT;
        }

        LOG_DEBUG2("[TCCallbackW] progress: {}%", progress);
        return CALLBACK_OK;
    }

};
