#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ModelOrder2.h"


class ModelOrder3: public BasicModel
{
private:
	ModelOrder2* models[UCHAR_CNT];
	//RangeCoder& coder;
public:
	ModelOrder3()//: coder(RangeCoder::GetCoder())
	{
		//coder = RangeCoder::GetCoder();
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			models[i] = new ModelOrder2();
		}
	}

	~ModelOrder3()
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			delete models[i];
		}
	}

	//void updateStatistics(uchar* ctx)
	//{
	//	models[ctx[1]]->updateStatistics(ctx);
	//}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[2]]->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[1]]->DecodeSymbol(ctx);
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
	}*/

};


class ModelOrder4: public BasicModel
{
private:
	ModelOrder3* models[UCHAR_CNT];
//	RangeCoder& coder;
public:
	ModelOrder4()//: coder(RangeCoder::GetCoder())
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			models[i] = new ModelOrder3();
		}
	}

	~ModelOrder4()
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			delete models[i];
		}
	}

	//void updateStatistics(uchar* ctx)
	//{
	//	models[ctx[2]]->updateStatistics(ctx); // или может с0 и с1 поменять местами?
	//}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[3]]->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[2]]->DecodeSymbol(ctx);
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
};
