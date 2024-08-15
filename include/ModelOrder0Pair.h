#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

//#define UCHAR_CNT 256

// This is ordinary order0 model, but it encodes pairs of symbols.
// Idea - speed optimization. Resulting encoded sequence will be the same as using ordinary order0 model. 
// Pair of symbols encoded one-by-one however number of calculations smaller because part of symbol1 calculation used to encode symbol2.

template<class UINT>
class ModelOrder0Pair : public BasicModel<UINT>
{
protected:
	static const UINT UCHAR_CNT = 256;
	UINT weights[UCHAR_CNT];
	UINT summFreq;
	uchar symbol1;
	bool haveSymbol;

	// 'old' EncodeSymbol() function used for encoding sinle symbols when rescaling going to happer in the middle of pair of symbols
	// or file contains odd number of symbols.
	void EncodeSymbol(uchar sym) 
	{ 
		UINT i = 0;
		UINT LowFreq = 0;

		while (i < sym)
			LowFreq += weights[i++];

		this->coder.EncodeByte(LowFreq, weights[i], summFreq);

		updateStatistics(sym);
	}

public:
	ModelOrder0Pair(IBlockCoder<UINT>& cr): BasicModel<UINT>(cr)
	{
		haveSymbol = false;
		symbol1 = 0;
		summFreq = 256; // sum of all default weights
		for (int i = 0; i < UCHAR_CNT; i++) weights[i] = static_cast<UINT>(1); // by default all weights are set to 1
	}

	void StopEncode() override
	{
		if (haveSymbol) // check if we have single "cached" symbol. If yes then encode it and then finish encoding
			EncodeSymbol((uchar)symbol1);

		BasicModel<UINT>::StopEncode();
	}

	void EncodeSymbol(uchar*, uchar sym) override
	//void EncodeSymbol(uchar* sym) 
	{
		if (!haveSymbol)
		{
			// sometime Rescale can happen between pair of symbols. Code below avoids this situation.
			if (summFreq == this->coder.GetIntParam("MAX_FREQ")) // Rescale going to happen after coding this first symbol. In this case we just encode single symbol (instead of pair) with rescale afterwards
			{
				EncodeSymbol(sym);
				return;
			}
			symbol1 = sym; // remember first symbol and exit, waiting for the second one
			haveSymbol = true;
			return;
		}

		//second symbol arrived, reset haveSymbol flag
		haveSymbol = false;

		UINT i = 0;
		UINT LowFreq1 = 0, LowFreq2 = 0;
		uchar symbol2 = sym;

		uchar minsym = valuemin(symbol1, symbol2);
		uchar maxsym = valuemax(symbol1, symbol2);

		while (i < minsym)
		{
			LowFreq1 += weights[i++];
		}

		LowFreq2 = LowFreq1;

		while (i < maxsym)
		{
			LowFreq2 += weights[i++];
		}

		if (symbol1 < symbol2)
		{
			this->coder.EncodeByte(LowFreq1, weights[symbol1], summFreq);
			this->coder.EncodeByte(LowFreq2 + 1, weights[symbol2], summFreq + 1);
		}
		else if(symbol1 > symbol2)
		{
			this->coder.EncodeByte(LowFreq2, weights[symbol1], summFreq);
			this->coder.EncodeByte(LowFreq1, weights[symbol2], summFreq + 1);
		}
		else // symbol1 == symbol2
		{
			this->coder.EncodeByte(LowFreq1, weights[symbol1], summFreq);
			this->coder.EncodeByte(LowFreq1, weights[symbol2] + 1, summFreq + 1);
		}

		updateStatistics(symbol1);
		updateStatistics(symbol2);
	}

	uchar DecodeSymbol(uchar*) override
	{
		uchar sym;
		UINT HiCount = 0;
		UINT count = this->coder.GetCumFreq(summFreq); // changes coder.range

		for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}

		this->coder.DecodeByte(HiCount - weights[sym], weights[sym], summFreq); //changes low, range and code

		updateStatistics(sym);

		return sym;
	}
	void updateStatistics(const uchar c)
	{
		weights[c]++;
		summFreq++;
		if (summFreq > this->coder.GetIntParam("MAX_FREQ")) Rescale();
	}

	void UpdateStatistics(uchar*, uchar) override
	{
		// nothing
	}

	void Rescale()
	{
		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));
	}

};

using ModelOrder0Pair32 = ModelOrder0Pair<uint32_t>;
using ModelOrder0Pair64 = ModelOrder0Pair<uint64_t>;