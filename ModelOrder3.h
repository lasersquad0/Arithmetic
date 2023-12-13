#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ModelOrder2.h"


class ModelOrder3: public BasicModel
{
private:
	static const uint32or64 UCHAR_CNT = 256;
	ModelOrder2* models[UCHAR_CNT];
public:
	ModelOrder3(IBlockCoder& cr): BasicModel(cr)
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			models[i] = new ModelOrder2(cr);
		}
	}

	~ModelOrder3()
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			delete models[i];
			models[i] = nullptr;
		}
	}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[2]]->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[1]]->DecodeSymbol(ctx);
	}

};


class ModelOrder4: public BasicModel
{
private:
	static const uint32or64 UCHAR_CNT = 256;
	ModelOrder3* models[UCHAR_CNT];
public:
	ModelOrder4(IBlockCoder& cr): BasicModel(cr)
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			models[i] = new ModelOrder3(cr);
		}
	}

	~ModelOrder4()
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			delete models[i];
			models[i] = nullptr;
		}
	}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[3]]->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[2]]->DecodeSymbol(ctx);
	}

};
