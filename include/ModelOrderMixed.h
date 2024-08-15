#pragma once

#include <cassert>
#include "BasicModel.h"
#include "ModelOrder0.h"
#include "ModelOrderN.h"

template<class UINT>
class ModelOrderMixed : public BasicModel<UINT>
{
private:
	using ModelOrder1 = ModelOrderN<ModelOrder0<UINT>, 1>;
	using ModelOrder2 = ModelOrderN<ModelOrder1, 2>;
	using ModelOrder3 = ModelOrderN<ModelOrder2, 3>;

	ModelOrder0<UINT> model0; // init this model with '1' default freqs
	ModelOrder1 model1; // init this and others models with '0' default freqs
	ModelOrder2 model2;
	ModelOrder3 model3;
public:
	
	ModelOrderMixed(IBlockCoder<UINT>& cr) : BasicModel<UINT>(cr), model0(cr, false), model1(cr, true), model2(cr, true), model3(cr, true)
	{
	}

	void ResetModel()
	{
		model0.ResetModel();
		model1.ResetModel();
		model2.ResetModel();
		model3.ResetModel();
	}

	void EncodeSymbol(uchar* ctx, uchar sym) override
	{
		if (model3.GetWeight(ctx, sym) > 0)
		{
			model3.EncodeSymbol(ctx, sym);     // we can call EncodeSymbol at any model because all of them use single instance of coder.
			model0.UpdateStatistics(ctx, sym); // we need proper statistic in all models, so update it here in all models 
			model1.UpdateStatistics(ctx, sym);
			model2.UpdateStatistics(ctx, sym);
		}
		else if (model2.GetWeight(ctx, sym) > 0)
		{
			model2.EncodeSymbol(ctx, sym);
			model0.UpdateStatistics(ctx, sym);
			model1.UpdateStatistics(ctx, sym);
			model3.UpdateStatistics(ctx, sym);
		}
		else if (model1.GetWeight(ctx, sym) > 0)
		{
			model1.EncodeSymbol(ctx, sym);
			model0.UpdateStatistics(ctx, sym);
			model2.UpdateStatistics(ctx, sym);
			model3.UpdateStatistics(ctx, sym);
		}
		else
		{
			model0.EncodeSymbol(ctx, sym);
			model1.UpdateStatistics(ctx, sym);
			model2.UpdateStatistics(ctx, sym);
			model3.UpdateStatistics(ctx, sym);
		}

	}

	void UpdateStatistics(uchar*, uchar) override
	{
		//nothing
	}

	uchar DecodeSymbol(uchar*) override
	{
		return 0;
	}

	void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) override
	{
		ResetModel(); // the same model can be used for encoding-decoding different files during one session. it needs to be reset to original state each time.
		BasicModel<UINT>::BeginEncode(fo, fi);
	}


	void BeginDecode(std::istream* f) override
	{
		ResetModel();
		BasicModel<UINT>::BeginDecode(f);
	}
};

using ModelOrderMixed32 = ModelOrderMixed<uint32_t>;
using ModelOrderMixed64 = ModelOrderMixed<uint64_t>;
