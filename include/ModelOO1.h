#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

//#define UCHAR_CNT 256

class ModelOO1 : public BasicModel
{
protected:
	static const uint32or64 UCHAR_CNT = 256;
	uint32or64 weights[UCHAR_CNT];
	uint32or64 summFreq;
	uchar symbol1;
	bool haveSymbol;

	void EncodeSymbol(uchar sym)
	{ 
		uint32or64 i = 0;
		uint32or64 LowFreq = 0;

		while (i < sym)
			LowFreq += weights[i++];

		coder.EncodeByte(LowFreq, weights[i], summFreq);

		updateStatistics(sym);
	}


public:
	ModelOO1(IBlockCoder& cr) :BasicModel(cr)
	{
		haveSymbol = false;
		symbol1 = 0;
		summFreq = 256;
		for (int i = 0; i < UCHAR_CNT; i++) weights[i] = static_cast<uint32or64>(1); // by default all weights are set to 1
	}

	void StopEncode() override
	{
		if (haveSymbol) // check if we have single "cached" symbol. If yes then encode it and then finish encoding
			EncodeSymbol((uchar)symbol1);

		BasicModel::StopEncode();
	}

	void EncodeSymbol(uchar* sym)
	{
		if (!haveSymbol)
		{
			// sometime Rescale can happer between pair of symbols. Code below avoids this situation.
			if (summFreq == coder.GetIntParam("MAX_FREQ")) // Rescale going to happen after coding this first symbol. In this case we just encode single symbol (instead of pair) with rescale afterwards
			{
				EncodeSymbol(*sym);
				return;
			}
			symbol1 = *sym; //remember first symbol and exit, waiting for the second one
			haveSymbol = true;
			return;
		}

		//second symbol arrived, reset haveSymbol flag
		haveSymbol = false;

		uint32or64 i = 0;
		uint32or64 LowFreq1 = 0, LowFreq2 = 0;
		uchar symbol2 = *sym;


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
			coder.EncodeByte(LowFreq1, weights[symbol1], summFreq);
			coder.EncodeByte(LowFreq2 + 1, weights[symbol2], summFreq + 1);
		}
		else if(symbol1 > symbol2)
		{
			coder.EncodeByte(LowFreq2, weights[symbol1], summFreq);
			coder.EncodeByte(LowFreq1, weights[symbol2], summFreq + 1);
		}
		else // symbol1 == symbol2
		{
			coder.EncodeByte(LowFreq1, weights[symbol1], summFreq);
			coder.EncodeByte(LowFreq1, weights[symbol2] + 1, summFreq + 1);
		}

		updateStatistics(symbol1);
		updateStatistics(symbol2);
	}

	uchar DecodeSymbol(uchar*) override
	{
		uchar sym;
		uint32or64 HiCount = 0;
		uint32or64 count = coder.GetCumFreq(summFreq); // меняет coder.range

		for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}

		coder.DecodeByte(HiCount - weights[sym], weights[sym], summFreq); //пересчитывает low, range and code

		updateStatistics(sym);

		return sym;
	}
	void updateStatistics(const uchar c)
	{
		weights[c]++;
		summFreq++;
		if (summFreq > coder.GetIntParam("MAX_FREQ")) Rescale();
	}

	void Rescale()
	{
		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));
	}

};

