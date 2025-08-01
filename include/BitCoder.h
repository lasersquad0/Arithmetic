#pragma once
#include "ICoder.h"

#pragma once

#include <fstream>
#include "ICoder.h"
#include "ARIExceptions.h"


#define DEF_BITCODER_CODEBITS  (16)

template<class UINT>
class BitCoder : public IBlockCoder<UINT>
{
public:
    const uint8_t CODEBITS = DEF_BITCODER_CODEBITS;
    const UINT TOP = static_cast<UINT>(1)<<CODEBITS;
    const UINT TOP_VALUE = TOP - 1; //0XFFFFFFFF;	  /* Largest code value */
    /* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
    const UINT FIRST_QTR = (TOP_VALUE / 4 + 1);  /* Point after first quarter    */
    const UINT HALF = (2 * FIRST_QTR);    /* Point after first half  */
    const UINT THIRD_QTR = (3 * FIRST_QTR);    /* Point after third quarter */
    const UINT MAX_FREQ = 16383; // ?????

    //const uint32or64 HIGHBYTE;
    //const uint64_t TOPTOP; // ��� ���� ������ ������ ���� 64bit
    //const uint32or64 BOTTOM;
private:
    
    UINT low    = 0;
    UINT high   = 0;
    UINT nextCh = 0;
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
	BitCoder(); // disable direct creating of RC instance 

    friend class CoderFactory;
    friend class ModelCoderFactory;

public:

    uint64_t GetBytesPassed() override { return bytesPassed; }
    UINT GetIntParam(const std::string& paramName) override;

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

    void EncodeByte(UINT cumFreq, UINT freq, UINT totalFreq) override;
    void DecodeByte(UINT cumFreq, UINT freq, UINT totalFreq) override;
    UINT GetCumFreq(UINT totFreq) override;
};

typedef BitCoder<uint32_t> BitCoder32;
typedef BitCoder<uint64_t> BitCoder64;

template<class UINT>
BitCoder<UINT>::BitCoder()
	//CODEBITS(codebits),
	//TOP(1ull << codebits),
	//TOP_VALUE(TOP - 1),
   // HIGHBYTE((codebits >> 3) + 1),
   // TOPTOP(1ull << (codebits + 8)),
   // BOTTOM(1ull << (codebits - 8))
{

}

template<class UINT>
UINT BitCoder<UINT>::GetIntParam(const std::string& paramName)
{
	if (paramName == "MAX_FREQ") return MAX_FREQ;
	throw std::invalid_argument("[BitCoder.GetIntParam] Invalid parameters has been requested.");
}

template<class UINT>
void BitCoder<UINT>::ResetLowRange()
{
	low = 0;
	high = TOP_VALUE;

	bitsToFollow = 0;
	buffer = 0;
	garbage_bits = 0;
}

template<class UINT>
void BitCoder<UINT>::StartEncode(std::ostream* f)
{
	fout = f;
	bytesPassed = 0;
	bitsToGo = 8;
	ResetLowRange();
}

template<class UINT>
void BitCoder<UINT>::SaveState()
{
	bitsToFollow++;     // output of two bits which define a quater inside currect interval 
	if (low < FIRST_QTR)
		outputBitPlusFollow(0);
	else
		outputBitPlusFollow(1);

	//  putc(buffer >> bits_to_go, stdout);
	OutByte((uchar)(buffer >> bitsToGo));
}

template<class UINT>
void BitCoder<UINT>::FinishEncode()
{
	SaveState();

	ResetLowRange();
}

template<class UINT>
void BitCoder<UINT>::LoadInitialBits()
{
	nextCh = 0;  // input bits to fill in code value  
	for (uint i = 0; i < CODEBITS; i++)
	{
		nextCh = (nextCh << 1) + inputBit();
	}
}

template<class UINT>
void BitCoder<UINT>::StartDecode(std::istream* f)
{
	fin = f;
	bytesPassed = 0;
	bitsToGo = 0; // Note that this value differs from startEncode()
	ResetLowRange();

	LoadInitialBits();
}

template<class UINT>
void BitCoder<UINT>::FinishDecode()
{
	//nothing
}

template<class UINT>
void BitCoder<UINT>::StartBlockEncode()
{
	bitsToGo = 8; // Note that this value differs from startDecode()
	ResetLowRange();
}

template<class UINT>
void BitCoder<UINT>::FinishBlockEncode()
{
	SaveState();
}

template<class UINT>
void BitCoder<UINT>::StartBlockDecode()
{
	bitsToGo = 0; // Note that this value differs from startEncode()
	ResetLowRange();

	LoadInitialBits();
}

template<class UINT>
void BitCoder<UINT>::FinishBlockDecode()
{
	// nothing
}

