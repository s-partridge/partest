#ifndef PARTESTLOG_H
#define PARTESTLOG_H

#include <string>
#include <chrono>
#include "partestcommon.h"

// For logs from assert statements, used internally
#define PARTEST_LOG_ASSERT "Assertion"
// For general test and subtest-level logs
#define PARTEST_LOG_TEST "Test"
// For general test and subtest-level logs
#define PARTEST_LOG_SUBTEST "Subtest"
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
		unsigned int testFrameID;

		LogEntry() : LogEntry(INFO, PARTEST_LOG_DEFAULT, "") { }
		LogEntry(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message, unsigned int testFrameID = 0) : level(level),  type(type), message(message), testFrameID(testFrameID) {}
	};
}

#endif //PARTESTLOG_H