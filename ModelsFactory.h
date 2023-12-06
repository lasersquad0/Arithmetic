#pragma once

#include "Exceptions.h"
#include "ModelOrder1.h"
#include "ModelOrder2.h"
#include "ModelOrder3.h"
#include "Parameters.h"

class ModelsFactory
{
public:
	static IModel* GetModel()
	{
		return GetModel(Parameters::MODEL_ORDER);
	}

	static IModel* GetModel(uint8_t mo)
	{
		switch (mo)
		{
		case 1:  return new ModelOrder1(); break;
		case 2:  return new ModelOrder2(); break;
		case 3:  return new ModelOrder3(); break;
		case 4:  return new ModelOrder4(); break;
		}
		throw std::invalid_argument("Invalid model order.");

	}
};
