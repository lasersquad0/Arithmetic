#pragma once

#include <iostream>
#include <cstdlib>
#include "BasicModel.h"


//typedef unsigned long U32;  // 32 bit type

//////////////////////////// Predictor /////////////////////////

/* A Predictor estimates the probability that the next bit of
   uncompressed data is 1.  
   Methods: 
   p() returns P(1) as a 12 bit number (0-4095).
   update(y) trains the predictor with the actual bit (0 or 1).
*/

class Predictor
{
private:
    int cxt;        // Context: last 0-8 bits with a leading 1
    int ct[512][2]; // 0 and 1 counts in context cxt
public:
    Predictor() : cxt(1)
    {
        memset(ct, 0, sizeof(ct));
    }

    // Assume a stationary order 0 stream of 9-bit symbols
    int p() const
    {
        return 4096 * (ct[cxt][1] + 1) / (ct[cxt][0] + ct[cxt][1] + 2);
    }

    void update(uchar y)
    {
        if (++ct[cxt][y] > 65534)
        {
            ct[cxt][0] >>= 1;
            ct[cxt][1] >>= 1;
        }

        if ((cxt += cxt + y) >= 512)
            cxt = 1;
    }
};


class fpaqBitCoder: public BasicModel, public IBlockCoder
{
private:
    Predictor predictor;
    //const Mode mode;       // Compress or decompress?
    //FILE* archive;         // Compressed data file
    std::ostream* fout = nullptr;
    std::istream* fin = nullptr;
    uint32or64 x1, x2;            // Range, initially [0, 1), scaled by 2^32
    uint32or64 x;                 // Last 4 input bytes of archive.
    uint32or64 bits_to_follow;
    uint32or64 bytesPassed;
    uchar bptr, bout, bptrin;
    int bin;
    void Init();

public:
    fpaqBitCoder(IBlockCoder& cr);
    //void encode(int y);    // Compress bit y
    //int decode();          // Uncompress and return bit y
    void flush();          // Call when done compressing
    void bit_plus_follow(int bit);
    int input_bit(void);
    uchar input_byte();
    void output_byte(uchar c);
    void EncodeSymbol(uchar y); // Compress bit y
    uchar DecodeSymbol(void);   // Uncompress and return bit y

    // BasicModel overrides
    void EncodeSymbol(uchar* ctx, uchar sym) override;
    void BeginEncode(std::ostream* f) override;
    void StopEncode() override;
    uchar DecodeSymbol(uchar* ctx) override;
    void BeginDecode(std::istream* f) override;
    void StopDecode() override;
    IBlockCoder& GetCoder() override { return *this; }

    // ICoder overrides
    uint32or64 GetBytesPassed() override { return bytesPassed; }
    void StartEncode(std::ostream*) override {}
    void FinishEncode() override {}
    void StartDecode(std::istream*) override {}
    void FinishDecode() override {}
    void EncodeByte(uint32or64, uint32or64, uint32or64 ) override {}
    uint32or64 GetCumFreq(uint32or64) override { return 0; }
    void DecodeByte(uint32or64, uint32or64, uint32or64) override {}
    uint32or64 GetIntParam(const std::string& ) override { return 0; }

    // IBlockCoder overrides
    void StartBlockEncode() override {}
    void FinishBlockEncode() override {}
    void StartBlockDecode() override {}
    void FinishBlockDecode() override {}
    
};

