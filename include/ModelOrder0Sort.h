#pragma once

#include "BasicModel.h"

// special End-Of-File symbol
//#define EOF_SYMBOL    (NO_OF_CHARS + 1)

template<class UINT>
class ModelOrder0Sort : public BasicModel<UINT>
{
protected:
	// number of alphabet symbols
	static const UINT NO_OF_CHARS = 256;
	// total number of symbols in model
	static const UINT NO_OF_SYMBOLS = NO_OF_CHARS + 1; // 257
	
	// transcoding tables
	uchar index_to_char[NO_OF_SYMBOLS]; //257
	UINT char_to_index[NO_OF_CHARS]; //256

	// frequencies tables
	UINT cum_freq[NO_OF_SYMBOLS + 1]; // 258 ??? why
	UINT     freq[NO_OF_SYMBOLS + 1]; // 258 ???? why

public:
	ModelOrder0Sort(IBlockCoder<UINT>& cr): BasicModel<UINT>(cr)
	{
		ResetModel();

		//for (int i = 0; i < UCHAR_CNT; i++)
		//	summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	void ResetModel()
	{
		int i;
		for (i = 0; i < NO_OF_CHARS; i++) // 256
		{
			char_to_index[i] = i + 1;
			index_to_char[i + 1] = (uchar)i; // TODO index_to_char[0] left uninitialised?
		}
		for (i = 0; i <= NO_OF_SYMBOLS; i++)  //258 (!)
		{
			freq[i] = 1;
			cum_freq[i] = NO_OF_SYMBOLS - i; // c_f[0]=257, c_f[1]=256 ... c_f[256]=1, c_f[257]=0
		}
		freq[0] = 0;
	}

	void EncodeSymbol(uchar*, uchar sym) override
	{
		UINT index = char_to_index[sym];

		this->coder.EncodeByte(cum_freq[index], freq[index], cum_freq[0]); // TODO shall instead of cum_freq[index] be something cum_freq[NO_OF_SYMBOLS- index] ? 

		updateStatistics(index);
	}

	uchar DecodeSymbol(uchar*) override
	{
		UINT ind;
		//uint32or64 HiCount = 0;
		UINT cum = this->coder.GetCumFreq(cum_freq[0]); // меняет coder.range

		// look for needed symbol in frequencies table 
		for (ind = 1; cum_freq[ind] > cum; ind++);

	/*	for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}*/

		uchar sym = index_to_char[ind];

		this->coder.DecodeByte(cum_freq[ind], freq[ind], cum_freq[0]); //recalculates low, range and code

		updateStatistics(ind);

		return sym;
	}


	void UpdateStatistics(uchar* , uchar) override
	{
		// nothing, see another updateStatistics() method
	}

	void updateStatistics(const UINT index)
	{
		//weights[c]++;
		//summFreq++;
		//if (summFreq > coder.GetIntParam("BOTTOM")) Rescale();

		uchar ch_i, ch_symbol;
		UINT cum;

		// check if freq counter needs scaling 
		if (cum_freq[0] == this->coder.GetIntParam("MAX_FREQ")) //MAX_FREQUENCY)
		{
			cum = 0;
			// scale frequencies 
			for (int i = NO_OF_SYMBOLS; i >= 0; i--)
			{
				freq[i] = (freq[i] + 1) / 2;
				cum_freq[i] = cum;
				cum += freq[i];
			}
		}

		UINT i; // int i; TODO may be introduced bug here.....
		for (i = index; freq[i] == freq[i - 1]; i--); // look back for index where two successive freqs differ from each other 

		if (i < index)
		{
			ch_i = index_to_char[i];
			ch_symbol = index_to_char[index];
			index_to_char[i] = ch_symbol;
			index_to_char[index] = ch_i;
			char_to_index[ch_i] = index;
			char_to_index[ch_symbol] = i;
		}

		// refresh values in freq tables
		freq[i]++; // now being encoded symbol has index i instead of index.
		while (i > 0)
		{
			i--;
			cum_freq[i]++;
		}
	}

	void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) override
	{
		//ResetModel(); // the same model can be used for encoding-decoding different files during one session. it need to be reset to original state each time.
		BasicModel<UINT>::BeginEncode(fo, fi);
	}

	void BeginDecode(std::istream* f) override
	{
		//ResetModel();
		BasicModel<UINT>::BeginDecode(f);
	}
};

using ModelOrder0Sort32 = ModelOrder0Sort<uint32_t>;
using ModelOrder0Sort64 = ModelOrder0Sort<uint64_t>;
