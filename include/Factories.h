#pragma once

//#include "ARIExceptions.h"
#include "FileRecord.h"
#include "ModelOrder0Fixed.h"
#include "ModelOrder0.h"
#include "ModelOrderN.h"
#include "ModelOrderMixed.h"
#include "ModelOrder0Pair.h"
#include "fpaq0.h"
#include "ModelOrder0Sort.h"
#include "RangeCoder.h"
#include "BitCoder.h"


class ModelCoderFactory
{
private:
	static inline IBlockCoder32* coders32[sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];
	static inline IBlockCoder64* coders64[sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];
	static inline IModel* models[sizeof(Parameters::ModelTypeCode) / sizeof(Parameters::ModelTypeCode[0])][sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];

	//friend class Constructor;
	//struct Constructor
	//{
	//	Constructor() { 
	//		memset(coders, 0, sizeof(coders)); 
	//		memset(coders, 0, sizeof(models)); }
	//	~Constructor() {
	//		for (auto cd : coders) delete cd; 
	//		for (auto m : models) delete m; }
	//};
	//Constructor cons;
public:
	static IModel* GetModelAndCoder(FileRecord& fr)
	{
		if (models[fr.modelOrder][fr.alg] != nullptr) return (models[fr.modelOrder][fr.alg]); // return cached model

		if (Parameters::IsCoder64bit[fr.alg])
		{
			IBlockCoder64& coder64 = GetCoder64((CoderType)fr.alg);

			switch ((ModelType)fr.modelOrder)
			{
			case ModelType::O0:    models[fr.modelOrder][fr.alg] = new ModelOrder064(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1:    models[fr.modelOrder][fr.alg] = new ModelOrder164(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O2:    models[fr.modelOrder][fr.alg] = new ModelOrder264(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3:    models[fr.modelOrder][fr.alg] = new ModelOrder364(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0FIX: models[fr.modelOrder][fr.alg] = new ModelOrder0Fixed64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0SORT:models[fr.modelOrder][fr.alg] = new ModelOrder0Sort64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0PAIR:models[fr.modelOrder][fr.alg] = new ModelOrder0Pair64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3MIX: models[fr.modelOrder][fr.alg] = new ModelOrderMixed64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1FPAQ: models[fr.modelOrder][fr.alg] = new fpaqBitCoder64(coder64); return models[fr.modelOrder][fr.alg]; break; // fpaqBitCoder model always works with ABITARI coder
			}
			throw std::invalid_argument("Invalid model type.");
		}
		else
		{
			IBlockCoder32& coder32 = GetCoder32((CoderType)fr.alg);

			switch ((ModelType)fr.modelOrder)
			{
			case ModelType::O0:    models[fr.modelOrder][fr.alg] = new ModelOrder032(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1:    models[fr.modelOrder][fr.alg] = new ModelOrder132(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O2:    models[fr.modelOrder][fr.alg] = new ModelOrder232(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3:    models[fr.modelOrder][fr.alg] = new ModelOrder332(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0FIX: models[fr.modelOrder][fr.alg] = new ModelOrder0Fixed32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0SORT:models[fr.modelOrder][fr.alg] = new ModelOrder0Sort32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0PAIR:models[fr.modelOrder][fr.alg] = new ModelOrder0Pair32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3MIX: models[fr.modelOrder][fr.alg] = new ModelOrderMixed32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1FPAQ:models[fr.modelOrder][fr.alg] = new fpaqBitCoder32(coder32); return models[fr.modelOrder][fr.alg]; break; // fpaqBitCoder model always works with ABITARI coder
			}
			throw std::invalid_argument("Invalid model type.");
		}
	}

	static IBlockCoder32& GetCoder32(CoderType ct)
	{
		uint8_t cindex = (uint8_t)ct;
		if (coders32[cindex] != nullptr) return *(coders32[cindex]); // return cached Coder

		switch (ct)
		{
		case CoderType::HUFFMAN:        throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AHUFFMAN:       throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::ARITHMETIC:
		case CoderType::ARITHMETIC32:  coders32[cindex] = new RangeCoder32(); return *(coders32[cindex]); break;
		case CoderType::BITARITHMETIC: coders32[cindex] = new BitCoder32();   return *(coders32[cindex]); break;
			//case CoderType::ABITARITHMETIC: coders[cindex] = new fpaqBitCoder(*(new RangeCoder()));   return *(coders[cindex]); break; // TODO dirty hack here!!!

		}
		throw std::invalid_argument("Invalid coder type.");
	}

	static IBlockCoder64& GetCoder64(CoderType ct)
	{
		uint8_t cindex = (uint8_t)ct;
		if (coders64[cindex] != nullptr) return *(coders64[cindex]); // return cached Coder

		switch (ct)
		{
		case CoderType::HUFFMAN:        throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AHUFFMAN:       throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::ARITHMETIC64:  coders64[cindex] = new RangeCoder64(); return *(coders64[cindex]); break;
		case CoderType::BITARITHMETIC: coders64[cindex] = new BitCoder64();   return *(coders64[cindex]); break;
		//case CoderType::ABITARITHMETIC: coders64[cindex] = new fpaqBitCoder(*(new RangeCoder()));   return *(coders[cindex]); break; // TODO dirty hack here!!!

		}
		throw std::invalid_argument("Invalid coder type.");
	}
};