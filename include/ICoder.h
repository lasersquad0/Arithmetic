#pragma once
#include <stdint.h>
#include <iostream>
#include "CommonFunctions.h"

typedef uint64_t uint32or64; // переводит coder и model либо в uint32_t режим либо uint64_t. все int переменные становятся либо 32bit либо 64bit
typedef int64_t int32or64;

class ICoder
{
public:
    virtual uint32or64 GetBytesPassed() = 0;
    virtual void StartEncode(std::ostream* f) = 0;
    virtual void FinishEncode() = 0;
    virtual void StartDecode(std::istream* f) = 0;
    virtual void FinishDecode() = 0;
    virtual void EncodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq) = 0;
    virtual uint32or64 GetCumFreq(uint32or64 totalFreq) = 0;
    virtual void DecodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq) = 0;
    virtual uint32or64 GetIntParam(const std::string& paramName) = 0;
    virtual ~ICoder() {};
};

class IBlockCoder : public ICoder
{
public:
    virtual void StartBlockEncode() = 0;
    virtual void FinishBlockEncode() = 0;
    virtual void StartBlockDecode() = 0;
    virtual void FinishBlockDecode() = 0;
    virtual ~IBlockCoder() {};
};