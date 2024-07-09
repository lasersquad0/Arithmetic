#pragma once

#include <stdexcept>

//using namespace std;

class bad_file_format :public std::runtime_error
{
public:
	bad_file_format(const std::string& what_arg) : std::runtime_error(what_arg) {}

};

class file_error :public std::runtime_error
{
public:
	file_error(const std::string& what_arg) : std::runtime_error(what_arg) {}
};

