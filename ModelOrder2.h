#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ModelOrder1.h"

class ModelOrder2: public BasicModel
{
private:
	ModelOrder1 models[UCHAR_CNT];
	//RangeCoder& coder;
public:
	ModelOrder2()
	{
	}

	
	//void updateStatistics(uchar* ctx)
	//{
	//	models[ctx[1]].updateStatistics(ctx[0]); 
	//}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[1]].EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[0]].DecodeSymbol(ctx); // здесь 0 вместо 1 как в Encode, потому что при декодировании предыдущий декодированный символ у нас это ctx[0]
	}
	/*
	void StartEncode(std::ostream* f) override
	{
		coder.StartEncode(f);
	}

	void StopEncode() override
	{
		coder.FinishEncode();
	}

	void StartDecode(std::istream* f) override
	{
		coder.StartDecode(f);
	}

	void StopDecode() override 
	{
		coder.FinishDecode();
	}
	*/

	//uint32or64 getCount()
	//{
	//	uint32or64 cnt = 0;
	//	for (int i = 0; i < UCHAR_CNT; i++)
	//	{
	//		cnt += m_Freqs[i].getCount();
	//	}
	//	return cnt;
	//}

	//void Print()
	//{
	//	for (int i = 0; i < UCHAR_CNT; i++)
	//	{
	//		m_Freqs[i].Print((uchar)i);
	//	}
	//}
};


