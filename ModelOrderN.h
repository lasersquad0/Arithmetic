#pragma once

#include <cassert>
#include "BasicModel.h"
#include "DynamicArrays.h"

template<class LowerModel, int Order>
class ModelOrderN : public BasicModel
{
private:
	THash<uchar, LowerModel*> models;
public:
	ModelOrderN(IBlockCoder& cr) : BasicModel(cr)
	{
	}

	~ModelOrderN()
	{
		for (uint i = 0; i < models.Count(); i++)
			delete models.GetValues()->GetValue(i);

		models.ClearMem();
	}

	void ResetModel()
	{
		for (uint i = 0; i < models.Count(); i++)
			models.GetValues()->GetValue(i)->ResetModel();
	}

	void EncodeSymbol(uchar* ctx) override
	{
		assert(Order > 0);

		int ind = models.GetKeys()->IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			models.GetValues()->GetValue(ind)->EncodeSymbol(ctx);
		}
		else
		{
			LowerModel* tmp = new LowerModel(coder);
			ind = models.GetKeys()->AddValue(ctx[Order - 1]);
			models.GetValues()->InsertValue(ind, tmp);
			tmp->EncodeSymbol(ctx);
		}

		/*if (!models.IfExists(ctx[Order-1]))
		{
			LowerModel* tmp = new LowerModel(coder);
			models.SetValue(ctx[Order-1], tmp);
			tmp->EncodeSymbol(ctx);
		}
		else
			models.GetValue(ctx[Order-1])->EncodeSymbol(ctx);*/
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		assert(Order > 1);

		int ind = models.GetKeys()->IndexOf(ctx[Order - 2]);
		if (ind >= 0)
		{
			return models.GetValues()->GetValue(ind)->DecodeSymbol(ctx);
		}
		else
		{
			LowerModel* tmp = new LowerModel(coder);
			ind = models.GetKeys()->AddValue(ctx[Order - 2]);
			models.GetValues()->InsertValue(ind, tmp);
			return tmp->DecodeSymbol(ctx);
		}

		/*if (!models.IfExists(ctx[Order-2]))
		{
			LowerModel* tmp = new LowerModel(coder);
			models.SetValue(ctx[Order-2], tmp);
			return tmp->DecodeSymbol(ctx);
		}

		return models.GetValue(ctx[Order-2])->DecodeSymbol(ctx);*/
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