#pragma once

#include "BasicModel.h"

// Специальный символ КонецФайла
//#define EOF_SYMBOL    (NO_OF_CHARS + 1)


class ModelSortOrder1 : public BasicModel
{
protected:
	// Количество символов алфавита
	static const uint32or64 NO_OF_CHARS = 256;
	// Всего символов в модели
	static const uint32or64 NO_OF_SYMBOLS = NO_OF_CHARS + 1; // 257
	
	// Таблицы перекодировки
	uchar index_to_char[NO_OF_SYMBOLS]; //257
	uint32or64 char_to_index[NO_OF_CHARS]; //256

	// Таблицы частот
	uint32or64 cum_freq[NO_OF_SYMBOLS + 1]; // 258 ??? why
	uint32or64     freq[NO_OF_SYMBOLS + 1]; // 258 ???? why

public:
	ModelSortOrder1(IBlockCoder& cr) :BasicModel(cr)
	{
		ResetModel();

		//for (int i = 0; i < UCHAR_CNT; i++)
		//	summFreq += (weights[i] = static_cast<uint32or64>(1)); // by default all weights are set to 1
	}

	void ResetModel()
	{
		int i;
		for (i = 0; i < NO_OF_CHARS; i++)
		{
			char_to_index[i] = i + 1;
			index_to_char[i + 1] = (uchar)i; // TODO index_to_char[0] left uninitialised?
		}
		for (i = 0; i <= NO_OF_SYMBOLS; i++)
		{
			freq[i] = 1;
			cum_freq[i] = NO_OF_SYMBOLS - i; // c_f[0]=257, c_f[1]=256 ... c_f[256]=1, c_f[257]=0
		}
		freq[0] = 0;
	}

	void EncodeSymbol(uchar* sym) override
	{
		uint32or64 index = char_to_index[*sym];

		coder.EncodeByte(cum_freq[index], freq[index], cum_freq[0]);

		updateStatistics(index);
	}

	uchar DecodeSymbol(uchar*) override
	{
		uint32or64 ind;
		//uint32or64 HiCount = 0;
		uint32or64 cum = coder.GetCumFreq(cum_freq[0]); // меняет coder.range

		// поиск соответствующего символа в таблице частот
		for (ind = 1; cum_freq[ind] > cum; ind++);

	/*	for (sym = 0; ; sym++)
		{
			HiCount += weights[sym];
			if (HiCount > count) break;
		}*/

		uchar sym = index_to_char[ind];

		coder.DecodeByte(cum_freq[ind], freq[ind], cum_freq[0]); //пересчитывает low, range and code

		updateStatistics(ind);

		return sym;
	}


	void updateStatistics(const uint32or64 index)
	{
		//weights[c]++;
		//summFreq++;
		//if (summFreq > coder.GetIntParam("BOTTOM")) Rescale();

		int i;
		uchar ch_i, ch_symbol;
		uint32or64 cum;

		// проверка на переполнение счетчика частоты
		if (cum_freq[0] == coder.GetIntParam("MAX_FREQ")) //MAX_FREQUENCY)
		{
			cum = 0;
			// масштабирование частот при переполнении
			for (i = NO_OF_SYMBOLS; i >= 0; i--)
			{
				freq[i] = (freq[i] + 1) / 2;
				cum_freq[i] = cum;
				cum += freq[i];
			}
		}

		for (i = index; freq[i] == freq[i - 1]; i--); // ищем назад номер где два рядом стояших freq не равны друг другу

		if (i < index)
		{
			ch_i = index_to_char[i];
			ch_symbol = index_to_char[index];
			index_to_char[i] = ch_symbol;
			index_to_char[index] = ch_i;
			char_to_index[ch_i] = index;
			char_to_index[ch_symbol] = i;
		}

		// обновление значений в таблицах частот
		freq[i]++;
		while (i > 0)
		{
			i--;
			cum_freq[i]++;
		}
	}

	void BeginEncode(std::ostream* f) override
	{
		//ResetModel(); // the same model can be used for encoding-decoding different files sduring one session. it need to be reset to original state each time.
		BasicModel::BeginEncode(f);
	}


	void BeginDecode(std::istream* f) override
	{
		//ResetModel();
		BasicModel::BeginDecode(f);
	}

};


