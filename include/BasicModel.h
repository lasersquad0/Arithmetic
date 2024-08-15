#pragma once

#include "CommonFunctions.h"
#include "ICoder.h"

class IModel
{
public:
	virtual void EncodeSymbol(uchar* ctx, uchar sym) = 0;
	virtual void BeginEncode(std::ostream* fo, std::istream* fi = nullptr) = 0;
	virtual void StopEncode() = 0;
	virtual void BeginBlockEncode() = 0;
	virtual void StopBlockEncode() = 0;
	virtual uchar DecodeSymbol(uchar* ctx) = 0;
	virtual void BeginDecode(std::istream* f) = 0;
	virtual void StopDecode() = 0;
	virtual void BeginBlockDecode() = 0;
	virtual void StopBlockDecode() = 0;
	virtual uint64_t GetBytesPassed() = 0;
	virtual void UpdateStatistics(uchar* ctx, uchar sym) = 0;
};

template<class UINT>
class BasicModel : public IModel
{
protected:
	IBlockCoder<UINT>& coder;
	BasicModel(IBlockCoder<UINT>& cr) :coder(cr) { }
public:
	virtual UINT GetWeight(uchar*, uchar) { return 0; }

	IBlockCoder<UINT>& GetCoder(){ return coder; }

	uint64_t GetBytesPassed() override { return coder.GetBytesPassed(); };

	void BeginBlockEncode() override 
	{
		coder.StartBlockEncode();
	}

	void StopBlockEncode() override
	{
		coder.FinishBlockEncode();
	}
	
	void BeginBlockDecode() override
	{
		coder.StartBlockDecode();
	}

	void StopBlockDecode() override
	{
		coder.FinishBlockDecode();
	}
	void BeginEncode(std::ostream* f, std::istream*) override
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

