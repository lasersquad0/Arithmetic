#pragma once

#include <assert.h>
#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

class ModelOrder0: public BasicModel
{
protected:
	static const uint32or64 UCHAR_CNT = 256;
	uint32or64 weights[UCHAR_CNT];
	uint32or64 summFreq;
	bool zeroInit;
public:
	ModelOrder0(IBlockCoder& cr, bool zInit = false):BasicModel(cr)
	{
		zeroInit = zInit;
		ResetModel();
	}

	void ResetModel()
	{
		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++) 
			summFreq += (weights[i] = static_cast<uint32or64>(zeroInit?0:1));
		
			//summFreq = 0;
		//for (int i = 0; i < UCHAR_CNT; i++)
		//	summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	void EncodeSymbol(uchar* ctx, uchar sym) override
	{
		uint32or64 i = 0;
		uint32or64 LowFreq = 0;
		//uint32or64 symbol = *sym;

		while(i < sym)
			LowFreq += weights[i++];

		coder.EncodeByte(LowFreq, weights[i], summFreq);

		UpdateStatistics(ctx, sym);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		uchar sym;
		uint32or64 HiCount = 0;
		uint32or64 count = coder.GetCumFreq(summFreq); // changes coder.range

		for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}

		coder.DecodeByte(HiCount - weights[sym], weights[sym], summFreq); //changes low, range and code

		UpdateStatistics(ctx, sym);

		return sym;
	}

	uint32or64 GetWeight(uchar*, uchar sym) override
	{
		return weights[sym];
	}

	void UpdateStatistics(uchar*, uchar sym) override
	{
		//uchar c = *ctx;
		weights[sym]++;
		summFreq++;
		if (summFreq > coder.GetIntParam("MAX_FREQ")) Rescale();
	}

	void Rescale()
	{
		//static unsigned int cnt = 0;
		//cnt++;
		//ParametersG::logger.info("ModelOrder0.Rescale() called (%u).", cnt);

		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));
	}

	void BeginEncode(std::ostream* f) override
	{
		ResetModel(); // the same model can be used for encoding-decoding different files sduring one session. it need to be reset to original state each time.
		BasicModel::BeginEncode(f);
	}

	
	void BeginDecode(std::istream* f) override
	{
		ResetModel();
		BasicModel::BeginDecode(f);
	}

};

