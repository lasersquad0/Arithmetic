#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ARIExceptions.h"

#define ALFABET_CNT 256

// Fixed model uses fixed statictics of char frequencies from a file
// statistic is not updated during compressing

template<class UINT>
class ModelOrder0Fixed : public BasicModel<UINT>
{
protected:
	UINT cumFreqs[ALFABET_CNT + 1];
	UINT weights[ALFABET_CNT]; 
	UINT summFreq;
	
	// changes only weights[], cumFreq remains unchanged and needs to be recalculated separately.
	void Rescale() // do not call Rescale() from anywhere. it is intended to be called from RescaleTo only!
	{
		summFreq = 0;
		for (int i = 0; i < ALFABET_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));
	}

public:
	ModelOrder0Fixed(IBlockCoder<UINT>&cr): BasicModel<UINT>(cr)
	{		
	}

	void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) override
	{
		//memset(weights, 0, ALFABET_CNT*sizeof(weights[0]));
		for (uint i = 0; i < ALFABET_CNT; i++) weights[i] = 1; // weights[i] cannot be zero

		summFreq = ALFABET_CNT; // default sum of weights
		while (true)
		{
			uchar ch = (uchar)fi->get();
			if (fi->eof()) break;

			weights[ch]++;
			summFreq++;
		}

		cumFreqs[0] = 0;
		for (uint i = 0; i < ALFABET_CNT; i++)
		{
			cumFreqs[i + 1] = cumFreqs[i] + weights[i];
		}

		RescaleTo(this->coder.GetIntParam("MAX_FREQ"));
		
		// when we called fi->get() at the end of file two bit raised: failbit and eofbit. 
		// need to clear them before moving reading point to beginning
		fi->clear();
		fi->seekg(0, std::ios_base::beg); // moving file pointer back to the beginning
		
		// saving frequency table into archive file
		fo->write((const char*)weights, ALFABET_CNT * sizeof(weights[0]));
		
		this->coder.StartEncode(fo);
	}

	void BeginDecode(std::istream* f) override
	{
		// loading frequency table from archive file
		f->read((char*)weights, ALFABET_CNT * sizeof(weights[0]));

		cumFreqs[0] = 0;
		for (uint i = 0; i < ALFABET_CNT; i++)
		{
			cumFreqs[i + 1] = cumFreqs[i] + weights[i];
		}
		summFreq = cumFreqs[ALFABET_CNT];

		this->coder.StartDecode(f);
	}

	void EncodeSymbol(uchar*, uchar sym) override
	{
		//uint32or64 i = 0;
		//uint32or64 LowCount = 0;
		//uint32or64 symbol = *sym;

		//while (i < symbol)
		//	LowCount += weights[i++];


		this->coder.EncodeByte(cumFreqs[sym], weights[sym], summFreq);

		//UpdateStatistics(*sym);
	}

	uchar DecodeSymbol(uchar*) override
	{
		uint sym = 1; // should be uint, do not change to uchar
		//uint32or64 HiCount = 0;
		UINT count = this->coder.GetCumFreq(summFreq); // changes coder.range

		while (cumFreqs[sym] <= count) sym++; //TODO what about binary search here?

		/*for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}
*/
		this->coder.DecodeByte(cumFreqs[sym-1], weights[sym-1], summFreq); //changes low, range and code

		//UpdateStatistics(sym);

		return (uchar)(sym - 1);
	}

	void UpdateStatistics(uchar*, uchar) override
	{
		// nothing
	}

	void RescaleTo(UINT limit)
	{
		if (limit > summFreq) return; // no scaling needed

		do
			Rescale(); // changes only weights[], cumFreq[] remains unchanged and needs to be recalculated separately (see below).
		while (summFreq > limit);

		cumFreqs[0] = 0;
		for (int i = 0; i < ALFABET_CNT; i++)
			cumFreqs[i + 1] = cumFreqs[i] + weights[i];
	}
};

using ModelOrder0Fixed32 = ModelOrder0Fixed<uint32_t>;
using ModelOrder0Fixed64 = ModelOrder0Fixed<uint64_t>;
