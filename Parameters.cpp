#include <algorithm>
#include "Parameters.h"

std::string Parameters::OUTPUT_DIR = ".\\";
log4cpp::Category& Parameters::logger = log4cpp::Category::getInstance(Parameters::LOGGER_NAME);


uint32_t Parameters::parseNumber(std::string s)
{
	int factor = 1;
	std::transform(s.begin(), s.end(), s.begin(), ::toupper); // to uppercase

	if(s.find_last_of('K') != std::string::npos)
	//if (s.lastIndexOf("K") > 0)
	{
		s = s.substr(0, s.length() - 1);
		factor = 1024;
	}
	else if (s.find_last_of('M') != std::string::npos)
	{
		s = s.substr(0, s.length() - 1);
		factor = 1024 * 1024;
	}

	return stoi(s) * factor;
}

uint8_t Parameters::parseModelOrder(std::string s)
{
	uint8_t order = (uint8_t)stoi(s);

	if (order < 1 || order > 4) throw std::invalid_argument("Specified model order is out of range (1..4).");

	return order;
}
