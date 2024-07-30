#pragma once
#include <map>
#include <string>
#include <chrono>
#include <iostream>
#include "CommonFunctions.h"

std::string millisecToStr(long long ms);

class Ticks
{
public:
	typedef std::chrono::steady_clock::time_point timepoint;

private:
	static std::map<string_t, timepoint> s;
	static std::map<string_t, timepoint> f;

public:

	static void Start(string_t& tickName)
	{
		s[tickName] = std::chrono::steady_clock::now();
	}
	static long long Finish(string_t& tickName)
	{
		f[tickName] = std::chrono::steady_clock::now();
		return GetTick(tickName);
	}
	static long long GetTick(string_t& tickName)
	{
		timepoint fin = f.at(tickName);
		return std::chrono::duration_cast<std::chrono::milliseconds>(fin - s.at(tickName)).count();
	}
	static void Print(long factor)
	{
		for (auto& item : f)
		{
			std::wcout << item.first << _T("=") << std::chrono::duration_cast<std::chrono::milliseconds>(item.second - s.at(item.first)).count() / factor << std::endl;
		}
	}
};

