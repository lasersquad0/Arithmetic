#pragma once

#include <cassert>
#include "BasicModel.h"
#include "DynamicArrays.h"

template<class LowerModel, int Order>
class ModelOrderN : public BasicModel
{
private:
	THash<uchar, LowerModel*> models;
	bool zeroInit;
public:
	ModelOrderN(IBlockCoder& cr, bool zInit = false) : BasicModel(cr)
	{
		zeroInit = zInit;
	}

	~ModelOrderN()
	{
		for (uint i = 0; i < models.Count(); i++)
			delete models.GetValues()[i];

		models.ClearMem();
	}

	void ResetModel()
	{
		for (uint i = 0; i < models.Count(); i++)
			models.GetValues()[i]->ResetModel();
	}

	void EncodeSymbol(uchar* ctx, uchar sym) override
	{
		assert(Order > 0);

		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			models.GetValues()[ind]->EncodeSymbol(ctx, sym);
		}
		else
		{
			LowerModel* tmp = new LowerModel(coder, zeroInit);
			ind = models.GetKeys().AddValue(ctx[Order - 1]);
			models.GetValues().InsertValue(ind, tmp);
			tmp->EncodeSymbol(ctx, sym);
		}

		/*if (!models.IfExists(ctx[Order]))
		{
			LowerModel* tmp = new LowerModel(coder);
			models.SetValue(ctx[Order], tmp);
			tmp->EncodeSymbol(ctx);
		}
		else
			models.GetValue(ctx[Order])->EncodeSymbol(ctx);*/
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		assert(Order > 0);

		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			return models.GetValues()[ind]->DecodeSymbol(ctx);
		}
		else
		{
			LowerModel* tmp = new LowerModel(coder, zeroInit);
			ind = models.GetKeys().AddValue(ctx[Order - 1]);
			models.GetValues().InsertValue(ind, tmp);
			return tmp->DecodeSymbol(ctx);
		}

		/*if (!models.IfExists(ctx[Order-1]))
		{
			LowerModel* tmp = new LowerModel(coder);
			models.SetValue(ctx[Order-2], tmp);
			return tmp->DecodeSymbol(ctx);
		}

		return models.GetValue(ctx[Order-1])->DecodeSymbol(ctx);*/
	}

	uint32or64 GetWeight(uchar* ctx, uchar sym) override
	{
		assert(Order > 0);

		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			return models.GetValues()[ind]->GetWeight(ctx, sym);
		}
		else
		{
			return 0;
		}

	}

	void UpdateStatistics(uchar* ctx, uchar sym) override
	{
		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			models.GetValues()[ind]->UpdateStatistics(ctx, sym);
		}
		else
		{
			LowerModel* tmp = new LowerModel(coder, zeroInit);
			ind = models.GetKeys().AddValue(ctx[Order - 1]);
			models.GetValues().InsertValue(ind, tmp);
			tmp->UpdateStatistics(ctx, sym);
		}
	}

	void BeginEncode(std::ostream* f) override
	{
		ResetModel(); // the same model can be used for encoding-decoding different files during one session. it needs to be reset to original state each time.
		BasicModel::BeginEncode(f);
	}


	void BeginDecode(std::istream* f) override
	{
		ResetModel();
		BasicModel::BeginDecode(f);
	}
};


using ModelOrder1 = ModelOrderN<ModelOrder0, 1>;
using ModelOrder2 = ModelOrderN<ModelOrder1, 2>;
using ModelOrder3 = ModelOrderN<ModelOrder2, 3>;
