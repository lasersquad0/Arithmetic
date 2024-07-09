#pragma once
#include <map>
#include <string>
#include <chrono>
#include <iostream>

std::string millisecToStr(long long ms);

class Ticks
{
public:
	typedef std::chrono::steady_clock::time_point timepoint;

private:
	static std::map<std::string, timepoint> s;
	static std::map<std::string, timepoint> f;

public:

	static void Start(std::string& tickName)
	{
		s[tickName] = std::chrono::steady_clock::now();
	}
	static long long Finish(std::string& tickName)
	{
		f[tickName] = std::chrono::steady_clock::now();
		return GetTick(tickName);
	}
	static long long GetTick(std::string& tickName)
	{
		timepoint fin = f.at(tickName);
		return std::chrono::duration_cast<std::chrono::milliseconds>(fin - s.at(tickName)).count();
	}
	static void Print(long factor)
	{
		for (auto& item : f)
		{
			std::cout << item.first << "=" << std::chrono::duration_cast<std::chrono::milliseconds>(item.second - s.at(item.first)).count() / factor << std::endl;
		}
	}
};

