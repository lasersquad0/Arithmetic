#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

#define UCHAR_CNT 256

class ModelOrder1: public BasicModel
{
protected:
	uint32or64 weights[UCHAR_CNT];
	uint32or64 summFreq{ 0 };

public:
	ModelOrder1()//:coder(RangeCoder::GetCoder())
	{
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	//RangeCoder& GetCoder() override { return coder; }

	void EncodeSymbol(uchar* sym) override
	{
		uint32or64 i = 0;
		uint32or64 LowCount = 0;
		uint32or64 symbol = *sym;

		while(i < symbol) 
			LowCount += weights[i++];

		//for (i = 0; i < *sym; i++)
		//	LowCount += weights[i];

		coder.EncodeByte(LowCount, weights[i], summFreq);

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
		if (summFreq > coder.BOTTOM) Rescale();
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
/*
	uint32or64 getCount()
	{
		uint32or64 cnt = 0;
		for (int i = 0; i < UCHAR_CNT; i++)
		{
			if (weights[i] > 1) cnt++;
		}
		return cnt;
	}

	void Print(const uchar c)
	{
		for (int i = 0; i < UCHAR_CNT; i++)
		{
			if (weights[i] > 1) std::cout << " " << (uchar)i << c << "=" << weights[i] << ",";
		}
		std::cout << std::endl;
	}
	*/
};

