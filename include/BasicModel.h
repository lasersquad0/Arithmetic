#pragma once

#include "CommonFunctions.h"
#include "ICoder.h"

class IModel
{
public:
	virtual void EncodeSymbol(uchar* ctx, uchar sym) = 0;
	virtual void BeginEncode(std::ostream* f) = 0;
	virtual void StopEncode() = 0;
	virtual uchar DecodeSymbol(uchar* ctx) = 0;
	virtual void BeginDecode(std::istream* f) = 0;
	virtual void StopDecode() = 0;
	virtual uint32or64 GetWeight(uchar*, uchar) { return 0; }
	virtual void UpdateStatistics(uchar* ctx, uchar sym) = 0;
	virtual IBlockCoder& GetCoder() = 0;
};

class BasicModel : public IModel
{
protected:
	IBlockCoder& coder;
	BasicModel(IBlockCoder& cr) :coder(cr) { }
public:
	IBlockCoder& GetCoder() override { return coder; }

	void BeginEncode(std::ostream* f) override
	{
		coder.StartEncode(f);
	}

	void StopEncode() override
	{
		coder.FinishEncode();
	}

	void BeginDecode(std::istream* f) override
	{
		coder.StartDecode(f);
	}

	void StopDecode() override
	{
		coder.FinishDecode();
	}
};

