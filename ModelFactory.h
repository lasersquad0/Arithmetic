#pragma once

#include "Exceptions.h"
#include "ModelOrder1Fixed.h"
#include "ModelOrder1.h"
#include "ModelOrder2.h"
#include "ModelOrder3.h"
#include "ModelOO1.h"
#include "fpaq0.h"
#include "ModelBitOrder1.h"
#include "Parameters.h"
#include "CoderFactory.h"

class ModelsFactory
{
private:

public:
	static IModel* GetModel()
	{
		return GetModel(Parameters::MODEL_TYPE, Parameters::CODER_TYPE);
	}

	static IModel* GetModel(ModelType mo, CoderType ct, std::string fileName = "")
	{
		switch (mo)
		{
		case ModelType::O1:    return new ModelOrder1(CoderFactory::GetCoder(ct)); break;
		case ModelType::O2:    return new ModelOrder2(CoderFactory::GetCoder(ct)); break;
		case ModelType::O3:    return new ModelOrder3(CoderFactory::GetCoder(ct)); break;
		case ModelType::O4:    return new ModelOO1(CoderFactory::GetCoder(ct)); break;
		case ModelType::FO1:   return new ModelOrder1Fixed(CoderFactory::GetCoder(ct), fileName); break;
		case ModelType::BITO1: return new ModelBitOrder1(CoderFactory::GetCoder(CoderType::ABITARITHMETIC)); break; // fpaqBitCoder model always works with ABITARI coder
		}
		throw std::invalid_argument("Invalid model type.");

	}
};

