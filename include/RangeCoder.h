#pragma once

#include <fstream>
#include <assert.h>
//#include <stdint.h>
#include "ICoder.h"
#include "ARIExceptions.h"

#define  DEF_RANGECODER_CODEBITS  (24)
//#define  DEFAULT_BOTTOM    (16)

//#define  TOP       (1<<CODEBITS)
//#define  BOTTOM    (1<<(CODEBITS - 8))

template<class UINT>
class RangeCoder: public IBlockCoder<UINT>
{
public:
    const uint8_t CODEBITS;
    const uint8_t HIGHBYTE;
    const UINT TOP;
    const uint64_t TOPTOP; // this field always should be 64bit
    const UINT BOTTOM;
private:
    UINT low = 0;
    UINT code = 0;
    UINT range;
    uint64_t bytesPassed = 0; //should always be 64bit variable, because we may compress files with more than 4G size
    std::ostream* fout = nullptr;
    std::istream* fin  = nullptr;

    void OutByte(uchar c);
    uchar InByte();

    void SaveState();
    void LoadState();
    void ResetLowRange();
    RangeCoder(uint8_t codebits = DEF_RANGECODER_CODEBITS); // disable direct creating of RC instance
    
  //  friend class CoderFactory;
    friend class ModelCoderFactory;

public:
    //static RangeCoder& GetCoder() 
    //{ 
    //    if (instance == nullptr) instance = new RangeCoder();
    //    return *instance;
    //};

    uint64_t GetBytesPassed() override { return bytesPassed; }
    UINT GetIntParam(const std::string& paramName) override;

    void StartEncode(std::ostream* f) override;
    void FinishEncode() override;
    void StartDecode(std::istream* f) override;
    void FinishDecode() override;
    
    void StartBlockEncode() override;
    void FinishBlockEncode() override;
    void StartBlockDecode() override;
    void FinishBlockDecode() override;

    void EncodeByte(UINT cumFreq, UINT freq, UINT totalFreq) override;
    void DecodeByte(UINT cumFreq, UINT freq, UINT totalFreq) override;
    UINT GetCumFreq(UINT totFreq) override;
};

typedef RangeCoder<uint32_t> RangeCoder32;
typedef RangeCoder<uint64_t> RangeCoder64;


template<class UINT>
RangeCoder<UINT>::RangeCoder(uint8_t codebits) :
    CODEBITS(codebits),
    TOP(1ull << codebits),
    HIGHBYTE((codebits >> 3) + 1),
    TOPTOP(1ull << (codebits + 8)), // TOPTOP is always 64 bit
    BOTTOM(1ull << (codebits - 8))
{
    range = (UINT)(TOPTOP - 1); // initialise into widest interval
}

template<class UINT>
UINT RangeCoder<UINT>::GetCumFreq(UINT totalFreq)
{
    UINT tmp = (code - low) / (range /= totalFreq);
    if (tmp >= totalFreq)
        throw bad_file_format("Input data corrupt."); // or force it to return a valid value :)
    return tmp;
}

template<class UINT>
void RangeCoder<UINT>::EncodeByte(UINT cumFreq, UINT freq, UINT totalFreq)
{
    // Fixed error: 'cumFreq+freq<totFreq' was replaced wit h 'cumFreq+freq<=totFreq'
    assert(cumFreq + freq <= totalFreq);
    assert(freq > 0);
    assert(totalFreq <= BOTTOM);

    low += cumFreq * (range /= totalFreq);
    range *= freq;
    //while ((low ^ low+range)<TOP || range<BOTTOM && ((range= -low & BOTTOM-1),1))
    while ((low ^ (low + range)) < TOP || (range < BOTTOM) && ((range = BOTTOM - (low & (BOTTOM - 1))), 1))
        OutByte((uchar)(low >> CODEBITS)),
        range <<= 8,
        low <<= 8;

    // (low ^ (low + range)) < TOP - returns true when 4th byte is the same in low and high - candidate to write to the output.
    // ((range = BOTTOM - (low & (BOTTOM - 1))), 1) - triggers only when range<BOTTOM, weird modification of range.
}

template<class UINT>
void RangeCoder<UINT>::DecodeByte(UINT cumFreq, UINT freq, UINT totalFreq)
{
    // Fixed error: 'cumFreq+freq<totFreq' was replaced with 'cumFreq+freq<=totFreq'
    assert(cumFreq + freq <= totalFreq);
    assert(freq > 0);
    assert(totalFreq <= BOTTOM);

    low += cumFreq * range;
    range *= freq;
    while ((low ^ (low + range)) < TOP || (range < BOTTOM) && ((range = BOTTOM - (low & (BOTTOM - 1))), 1))
        code = code << 8 | InByte(),
        range <<= 8,
        low <<= 8;
}

template<class UINT>
uchar RangeCoder<UINT>::InByte()
{
    int ch = fin->get();
    if (ch == std::char_traits<char>::eof())
        throw bad_file_format("EOF got unexpectedly.");

    bytesPassed++;
    return (uchar)ch;
}

template<class UINT>
void RangeCoder<UINT>::OutByte(uchar c)
{
    bytesPassed++;
    fout->put((char)c);
}

template<class UINT>
void RangeCoder<UINT>::SaveState()
{
    for (uint8_t i = 0; i < HIGHBYTE; i++)
        OutByte((uchar)(low >> CODEBITS)), low <<= 8;
}

template<class UINT>
void RangeCoder<UINT>::LoadState()
{
    for (uint8_t i = 0; i < HIGHBYTE; i++)
        code = code << 8 | InByte();
}

template<class UINT>
void RangeCoder<UINT>::ResetLowRange()
{
    low = code = 0;
    range = (UINT)(TOPTOP - 1);

}

template<class UINT>
UINT RangeCoder<UINT>::GetIntParam(const std::string& paramName)
{
    if (paramName == "MAX_FREQ") return BOTTOM;
    throw std::invalid_argument("[RangeCoder.GetIntParam] Invalid parameters has been requested.");
}

template<class UINT>
void RangeCoder<UINT>::StartEncode(std::ostream* f)
{
    fout = f;
    bytesPassed = 0;

    ResetLowRange();
}

template<class UINT>
void RangeCoder<UINT>::FinishEncode()
{
    SaveState();
}

template<class UINT>
void RangeCoder<UINT>::StartDecode(std::istream* f)
{
    fin = f;
    bytesPassed = 0;

    ResetLowRange();
    LoadState();
};

template<class UINT>
void RangeCoder<UINT>::FinishDecode()
{
    //nothing
};


template<class UINT>
void RangeCoder<UINT>::StartBlockEncode()
{
    ResetLowRange();
}

template<class UINT>
void RangeCoder<UINT>::FinishBlockEncode()
{
    SaveState();
}

template<class UINT>
void RangeCoder<UINT>::StartBlockDecode()
{
    ResetLowRange();
    LoadState();
}

template<class UINT>
void RangeCoder<UINT>::FinishBlockDecode()
{
    //nothing
}