template<class UINT>
void BitCoder<UINT>::EncodeByte(UINT cumFreq, UINT freq, UINT totalFreq)
{
	assert(cumFreq + freq <= totalFreq);
	assert(freq > 0);
	assert(totalFreq <= MAX_FREQ);

	UINT range = (high - low) + 1;
	high = low + (range * (cumFreq + freq)) / totalFreq - 1;
	low = low + (range * cumFreq) / totalFreq;
	assert(high >= low);

	//logger.finer(()->String.format("low=%X high=%X left=%X freq=%X symbol#%d", low, high, left, freq, inputBytes));

	for (; ; )
	{
		if (high < HALF)
		{
			outputBitPlusFollow(0);
		}
		else if (low >= HALF)
		{
			outputBitPlusFollow(1);
			low -= HALF;
			high -= HALF;
		}
		else if ((low >= FIRST_QTR) && (high < THIRD_QTR))
		{
			bitsToFollow++;
			low -= FIRST_QTR;
			high -= FIRST_QTR;
		}
		else break;

		low <<= 1;
		high = (high << 1) + 1;

		assert(low < TOP);
		assert(high < TOP);

		//logger.finer(()->String.format("low=%X high=%X", low, high));
	}
}

template<class UINT>
void BitCoder<UINT>::DecodeByte(UINT cumFreq, UINT freq, UINT totalFreq)
{
	assert(cumFreq + freq <= totalFreq);
	assert(freq > 0);
	assert(totalFreq <= MAX_FREQ);

	UINT range = (high - low) + 1;

	high = low + (range * (cumFreq + freq)) / totalFreq - 1;              /* ����� ���������� ������� */
	low = low + (range * cumFreq) / totalFreq;

	assert(high >= low);

	//logger.finer(()->String.format("low=%X high=%X nextCh=%X left=%X freq=%X", low, high, nextCh, left, freq));

	for (;;)                      //loop to throw away bits
	{
		if (high < HALF)         // expanding lower range
		{
			/* nothing */
		}
		else if (low >= HALF)   // expanding upper half after deducting Half shift
		{
			nextCh -= HALF;
			low -= HALF;
			high -= HALF;
		}
		else if ((low >= FIRST_QTR) && (high < THIRD_QTR)) // expanding middle half 
		{
			nextCh -= FIRST_QTR;
			low -= FIRST_QTR;
			high -= FIRST_QTR;
		}
		else break;

		low <<= 1;               // scaling interval 
		high = (high << 1) + 1;
		nextCh = (nextCh << 1) + inputBit();  // add new bit

		assert(low < TOP);
		assert(high < TOP);
		assert(nextCh < TOP);

		//logger.finer(()->String.format("low=%X high=%X nextCh=%X", low, high, nextCh));
	}
}

template<class UINT>
UINT BitCoder<UINT>::GetCumFreq(UINT totFreq)
{
	UINT tmp = ((nextCh - low + 1) * totFreq - 1) / (high - low + 1);
	if (tmp >= totFreq)
		throw bad_file_format("Input data corrupt."); // or force it to return a valid value :)
	return tmp;
}

template<class UINT>
void BitCoder<UINT>::outputBitPlusFollow(int bit)
{
	outputBit(bit);
	while (bitsToFollow > 0)
	{
		outputBit((~bit) & 0x01);
		bitsToFollow--;
	}
}

// Called during Compression only.
template<class UINT>
void BitCoder<UINT>::outputBit(int bit) // inputting bits starting from major bits and they are moving towards to minor bits.
{
	buffer >>= 1;   // free space for a new bit
	if (bit > 0) buffer |= 0x80;

	if (--bitsToGo > 0)
		return;

	OutByte((uchar)buffer);
	buffer = 0;   // clear buffer after writing
	bitsToGo = 8;
}

// called during Uncompression only

template<class UINT>
int BitCoder<UINT>::inputBit()
{
	if (bitsToGo == 0)
	{
		buffer = fin->get();
		bytesPassed++;

		if (buffer == EOF)
		{
			garbage_bits++;     // extra bits after EOF to proper finish algorythm + check on number of such bits (should not be too many of them)
			assert(garbage_bits <= CODEBITS - 2); //, "Incorrect archive file.");
		}
		bitsToGo = 8;
	}

	int t = buffer & 1;  // output bits from minor bits (right part of the byte)
	buffer >>= 1;        // TODO redo as implemented in AdaptHuffman
	bitsToGo--;
	return t;
}

template<class UINT>
void BitCoder<UINT>::OutByte(uchar c)
{
	bytesPassed++;
	fout->put(c);

	//fputc(c, f);
	//logger.debug("Output byte: 0x%X count:%d", c, bytesPassed);
}

