#pragma once
#include "ICoder.h"

#pragma once

#include <fstream>
#include "ICoder.h"


#define DEF_BITCODER_CODEBITS  (16)


class BitCoder : public IBlockCoder
{
public:
    const uint32or64 CODEBITS = DEF_BITCODER_CODEBITS;
    const uint32or64 TOP = static_cast<uint32or64>(1)<<CODEBITS;
    const uint32or64 TOP_VALUE = TOP - 1; //0XFFFFFFFF;	  /* Largest code value */
    /* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
    const uint32or64 FIRST_QTR = (TOP_VALUE / 4 + 1);  /* Point after first quarter    */
    const uint32or64 HALF = (2 * FIRST_QTR);    /* Point after first half  */
    const uint32or64 THIRD_QTR = (3 * FIRST_QTR);    /* Point after third quarter */
    const uint32or64 MAX_FREQ = 16383; // ?????

    //const uint32or64 HIGHBYTE;
    //const uint64_t TOPTOP; // эта поле всегда должно быть 64bit
    //const uint32or64 BOTTOM;
private:
    
    uint32or64 low    = 0;
    uint32or64 high   = 0; 
    uint32or64 nextCh = 0;
    //uint32or64 range;
    uint8_t bitsToFollow = 0;
    int buffer = 0;
    uint8_t bitsToGo = 0;
    uint8_t garbage_bits = 0;

    uint64_t bytesPassed = 0; //should always be 64bit variable, because we may compress files with more than 4G size
    std::ostream* fout = nullptr;
    std::istream* fin = nullptr;

    void OutByte(uchar c);
    void outputBit(int bit);
    int inputBit();
    void outputBitPlusFollow(int bit);
    void ResetLowRange();
    BitCoder(uint32or64 codebits = DEF_BITCODER_CODEBITS); // disable direct creating of RC instance

    friend class CoderFactory;
    friend class ModelCoderFactory;

public:
    uint32or64 GetBytesPassed() override { return bytesPassed; }
    uint32or64 GetIntParam(const std::string& paramName);

    void StartEncode(std::ostream* f) override;
    void SaveState();
    void FinishEncode() override;
    void LoadInitialBits();
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

