#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

class ModelOrder1: public BasicModel
{
protected:
	static const uint32or64 UCHAR_CNT = 256;
	uint32or64 weights[UCHAR_CNT];
	uint32or64 summFreq{ 0 };
public:
	ModelOrder1(IBlockCoder& cr):BasicModel(cr)
	{
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	void EncodeSymbol(uchar* sym) override
	{
		uint32or64 i = 0;
		uint32or64 LowFreq = 0;
		uint32or64 symbol = *sym;

		while(i < symbol) 
			LowFreq += weights[i++];

		coder.EncodeByte(LowFreq, weights[i], summFreq);

		updateStatistics(*sym);
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

		coder.DecodeByte(HiCount - weights[sym], weights[sym], summFreq); //меняет low, range and code

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
		//static unsigned int cnt = 0;
		//cnt++;
		//Parameters::logger.info("ModelOrder1.Rescale() called (%u).", cnt);

		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));
	}

};

