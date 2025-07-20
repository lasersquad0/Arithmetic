#pragma once

#include <assert.h>
#include <string>
#include <iostream>
#include "BasicModel.h"
#include "Parameters.h"

// Model order0 uses frequencies of each symbol independently to other symbols
// frequencies are not fixed and collected as compression progresses
template<class UINT>
class ModelOrder0: public BasicModel<UINT>
{
protected:
	static const UINT UCHAR_CNT = 256;
	UINT weights[UCHAR_CNT];
	UINT summFreq;
	bool zeroInit;
public:
	using uint_type = UINT;

	ModelOrder0(IBlockCoder<UINT>& cr, bool zInit = false): BasicModel<UINT>(cr)
	{
		zeroInit = zInit;
		ResetModel();
	}

	void ResetModel()
	{
		summFreq = 0;
		for (int i = 0; i < UCHAR_CNT; i++) 
			summFreq += (weights[i] = static_cast<UINT>(zeroInit?0:1));
		
		//summFreq = 0;
		//for (int i = 0; i < UCHAR_CNT; i++)
		//	summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	void EncodeSymbol(uchar* ctx, uchar sym) override
	{
		uchar i = 0;
		UINT LowFreq = 0;
		//uint32or64 symbol = *sym;

		while(i < sym)
			LowFreq += weights[i++];

		this->coder.EncodeByte(LowFreq, weights[i], summFreq);

		UpdateStatistics(ctx, sym);
	}

	uchar DecodeSymbol(uchar* ctx) override
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

		UpdateStatistics(ctx, sym);

		return sym;
	}

	UINT GetWeight(uchar*, uchar sym) override
	{
		return weights[sym];
	}

	void UpdateStatistics(uchar*, uchar sym) override
	{
		//uchar c = *ctx;
		weights[sym]++;
		summFreq++;
		if (summFreq > this->coder.GetIntParam("MAX_FREQ")) Rescale();
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

	void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) override
	{
		ResetModel(); // the same model can be used for encoding-decoding different files sduring one session. it need to be reset to original state each time.
		BasicModel<UINT>::BeginEncode(fo, fi);
	}
	
	void BeginDecode(std::istream* f) override
	{
		ResetModel();
		BasicModel<UINT>::BeginDecode(f);
	}
};

typedef ModelOrder0<uint32_t> ModelOrder032;
typedef ModelOrder0<uint64_t> ModelOrder064;