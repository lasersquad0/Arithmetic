#pragma once

#include <fstream>
//#include <stdint.h>
//#include <log4cpp/Category.hh>

//using namespace std;

typedef unsigned char uchar;

typedef uint64_t uint32or64; // переводит coder и model либо в uint32_t режим либо uint64_t. все int переменные становятся либо 32bit либо 64bit

#define  DEFAULT_CODEBITS  (24)
//#define  DEFAULT_BOTTOM    (16)

//#define  TOP       (1<<CODEBITS)
//#define  BOTTOM    (1<<(CODEBITS - 8))

#define SHOW_PROGRESS_AFTER 1000

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
};

class IBlockCoder: public ICoder
{
public:
    virtual void StartBlockEncode()  = 0;
    virtual void FinishBlockEncode() = 0;
    virtual void StartBlockDecode()  = 0;
    virtual void FinishBlockDecode() = 0;
};

class RangeCoder: IBlockCoder
{
public:
    const uint32or64 CODEBITS;
    const uint32or64 HIGHBYTE;
    const uint32or64 TOP;
    const uint64_t TOPTOP; // эта поле всегда должно быть 64bit
    const uint32or64 BOTTOM;
private:
    inline static RangeCoder* instance = nullptr;
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
    RangeCoder(uint32or64 codebits = DEFAULT_CODEBITS); // disable direct creating of RC instance

public:
    static RangeCoder& GetCoder() 
    { 
        if (instance == nullptr) instance = new RangeCoder();
        return *instance;
    };

    uint32or64 GetBytesPassed() override { return bytesPassed; }
    
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

class CallBack
{
public:
    void start() {}
    void finish() { printf("\n"); }
    void progress(uint64_t progress)
    {
        printf("\rProgress %llu%%...", progress);
    }

};
