#pragma once

#include "RangeCoder.h"
#include "fpaq0.h"
#include "BitCoder.h"

class CoderFactory
{
private:
	static inline IBlockCoder* coders[sizeof(Parameters::CoderNames)/sizeof(Parameters::CoderNames[0])];// std::end(Parameters::CoderNames) - std::begin(Parameters::CoderNames)];
	
	friend class Constructor;
	struct Constructor
	{
		Constructor() { memset(coders, 0, sizeof(coders)); }
		~Constructor(){ for (auto cd: coders) delete cd;   }
	};
	Constructor cons;
public:
	static IBlockCoder& GetCoder()
	{
		return GetCoder(Parameters::CODER_TYPE);
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
