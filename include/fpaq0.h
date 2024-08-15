#pragma once

#include <iostream>
#include <cstdlib>
#include <cassert>
#include "BasicModel.h"
#include "ARIExceptions.h"

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

template<class UINT>
class fpaqBitCoder: public BasicModel<UINT>, public IBlockCoder<UINT>
{
private:

#define Top_value UINT(0XFFFFFFFF)	  /* Largest code value */
    /* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr UINT(Top_value/4+1)  /* Point after first quarter    */
#define Half	  UINT(2*First_qtr)    /* Point after first half  */
#define Third_qtr UINT(3*First_qtr)    /* Point after third quarter */

    Predictor predictor;
    //const Mode mode;       // Compress or decompress?
    //FILE* archive;         // Compressed data file
    std::ostream* fout = nullptr;
    std::istream* fin = nullptr;
    UINT x1, x2;            // Range, initially [0, 1), scaled by 2^32
    UINT x;                 // Last 4 input bytes of archive.
    UINT bits_to_follow;
    uint64_t bytesPassed; // should be 64bit value
    uchar bptr, bout, bptrin;
    int bin;
    void Init();

public:
    fpaqBitCoder(IBlockCoder<UINT>& cr);
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
    void BeginEncode(std::ostream* f, std::istream* fi = nullptr) override;
    void StopEncode() override;
    uchar DecodeSymbol(uchar* ctx) override;
    void BeginDecode(std::istream* f) override;
    void StopDecode() override;
    void UpdateStatistics(uchar*, uchar) override {}
    //IBlockCoder<UINT>& GetCoder() override { return *this; }

    // ICoder overrides
    uint64_t GetBytesPassed() override { return bytesPassed; }
    void StartEncode(std::ostream*) override {}
    void FinishEncode() override {}
    void StartDecode(std::istream*) override {}
    void FinishDecode() override {}
    void EncodeByte(UINT, UINT, UINT) override {}
    UINT GetCumFreq(UINT) override { return 0; }
    void DecodeByte(UINT, UINT, UINT) override {}
    UINT GetIntParam(const std::string& ) override { return 0; }

    // IBlockCoder overrides
    void StartBlockEncode() override {}
    void FinishBlockEncode() override {}
    void StartBlockDecode() override {}
    void FinishBlockDecode() override {}
    
};

typedef fpaqBitCoder<uint32_t> fpaqBitCoder32;
typedef fpaqBitCoder<uint64_t> fpaqBitCoder64;

template<class UINT>
fpaqBitCoder<UINT>::fpaqBitCoder(IBlockCoder<UINT>& cr) : BasicModel<UINT>(cr), predictor(), x1(0), x2(Top_value),
x(0), bits_to_follow(0), bptr(128), bout(0), bptrin(1), bytesPassed(0)
{

}

template<class UINT>
inline int fpaqBitCoder<UINT>::input_bit(void)
{
    if (!(bptrin >>= 1))
    {
        //bin = getc(archive);
        bin = input_byte();
        if (bin == EOF) bin = 0;
        bptrin = 128;
    }
    return ((bin & bptrin) != 0);
}

template<class UINT>
uchar fpaqBitCoder<UINT>::input_byte()
{
    int ch = fin->get();

    if (ch == std::char_traits<char>::eof())
        throw bad_file_format("EOF got unexpectedly.");

    bytesPassed++;
    return (uchar)ch; /* fgetc(f);*/
}

template<class UINT>
void fpaqBitCoder<UINT>::output_byte(uchar c)
{
    bytesPassed++;
    fout->put(c);
}

template<class UINT>
void fpaqBitCoder<UINT>::Init()
{
    bytesPassed = 0;
    x1 = 0;
    x2 = Top_value;
    x = 0;
    bits_to_follow = 0;
    bptr = 128;
    bout = 0;
    bptrin = 1;
}

template<class UINT>
void fpaqBitCoder<UINT>::BeginEncode(std::ostream* f, std::istream* fi)
{
    fout = f;
    Init();
}

template<class UINT>
void fpaqBitCoder<UINT>::StopEncode()
{
    this->flush();
}

template<class UINT>
void fpaqBitCoder<UINT>::BeginDecode(std::istream* f)
{
    fin = f;
    Init();

    // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
    //if (mode == DECOMPRESS)
    //{
    x = 1;
    for (; x < Half;) x += x + input_bit();
    x += x + input_bit();
    //}
}

