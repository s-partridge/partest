#ifndef PARTESTLOG_H
#define PARTESTLOG_H

#include <string>
#include "partestcommon.h"

// For logs from assert statements, used internally
#define PARTEST_LOG_ASSERT "Assertion"
// For logs generated on test completion
#define PARTEST_LOG_TEST_END "TestEnd"
// For all other logs
#define PARTEST_LOG_DEFAULT "Default"

namespace partest
{
	enum LogLevel : uint8_t
	{
		INFO = 0,
		WARNING,
		ERROR,
		DEBUG
	};

	struct LogEntry
	{
		LogLevel level;
		std::string type;
		std::string message;
		PARTEST_CONSTEXPR_20 LogEntry() : level(INFO), type(PARTEST_LOG_DEFAULT), message("") {}
		PARTEST_CONSTEXPR_20 LogEntry(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message) : level(level),  type(type), message(message) {}
	};
}

#endif //PARTESTLOG_H