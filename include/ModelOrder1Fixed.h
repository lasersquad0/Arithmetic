#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ARIExceptions.h"

#define ALFABET_CNT 256

class ModelOrder1Fixed : public BasicModel
{
protected:
	uint32or64 cumFreqs[ALFABET_CNT + 1];
	uchar weights[ALFABET_CNT];
	uint32or64 summFreq;

public:
	ModelOrder1Fixed(IBlockCoder&cr, std::string fileName): BasicModel(cr)
	{
		summFreq = 0;

		std::ifstream fin(fileName, std::ios::in | std::ios::binary);
		if (fin.fail())
		{
			throw file_error("Cannot open file '" + fileName + "' for reading.");
		}
		
		memset(weights, 0, ALFABET_CNT);

		while (true)
		{
			uchar ch = (uchar)fin.get();
			if (fin.eof()) break;

			weights[ch]++;
			summFreq++;
		}
		
		cumFreqs[0] = 0;
		for (int i = 0; i < ALFABET_CNT; i++)
		{
			cumFreqs[i + 1] = cumFreqs[i] + weights[i];
		}

		fin.close();

		RescaleTo(coder.GetIntParam("MAX_FREQ"));
	}

	void EncodeSymbol(uchar*, uchar sym) override
	{
		//uint32or64 i = 0;
		//uint32or64 LowCount = 0;
		//uint32or64 symbol = *sym;

		//while (i < symbol)
		//	LowCount += weights[i++];

		
		coder.EncodeByte(cumFreqs[sym], weights[sym], summFreq);

		//UpdateStatistics(*sym);
	}

	uchar DecodeSymbol(uchar*) override
	{
		uchar sym = 1; 
		//uint32or64 HiCount = 0;
		uint32or64 count = coder.GetCumFreq(summFreq); // changes coder.range

		while (cumFreqs[sym] <= count) sym++;

		/*for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}
*/
		coder.DecodeByte(cumFreqs[sym-1], weights[sym-1], summFreq); //changes low, range and code

		//UpdateStatistics(sym);

		return sym;
	}

	void UpdateStatistics(uchar*, uchar) override
	{
		// nothing
	}

	void Rescale() // do not call Rescale() from anywhere. it is intended to be called from RescaleTo only!
	{
		summFreq = 0;
		for (int i = 0; i < ALFABET_CNT; i++)
			summFreq += (weights[i] -= (weights[i] >> 1));

	}

	void RescaleTo(uint32or64 limit)
	{
		if (limit > summFreq) return; // no scaling needed

		do
			Rescale();
		while (summFreq > limit);

		cumFreqs[0] = 0;
		for (int i = 0; i < ALFABET_CNT; i++)
		{
			cumFreqs[i + 1] = cumFreqs[i] + weights[i];
		}

	}

};


