#pragma once

#include "Exceptions.h"
#include "FileRecord.h"
#include "ModelOrder1Fixed.h"
#include "ModelOrder1.h"
#include "ModelOrderN.h"
#include "ModelOO1.h"
#include "fpaq0.h"
#include "ModelSortOrder1.h"
#include "RangeCoder.h"
#include "BitCoder.h"


class ModelCoderFactory
{
private:
	static inline IBlockCoder* coders[sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];
	static inline IModel* models[sizeof(Parameters::ModelTypeCode) / sizeof(Parameters::ModelTypeCode[0])][sizeof(Parameters::CoderNames) / sizeof(Parameters::CoderNames[0])];

	friend class Constructor;
	struct Constructor
	{
		Constructor() { memset(coders, 0, sizeof(coders)); memset(coders, 0, sizeof(models)); }
		~Constructor() { for (auto cd : coders) delete cd; for (auto m : models) delete m; }
	};
	Constructor cons;
public:
	static IModel* GetModelAndCoder(FileRecord& fr)
	{
		if (models[fr.modelOrder][fr.alg] != nullptr) return (models[fr.modelOrder][fr.alg]); // return cached model

		IBlockCoder& coder = GetCoder((CoderType)fr.alg);
	
		switch ((ModelType)fr.modelOrder)
		{
		case ModelType::O1:    models[fr.modelOrder][fr.alg] = new ModelOrder1(coder); return models[fr.modelOrder][fr.alg]; break;
		case ModelType::O2:    models[fr.modelOrder][fr.alg] = new ModelOrderN<ModelOrder1, 2>(coder); return models[fr.modelOrder][fr.alg]; break;
		case ModelType::O3:    models[fr.modelOrder][fr.alg] = new ModelOrderN<ModelOrderN<ModelOrder1, 2>, 3>(coder); return models[fr.modelOrder][fr.alg]; break;
		case ModelType::O4:    models[fr.modelOrder][fr.alg] = new ModelOrderN<ModelOrderN<ModelOrderN<ModelOrder1, 2>, 3>, 4>(coder); return models[fr.modelOrder][fr.alg]; break;
		case ModelType::FO1:   models[fr.modelOrder][fr.alg] = new ModelOrder1Fixed(coder, fr.origFilename); return models[fr.modelOrder][fr.alg]; break;
		case ModelType::BITO1: models[fr.modelOrder][fr.alg] = new ModelSortOrder1(coder); return models[fr.modelOrder][fr.alg]; break; // fpaqBitCoder model always works with ABITARI coder
		}
		throw std::invalid_argument("Invalid model type.");

	}

	static IBlockCoder& GetCoder(CoderType ct)
	{
		uint8_t cindex = (uint8_t)ct;
		if (coders[cindex] != nullptr) return *(coders[cindex]); // return cached Coder

		switch (ct)
		{
		case CoderType::HUFFMAN:        throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AHUFFMAN:       throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::ARITHMETIC:     throw std::invalid_argument("Invalid coder type."); //return new RangeCoder(); break;
		case CoderType::AARITHMETIC:    coders[cindex] = new RangeCoder(); return *(coders[cindex]); break;
		case CoderType::ABITARITHMETIC: coders[cindex] = new BitCoder();   return *(coders[cindex]); break;
			//case CoderType::ABITARITHMETIC: coders[cindex] = new fpaqBitCoder(*(new RangeCoder()));   return *(coders[cindex]); break; // TODO dirty hack here!!!

		}
		throw std::invalid_argument("Invalid coder type.");

	}
};