template<class UINT>
void fpaqBitCoder<UINT>::StopDecode()
{
    //nothing to do
}


/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange. Output leading bytes of the range as they become known. */
template<class UINT>
inline void fpaqBitCoder<UINT>::EncodeSymbol(uchar*, uchar sym) //(int y) 
{
    //int c = *sym;
    //EncodeSymbol((uchar)0);
    for (int i = 7; i >= 0; --i)
        EncodeSymbol((uchar)((sym >> i) & 1));
}

template<class UINT>
inline void fpaqBitCoder<UINT>::EncodeSymbol(uchar y) //(int y) 
{
    // Update the range
    const UINT xmid = x1 + ((x2 - x1) >> 12) * predictor.p();
    assert(x1 <= Top_value && x2 <= Top_value);
    assert(xmid >= x1 && xmid < x2);
    if (y)
        x2 = xmid;
    else
        x1 = xmid + 1;
    predictor.update(y);

    // Shift equal MSB's out
    for (;;)
    {
        if (x2 < Half)
        {
            bit_plus_follow(0);
        }
        else if (x1 >= Half)
        {
            bit_plus_follow(1);
            x1 -= Half;
            x2 -= Half;
        }
        else if (x1 >= First_qtr && x2 < Third_qtr)
        {
            bits_to_follow++;
            x1 -= First_qtr;
            x2 -= First_qtr;
            //x1 ^= First_qtr;
            //x2 ^= First_qtr;
        }
        else
        {
            break;
        }
        x1 += x1;
        //x1 &= Top_value;
        x2 += x2 + 1;
        //x2 &= Top_value;
    }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/
template<class UINT>
inline uchar fpaqBitCoder<UINT>::DecodeSymbol(uchar*)
{
    int c = 0;

    for (int i = 7; i >= 0; --i)
        c += c + DecodeSymbol();
    return (uchar)c;

    //while (c < 256)
    //    c += c + DecodeSymbol();
    ////putc(c - 256, out);
    //return (uchar)(c-256);
}

template<class UINT>
inline uchar fpaqBitCoder<UINT>::DecodeSymbol(void)
{
    // Update the range
    const UINT xmid = x1 + ((x2 - x1) >> 12) * predictor.p();
    assert(x1 <= Top_value && x2 <= Top_value);
    assert(xmid >= x1 && xmid < x2);
    uchar y = 0;
    if (x <= xmid)
    {
        y = 1;
        x2 = xmid;
    }
    else
        x1 = xmid + 1;

    predictor.update(y);

    // Shift equal MSB's out
    for (;;)
    {
        if (x2 < Half)
        {
        }
        else if (x1 >= Half) 	   /* Output 1 if in high half. */
        {
            x1 -= Half;
            x -= Half;
            x2 -= Half;		        /* Subtract offset to top.  */
        }
        else if (x1 >= First_qtr && x2 < Third_qtr)	/* Output an opposite bit  later if in middle half. */
        {
            x1 -= First_qtr;	    /* Subtract offset to middle */
            x -= First_qtr;
            x2 -= First_qtr;
        }
        else
        {
            break;			/* Otherwise exit loop.     */
        }
        x1 += x1;
        x += x + input_bit();
        x2 += x2 + 1;	/* Scale up code range.     */
    }
    return y;
}

// Should be called when there is no more to compress
template<class UINT>
void fpaqBitCoder<UINT>::flush()
{
    // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
   // if (mode == COMPRESS) 
   // {
    bits_to_follow = 0;       //FB 10/01/2006
    if (x1 == 0)
        bit_plus_follow(0);
    else
        bit_plus_follow(1);
    if (bout) output_byte(bout); //putc(bout, archive);
    // }
}

template<class UINT>
inline void fpaqBitCoder<UINT>::bit_plus_follow(int bit)
{
    bits_to_follow++;
    for (int notb = bit ^ 1; bits_to_follow > 0; bits_to_follow--, bit = notb)
    {
        if (bit) bout |= bptr;
        if (!(bptr >>= 1))
        {
            output_byte(bout);
            //putc(bout, archive);
            bptr = 128;
            bout = 0;
        }
    }
}

