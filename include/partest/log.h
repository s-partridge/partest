#ifndef PARTEST_LOG_H
#define PARTEST_LOG_H

#include <string>
#include <chrono>
#include <ostream>

#include <partest/common.h>

namespace partest
{
	// For logs from assert statements, used internally
	PARTEST_INLINE_VAR_17 constexpr const char *LOG_TYPE_ASSERT = "Assertion";
	// For general test and subtest-level logs
	PARTEST_INLINE_VAR_17 constexpr const char *LOG_TYPE_TEST = "Test";
	// For reporting unexpected exceptions
	PARTEST_INLINE_VAR_17 constexpr const char *LOG_TYPE_EXCEPTION = "Exception";
	// For general test and subtest-level logs
	PARTEST_INLINE_VAR_17 constexpr const char *LOG_TYPE_SUBTEST = "Subtest";
	// For all other logs
	PARTEST_INLINE_VAR_17 constexpr const char *LOG_TYPE_DEFAULT = "Default";

	enum class LogLevel : uint8_t
	{
		Error = 0,
		Warning,
		Info,
		Debug
	};

	inline std::ostream &operator<<(std::ostream &out, const LogLevel &rhs)
	{
		switch(rhs)
		{
		case LogLevel::Error:
			out << "Error";
			break;

		case LogLevel::Warning:
			out << "Warning";
			break;

		case LogLevel::Info:
			out << "Info";
			break;

		case LogLevel::Debug:
			out << "Debug";
			break;

		default:
			out << "Unknown";
			break;
		}
		return out;
	}

	struct LogEntry
	{
		LogLevel level;
		std::string type;
		std::string message;
		unsigned int testFrameId;

		LogEntry() : LogEntry(LogLevel::Info, LOG_TYPE_DEFAULT, "") { }
		LogEntry(LogLevel level, PARTEST_STRING_PARAM type, PARTEST_STRING_PARAM message, unsigned int testFrameID = 0) : level(level),  type(type), message(message), testFrameId(testFrameID) {}
	};
}

#endif //PARTESTLOG_H