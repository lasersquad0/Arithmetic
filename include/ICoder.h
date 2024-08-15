#pragma once
#include <stdint.h>
#include <iostream>
#include "CommonFunctions.h"

//typedef uint64_t uint32or64; // switches coder and model into either uint32_t or uint64_t mode. all integer variables become either 32bit or 64bit 
//typedef int64_t int32or64;

template<class UINT>
class ICoder
{
public:
    virtual uint64_t GetBytesPassed() = 0; // return value should always be 64bit - uint64_t 
    virtual void StartEncode(std::ostream* f) = 0;
    virtual void FinishEncode() = 0;
    virtual void StartDecode(std::istream* f) = 0;
    virtual void FinishDecode() = 0;
    virtual void EncodeByte(UINT cumFreq, UINT freq, UINT totalFreq) = 0;
    virtual UINT GetCumFreq(UINT totalFreq) = 0;
    virtual void DecodeByte(UINT cumFreq, UINT freq, UINT totalFreq) = 0;
    virtual UINT GetIntParam(const std::string& paramName) = 0;
    virtual ~ICoder() {}
};

template<class UINT>
class IBlockCoder : public ICoder<UINT>
{
public:
    virtual void StartBlockEncode() = 0;
    virtual void FinishBlockEncode() = 0;
    virtual void StartBlockDecode() = 0;
    virtual void FinishBlockDecode() = 0;
    virtual ~IBlockCoder() {}
};

typedef IBlockCoder<uint32_t> IBlockCoder32;
typedef IBlockCoder<uint64_t> IBlockCoder64;
