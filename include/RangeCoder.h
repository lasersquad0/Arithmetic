#pragma once

#include <fstream>
//#include <stdint.h>
#include "ICoder.h"


#define  DEF_RANGECODER_CODEBITS  (24)
//#define  DEFAULT_BOTTOM    (16)

//#define  TOP       (1<<CODEBITS)
//#define  BOTTOM    (1<<(CODEBITS - 8))

#define SHOW_PROGRESS_AFTER 1000



class RangeCoder: public IBlockCoder
{
public:
    const uint32or64 CODEBITS;
    const uint32or64 HIGHBYTE;
    const uint32or64 TOP;
    const uint64_t TOPTOP; // this field always should be 64bit
    const uint32or64 BOTTOM;
private:
    uint32or64 low = 0;
    uint32or64 code = 0;
    uint32or64 range;
    uint64_t bytesPassed = 0; //should always be 64bit variable, because we may compress files with more than 4G size
    std::ostream* fout = nullptr;
    std::istream* fin  = nullptr;

    void OutByte(uchar c);
    uchar InByte();

    void SaveState();
    void ResetLowRange();
    RangeCoder(uint32or64 codebits = DEF_RANGECODER_CODEBITS); // disable direct creating of RC instance
    
    friend class CoderFactory;
    friend class ModelCoderFactory;

public:
    //static RangeCoder& GetCoder() 
    //{ 
    //    if (instance == nullptr) instance = new RangeCoder();
    //    return *instance;
    //};

    uint32or64 GetBytesPassed() override { return bytesPassed; }
    uint32or64 GetIntParam(const std::string& paramName);

    void StartEncode(std::ostream* f) override;
    void FinishEncode() override;
    void StartDecode(std::istream* f) override;
    void FinishDecode() override;
    
    void StartBlockEncode() override;
    void FinishBlockEncode() override;
    void StartBlockDecode() override;
    void FinishBlockDecode() override;

    void EncodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq) override;
    void DecodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq) override;
    uint32or64 GetCumFreq(uint32or64 totFreq) override;
};

/*
class RangeCoderOLD
{
    // log4cpp::Category& logger;
    uint  low, code, range;
    uint64_t bytesPassed;
    ofstream* fout;
    ifstream* fin;
   

    void OutByte(uchar c);
    uchar   InByte();

public:
    RangeCoderOLD();
    uint64_t GetBytesPassed() { return bytesPassed; }
    void StartEncode(ofstream* f) { fout = f; bytesPassed = low = 0;  range = (uint)-1; }
    void FinishEncode() { DO(4)  OutByte(low >> 24), low <<= 8; }
    void StartDecode(ifstream* f);
    void Encode(uint cumFreq, uint freq, uint totFreq);
    uint GetFreq(uint totFreq);
    void Decode(uint cumFreq, uint freq, uint totFreq);
};
*/


