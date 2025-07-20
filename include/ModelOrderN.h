#pragma once

#include <cassert>
#include "BasicModel.h"
#include "DynamicArrays.h"

template<class LowerModel, int Order>
class ModelOrderN : public BasicModel<typename LowerModel::uint_type>
{
private:
	THash<uchar, LowerModel*> models;
	bool zeroInit;
public:
	using uint_type = typename LowerModel::uint_type;

	ModelOrderN(IBlockCoder<uint_type>& cr, bool zInit = false) : BasicModel<uint_type>(cr)
	{
		zeroInit = zInit;
	}

	~ModelOrderN()
	{
		for (uint i = 0; i < models.Count(); i++)
			delete models.GetValues()[i];

		models.ClearMem();
	}

	// the same model can be used for encoding-decoding different files during one session. it needs to be reset to original state each time.
	void ResetModel()
	{
		for (uint i = 0; i < models.Count(); i++)
			models.GetValues()[i]->ResetModel();
	}

	void EncodeSymbol(uchar* ctx, uchar sym) override
	{
		assert(Order > 0);

		//TODO rewrite it using GetValuePointer??? will be faster
		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			models.GetValues()[ind]->EncodeSymbol(ctx, sym);
		}
		else
		{
			LowerModel* tmp = new LowerModel(this->coder, zeroInit);
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

		//TODO rewrite it using GetValuePointer??? will be faster
		int ind = models.GetKeys().IndexOf(ctx[Order - 1]);
		if (ind >= 0)
		{
			return models.GetValues()[ind]->DecodeSymbol(ctx);
		}
		else
		{
			LowerModel* tmp = new LowerModel(this->coder, zeroInit);
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

	uint_type GetWeight(uchar* ctx, uchar sym) override
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
			LowerModel* tmp = new LowerModel(this->coder, zeroInit);
			ind = models.GetKeys().AddValue(ctx[Order - 1]);
			models.GetValues().InsertValue(ind, tmp);
			tmp->UpdateStatistics(ctx, sym);
		}
	}

	void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) override
	{
		ResetModel(); // the same model can be used for encoding-decoding different files during one session. it needs to be reset to original state each time.
		BasicModel<uint_type>::BeginEncode(fo, fi);
	}


	void BeginDecode(std::istream* f) override
	{
		ResetModel();
		BasicModel<uint_type>::BeginDecode(f);
	}
};


using ModelOrder132 = ModelOrderN<ModelOrder032, 1>;
using ModelOrder164 = ModelOrderN<ModelOrder064, 1>;
using ModelOrder232 = ModelOrderN<ModelOrder132, 2>;
using ModelOrder264 = ModelOrderN<ModelOrder164, 2>;
using ModelOrder332 = ModelOrderN<ModelOrder232, 3>;
using ModelOrder364 = ModelOrderN<ModelOrder264, 3>;
