
#include <cassert>
#include "BitCoder.h"
#include "Exceptions.h"

using namespace std;

BitCoder::BitCoder(uint32or64 )
    //CODEBITS(codebits),
    //TOP(1ull << codebits),
	//TOP_VALUE(TOP - 1),
   // HIGHBYTE((codebits >> 3) + 1),
   // TOPTOP(1ull << (codebits + 8)),
   // BOTTOM(1ull << (codebits - 8))
{

}

uint32or64 BitCoder::GetIntParam(const string& paramName)
{
	if (paramName == "MAX_FREQ") return MAX_FREQ;
	throw invalid_argument("[BitCoder.GetIntParam] Invalid parameters has been requested.");
}

void BitCoder::ResetLowRange()
{
    low = 0;
    high = TOP_VALUE;

    bitsToFollow = 0;
    buffer = 0;
	garbage_bits = 0;
}

void BitCoder::StartEncode(ostream* f)
{
    fout = f;
    bytesPassed = 0;
    bitsToGo = 8;
    ResetLowRange();
}

void BitCoder::SaveState()
{
	bitsToFollow++;     // ����� ���� ����� ��p��������� ��p��p��, ������� � ������� ����p����
	if (low < FIRST_QTR)
		outputBitPlusFollow(0);
	else
		outputBitPlusFollow(1);

	//  putc(buffer >> bits_to_go, stdout);
	OutByte((uchar)(buffer >> bitsToGo));
}

void BitCoder::FinishEncode()
{
	SaveState();

	ResetLowRange();
}
   
void BitCoder::LoadInitialBits()
{
	nextCh = 0;  // ���� ����� ��� ���������� �������� ���� 
	for (int i = 0; i < CODEBITS; i++)
	{
		nextCh = (nextCh << 1) + inputBit();
	}
}

void BitCoder::StartDecode(istream* f)
{
    fin = f;
    bytesPassed = 0;
    bitsToGo = 0; // Note that this value differs from startEncode()
    ResetLowRange();

	LoadInitialBits();
}

void BitCoder::FinishDecode()
{
    //nothing
}

void BitCoder::StartBlockEncode()
{
	bitsToGo = 8; // Note that this value differs from startDecode()
	ResetLowRange();
}

void BitCoder::FinishBlockEncode()
{
	SaveState();
}

void BitCoder::StartBlockDecode()
{
	bitsToGo = 0; // Note that this value differs from startEncode()
	ResetLowRange();

	LoadInitialBits();
}

void BitCoder::FinishBlockDecode()
{
	// nothing
}

void BitCoder::EncodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq)
{
	assert(cumFreq + freq <= totalFreq);
	assert(freq > 0);
	assert(totalFreq <= MAX_FREQ);

	uint32or64 range = (high - low) + 1;
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

void BitCoder::DecodeByte(uint32or64 cumFreq, uint32or64 freq, uint32or64 totalFreq)
{
	assert(cumFreq + freq <= totalFreq);
	assert(freq > 0);
	assert(totalFreq <= MAX_FREQ);

	uint32or64 range = (high - low) + 1;

	high = low + (range * (cumFreq + freq)) / totalFreq - 1;              /* ����� ���������� ������� */
	low = low + (range * cumFreq) / totalFreq;

	assert(high >= low);

	//logger.finer(()->String.format("low=%X high=%X nextCh=%X left=%X freq=%X", low, high, nextCh, left, freq));

	for (;;)                      //���� ���p�������� �����
	{
		if (high < HALF)         // �����p���� ������  ��������
		{
			/* ������ */
		}
		else if (low >= HALF)   // �����p���� ��p���� �������� ����� ��������� �������� Half
		{
			nextCh -= HALF;
			low    -= HALF;
			high   -= HALF;
		}
		else if ((low >= FIRST_QTR) && (high < THIRD_QTR)) /* �����p���� �p����� ��������   */
		{
			nextCh -= FIRST_QTR;
			low    -= FIRST_QTR;
			high   -= FIRST_QTR;
		}
		else break;

		low <<= 1;               /* ��������� ������� ����p����    */
		high = (high << 1) + 1;
		nextCh = (nextCh << 1) + inputBit();  /* �������� ����� ��� */

		assert(low < TOP);
		assert(high < TOP);
		assert(nextCh < TOP);

		//logger.finer(()->String.format("low=%X high=%X nextCh=%X", low, high, nextCh));
	}
}

uint32or64 BitCoder::GetCumFreq(uint32or64 totFreq)
{
	uint32or64 tmp = ((nextCh - low + 1) * totFreq - 1)/ (high - low + 1);
	if (tmp >= totFreq)
		throw bad_file_format("Input data corrupt."); // or force it to return a valid value :)
	return tmp;
}

void BitCoder::outputBitPlusFollow(int bit)
{
	outputBit(bit);
	while (bitsToFollow > 0)
	{
		outputBit((~bit) & 0x01);
		bitsToFollow--;
	}
}

// Called during Compression only.
void BitCoder::outputBit(int bit) // ������ ���� �� �������� � ��� ���������� ������������ � ��������.
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

int BitCoder::inputBit()
{
	if (bitsToGo == 0)
	{
		buffer = fin->get();
		bytesPassed++;

		if (buffer == EOF)
		{
			garbage_bits++;     // ��������� ����� ����� ����� ����� ����� � �p���p��� �� ������� ������� �� ����������
			assert(garbage_bits <= CODEBITS - 2); //, "Incorrect archive file.");
		}
		bitsToGo = 8;
	}

	int t = buffer & 1;  // ������ ���p������ ���� � �p����� ����� (���) ����p�
	buffer >>= 1;        // TODO ���������� ��� � AdaptHuffman
	bitsToGo--;
	return t;
}

void BitCoder::OutByte(uchar c)
{
	bytesPassed++;
	fout->put(c);

	//fputc(c, f);
	//logger.debug("Output byte: 0x%X count:%d", c, bytesPassed);
}