#ifndef PARTESTLOG_H
#define PARTESTLOG_H

#include <string>
#include <chrono>
#include "partestcommon.h"

// For logs from assert statements, used internally
#define PARTEST_LOG_ASSERT "Assertion"
// For general test and subtest-level logs
#define PARTEST_LOG_TEST "Test"
// For all other logs
#define PARTEST_LOG_DEFAULT "Default"

namespace partest
{
	enum LogLevel : uint8_t
	{
		ERROR = 0,
		WARNING,
		INFO,
		DEBUG
	};

	struct LogEntry
	{
		LogLevel level;
		std::string type;
		std::string message;
		std::chrono::system_clock::time_point timestamp;

		LogEntry() : LogEntry(INFO, PARTEST_LOG_DEFAULT, "") { }
		LogEntry(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message) : level(level),  type(type), message(message), timestamp(std::chrono::system_clock::now()) {}
	};
}

#endif //PARTESTLOG_H