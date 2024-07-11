#pragma once

#include <cassert>
#include "BasicModel.h"
#include "ModelOrder0.h"
#include "ModelOrderN.h"
//#include "DynamicArrays.h"

class ModelOrderMixed : public BasicModel
{
private:
	ModelOrder0 model0; // init this model with '1' default freqs
	ModelOrder1 model1; // init this and others models with '0' default freqs
	ModelOrder2 model2;
	ModelOrder3 model3;
public:
	
	ModelOrderMixed(IBlockCoder& cr) : BasicModel(cr), model0(cr, false), model1(cr, true), model2(cr, true), model3(cr, true)
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

	uchar DecodeSymbol(uchar* ctx) override
	{
		return 0;
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
