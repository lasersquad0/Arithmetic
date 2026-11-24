/*
 * Exceptions.h
 *
 * Copyright 2026, LogEngine2 Project. All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#pragma once

#include <stdarg.h>
#include <string>
#include "Common.h"

LOGENGINE_NS_BEGIN

// 2K should be enough for one format line
//#define VARIABLE_LIST_BUFFER_SIZE (2*1024)

class LogException : public std::exception
{
private:
	std::string FMessage;
public:

	template<class... Args>
	LogException(const std::format_string<Args...> fmt, Args&&... args)
	{
		FMessage = std::format(fmt, std::forward<Args>(args)...);
	}
	/*
	LogException(const char* formatstr, ...)
	{
		char res[VARIABLE_LIST_BUFFER_SIZE];
		va_list list;
		va_start(list, formatstr);
#ifdef WIN32 // __STDC_SECURE_LIB__ //_MSC_VER < 1400
		vsprintf_s(res, VARIABLE_LIST_BUFFER_SIZE, formatstr, list);
#else
		vsprintf(res, formatstr, list);
#endif
		va_end(list);
		FMessage = res;
	}*/

	LogException(const std::string& Message) { FMessage = Message; }
	LogException(const LogException& ex) { FMessage = ex.FMessage; }
	~LogException() noexcept override { /*nothing to do */ }

	LogException& operator=(const LogException& ex)
	{
		FMessage = ex.FMessage;
		return *this;
	}

	virtual const char* what() const noexcept override //throw()
	{
		static std::string s = "[LogException] " + FMessage;
		return s.c_str();
	}
};

LOGENGINE_NS_END
