#ifndef PARTEST_LOG_H
#define PARTEST_LOG_H

#include <string>
#include <chrono>
#include "partestcommon.h"

// For logs from assert statements, used internally
#define PARTEST_LOG_TYPE_ASSERT "Assertion"
// For general test and subtest-level logs
#define PARTEST_LOG_TYPE_TEST "Test"
// For general test and subtest-level logs
#define PARTEST_LOG_TYPE_SUBTEST "Subtest"
// For all other logs
#define PARTEST_LOG_TYPE_DEFAULT "Default"

namespace partest
{
	enum class LogLevel : uint8_t
	{
		Error = 0,
		Warning,
		Info,
		Debug
	};

	struct LogEntry
	{
		LogLevel level;
		std::string type;
		std::string message;
		unsigned int testFrameID;

		LogEntry() : LogEntry(LogLevel::Info, PARTEST_LOG_TYPE_DEFAULT, "") { }
		LogEntry(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message, unsigned int testFrameID = 0) : level(level),  type(type), message(message), testFrameID(testFrameID) {}
	};
}

#endif //PARTESTLOG_H