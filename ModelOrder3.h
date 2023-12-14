#pragma once

#include <string>
#include <iostream>
#include <map>
#include "BasicModel.h"
#include "ModelOrder2.h"
#include "DynamicArrays.h"

/*
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


class ModelOrder4 : public BasicModel
{
private:
	static const uint32or64 UCHAR_CNT = 256;
	ModelOrder3* models[UCHAR_CNT];
public:
	ModelOrder4(IBlockCoder& cr) : BasicModel(cr)
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

*/




class ModelOrder3: public BasicModel
{
private:
	//static const uint32or64 UCHAR_CNT = 256;
	//ModelOrder3* models[UCHAR_CNT];
	//std::map<uchar, ModelOrder3*> models;
	THash<uchar, ModelOrder2*> models;
public:
	ModelOrder3(IBlockCoder& cr): BasicModel(cr)
	{
		//for (size_t i = 0; i < UCHAR_CNT; i++)
		//{
		//	models[i] = new ModelOrder3(cr);
		//}
	}

	~ModelOrder3()
	{
		for (uint i = 0; i < models.Count(); i++)
			delete models.GetValues()->GetValue(i);

		models.ClearMem();
	}

	void EncodeSymbol(uchar* ctx) override
	{
		if (!models.IfExists(ctx[2]))
		{
			ModelOrder2* tmp = new ModelOrder2(coder);
			models.SetValue(ctx[2], tmp);
			tmp->EncodeSymbol(ctx);
		}
		else
			models.GetValue(ctx[2])->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		if (!models.IfExists(ctx[1]))
		{
			ModelOrder2* tmp = new ModelOrder2(coder);
			models.SetValue(ctx[1], tmp);
			return tmp->DecodeSymbol(ctx);
		}

		return models.GetValue(ctx[1])->DecodeSymbol(ctx);
	}
};

class ModelOrder4: public BasicModel
{
private:
	//static const uint32or64 UCHAR_CNT = 256;
	//ModelOrder3* models[UCHAR_CNT];
	//std::map<uchar, ModelOrder3*> models;
	THash<uchar, ModelOrder3*> models;
public:
	ModelOrder4(IBlockCoder& cr): BasicModel(cr)
	{
		//for (size_t i = 0; i < UCHAR_CNT; i++)
		//{
		//	models[i] = new ModelOrder3(cr);
		//}
	}

	~ModelOrder4()
	{
		for (uint i = 0; i < models.Count(); i++)
			delete models.GetValues()->GetValue(i);

		models.ClearMem();
	}

	void EncodeSymbol(uchar* ctx) override
	{
		if (!models.IfExists(ctx[3]))
		{
			ModelOrder3* tmp = new ModelOrder3(coder);
			models.SetValue(ctx[3], tmp);
			tmp->EncodeSymbol(ctx);
		}
		else
			models.GetValue(ctx[3])->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		if (!models.IfExists(ctx[2]))
		{
			ModelOrder3* tmp = new ModelOrder3(coder);
			models.SetValue(ctx[2], tmp);
			return tmp->DecodeSymbol(ctx);
		}
		
		return models.GetValue(ctx[2])->DecodeSymbol(ctx);
	}

};

