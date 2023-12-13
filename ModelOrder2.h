#pragma once

#include <string>
#include <iostream>
#include "BasicModel.h"
#include "ModelOrder1.h"

class ModelOrder2 : public BasicModel
{
private:
	static const uint32or64 UCHAR_CNT = 256;
	ModelOrder1* models[UCHAR_CNT];
public:
	ModelOrder2(IBlockCoder& cr):BasicModel(cr)
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			models[i] = new ModelOrder1(cr);
		}
	}

	~ModelOrder2()
	{
		for (size_t i = 0; i < UCHAR_CNT; i++)
		{
			delete models[i];
			models[i] = nullptr;
		}
	}

	void EncodeSymbol(uchar* ctx) override
	{
		models[ctx[1]]->EncodeSymbol(ctx);
	}

	uchar DecodeSymbol(uchar* ctx) override
	{
		return models[ctx[0]]->DecodeSymbol(ctx); // ����� 0 ������ 1 ��� � Encode, ������ ��� ��� ������������� ���������� �������������� ������ � ��� ��� ctx[0]
	}

};
