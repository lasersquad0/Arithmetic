#pragma once

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

//TODO when we compress several files - will cached models be reset for new files?
class ModelCoderFactory
{
private:
	static inline IBlockCoder32* coders32[(uint)CoderType::CodersCount];
	static inline IBlockCoder64* coders64[(uint)CoderType::CodersCount];
	static inline IModel* models[(uint)ModelType::ModelsCount][(uint)CoderType::CodersCount];
//	static inline IModel* models[sizeof(Parameters::ModelTypeCode) / sizeof(Parameters::ModelTypeCode[0])][sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];

	// this class is for proper freeing coders and models in static arrays above to avoid memory leaks
	friend class Freeman;
	class Freeman
	{
	public:
		Freeman()
		{ 
			memset(coders32, 0, sizeof(coders32[0])*(uint)(CoderType::CodersCount));
			memset(coders64, 0, sizeof(coders64[0]) * (uint)(CoderType::CodersCount));
			memset(models,   0, sizeof(models[0][0])* (uint)(CoderType::CodersCount)*(uint)(ModelType::ModelsCount));
		}
		~Freeman()
		{
			for (auto cd : coders32) delete cd;
			for (auto cd : coders64) delete cd;
			for (int i = 0; i < (uint)ModelType::ModelsCount;i++)
				for (int j = 0;j < (uint)CoderType::CodersCount; j++)
					delete models[i][j];
		}
	};
	static inline Freeman cons;
public:
	static IModel* GetModelAndCoder(FileRecord& fr)
	{
		if (models[fr.modelOrder][fr.alg] != nullptr) return (models[fr.modelOrder][fr.alg]); // return cached model

		if (Parameters::IsCoder64bit[fr.alg])
		{
			IBlockCoder64& coder64 = GetCoder64((CoderType)fr.alg);

			switch ((ModelType)fr.modelOrder)
			{
			case ModelType::O0:    models[fr.modelOrder][fr.alg] = new ModelOrder064(coder64);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1:    models[fr.modelOrder][fr.alg] = new ModelOrder164(coder64);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O2:    models[fr.modelOrder][fr.alg] = new ModelOrder264(coder64);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3:    models[fr.modelOrder][fr.alg] = new ModelOrder364(coder64);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0FIX: models[fr.modelOrder][fr.alg] = new ModelOrder0Fixed64(coder64);return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0SORT:models[fr.modelOrder][fr.alg] = new ModelOrder0Sort64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0PAIR:models[fr.modelOrder][fr.alg] = new ModelOrder0Pair64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3MIX: models[fr.modelOrder][fr.alg] = new ModelOrderMixed64(coder64); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1FPAQ:models[fr.modelOrder][fr.alg] = new fpaqBitCoder64(/*coder64*/);    return models[fr.modelOrder][fr.alg]; break; // fpaqBitCoder model always works with ABITARI coder

			default: throw std::invalid_argument("Invalid model type.");
			}
			//throw std::invalid_argument("Invalid model type.");
		}
		else
		{
			IBlockCoder32& coder32 = GetCoder32((CoderType)fr.alg);

			switch ((ModelType)fr.modelOrder)
			{
			case ModelType::O0:    models[fr.modelOrder][fr.alg] = new ModelOrder032(coder32);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1:    models[fr.modelOrder][fr.alg] = new ModelOrder132(coder32);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O2:    models[fr.modelOrder][fr.alg] = new ModelOrder232(coder32);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3:    models[fr.modelOrder][fr.alg] = new ModelOrder332(coder32);     return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0FIX: models[fr.modelOrder][fr.alg] = new ModelOrder0Fixed32(coder32);return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0SORT:models[fr.modelOrder][fr.alg] = new ModelOrder0Sort32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O0PAIR:models[fr.modelOrder][fr.alg] = new ModelOrder0Pair32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O3MIX: models[fr.modelOrder][fr.alg] = new ModelOrderMixed32(coder32); return models[fr.modelOrder][fr.alg]; break;
			case ModelType::O1FPAQ:models[fr.modelOrder][fr.alg] = new fpaqBitCoder32(/*coder32*/);    return models[fr.modelOrder][fr.alg]; break; // fpaqBitCoder model always works with ABITARI coder

			default: throw std::invalid_argument("Invalid model type.");
			}
			//throw std::invalid_argument("Invalid model type.");
		}
	}

	static IBlockCoder32& GetCoder32(CoderType ct)
	{
		uint8_t cindex = (uint8_t)ct;
		if (coders32[cindex] != nullptr) return *(coders32[cindex]); // return cached Coder

		switch (ct)
		{
		case CoderType::HUFFMAN:       throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AHUFFMAN:      throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::ARITHMETIC:
		case CoderType::ARITHMETIC32:  coders32[cindex] = new RangeCoder32(); return *(coders32[cindex]); break;
		case CoderType::BITARITHMETIC: coders32[cindex] = new BitCoder32();   return *(coders32[cindex]); break;
		case CoderType::FPAQARITHMETIC:coders32[cindex] = new fpaqBitCoder32(/* *(new RangeCoder32())*/); return *(coders32[cindex]); break; //TODO dirty hack here!!!

		default: throw std::invalid_argument("Invalid coder type.");
		}
		//throw std::invalid_argument("Invalid coder type.");
	}

	static IBlockCoder64& GetCoder64(CoderType ct)
	{
		uint8_t cindex = (uint8_t)ct;
		if (coders64[cindex] != nullptr) return *(coders64[cindex]); // return cached Coder

		switch (ct)
		{
		case CoderType::HUFFMAN:       throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AHUFFMAN:      throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::ARITHMETIC64:  coders64[cindex] = new RangeCoder64(); return *(coders64[cindex]); break;
		case CoderType::BITARITHMETIC: coders64[cindex] = new BitCoder64();   return *(coders64[cindex]); break;
		case CoderType::FPAQARITHMETIC:coders64[cindex] = new fpaqBitCoder64(/* *(new RangeCoder64())*/); return *(coders64[cindex]); break; //TODO dirty hack here!!!

		default: throw std::invalid_argument("Invalid coder type.");
		}
		//throw std::invalid_argument("Invalid coder type.");
	}
};