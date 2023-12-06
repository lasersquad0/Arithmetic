#pragma once

#include "coder.h"


class IModel
{
public:
	virtual void EncodeSymbol(uchar* sym) = 0;
	virtual void StartEncode(std::ostream* f) = 0;
	virtual void StopEncode() = 0;
	virtual uchar DecodeSymbol(uchar* ctx) = 0;
	virtual void StartDecode(std::istream* f) = 0;
	virtual void StopDecode() = 0;
	virtual RangeCoder& GetCoder() = 0;
};

class BasicModel : public IModel
{
protected:
	RangeCoder& coder;
public:
	BasicModel() :coder(RangeCoder::GetCoder())
	{

	}

	RangeCoder& GetCoder() override { return coder; }

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
};

