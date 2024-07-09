#include <assert.h>
#include "RangeCoder.h"
#include "ARIExceptions.h"

using namespace std;

#define DO(n) for (uint32or64 _=0; _<n; _++)

RangeCoder::RangeCoder(uint32or64 codebits):
    CODEBITS(codebits),
    TOP(1ull << codebits),
    HIGHBYTE((codebits>>3) + 1), 
    TOPTOP(1ull << (codebits + 8)), 
    BOTTOM(1ull << (codebits - 8))
{
    range = (uint32or64)(TOPTOP - 1); // initialise into widest interval
}

uint32or64 RangeCoder::GetCumFreq(uint32or64 totalFreq)
{
    uint32or64 tmp = (code - low) / (range /= totalFreq);
    if (tmp >= totalFreq)
        throw bad_file_format("Input data corrupt."); // or force it to return a valid value :)
    return tmp;
}

void RangeCoder::EncodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq)
{
    // Fixed error: 'cumFreq+freq<totFreq' was replaced wit h 'cumFreq+freq<=totFreq'
    assert(cumFreq + freq <= totalFreq);
    assert(freq > 0);
    assert(totalFreq <= BOTTOM);
   
    low += cumFreq * (range /= totalFreq);
    range *= freq;
    //while ((low ^ low+range)<TOP || range<BOTTOM && ((range= -low & BOTTOM-1),1))
    while ((low ^ (low + range)) < TOP || (range <  BOTTOM) && ((range = BOTTOM - (low & (BOTTOM - 1))), 1))
        OutByte((uchar)(low >> CODEBITS)),
        range <<= 8,
        low <<= 8;

    // (low ^ (low + range)) < TOP - дает true когда 4й байт одинаковый у low и high - значит он кандидат на запись в выход.
    // ((range = BOTTOM - (low & (BOTTOM - 1))), 1) - Срабатывает только если range<BOTTOM переопределяет range странным способом.
}

void RangeCoder::DecodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq)
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

uchar RangeCoder::InByte() 
{ 
    int ch = fin->get();
    if(ch == std::char_traits<char>::eof())
        throw bad_file_format("EOF got unexpectedly.");
    
    bytesPassed++; 
    return (uchar)ch;
}

void RangeCoder::OutByte(uchar c)
{
    bytesPassed++; 
    fout->put(c);
}

void RangeCoder::SaveState()
{
    DO(HIGHBYTE) OutByte((uchar)(low >> CODEBITS)), low <<= 8;
}

void RangeCoder::ResetLowRange()
{
    low = code = 0;
    range = (uint32or64)(TOPTOP - 1); 

}

uint32or64 RangeCoder::GetIntParam(const string& paramName)
{
    if (paramName == "MAX_FREQ") return BOTTOM;
    throw invalid_argument("[RangeCoder.GetIntParam] Invalid parameters has been requested.");
}

void RangeCoder::StartEncode(ostream* f)
{ 
    fout = f; 
    bytesPassed = 0;

    ResetLowRange();
}

void RangeCoder::FinishEncode() 
{
    SaveState();
}

void RangeCoder::StartDecode(istream* f)
{
    fin = f; 
    bytesPassed = 0;

    ResetLowRange();

    DO(HIGHBYTE) code = code << 8 | InByte();
};

void RangeCoder::FinishDecode()
{
    //nothing
};


void RangeCoder::StartBlockEncode()
{
    ResetLowRange();
}

void RangeCoder::FinishBlockEncode()
{
    SaveState();
}

void RangeCoder::StartBlockDecode()
{
    ResetLowRange();

    DO(HIGHBYTE) code = code << 8 | InByte();
}

void RangeCoder::FinishBlockDecode()
{
    //nothing
}

