#ifndef PARTEST_STRING_OPS_H
#define PARTEST_STRING_OPS_H

#include <sstream>
#include <string>
#include <chrono>
#include <mutex>
#include <ctime>

#include <partest/common.h>

namespace partest
{
	struct XMLEscapeTable {
		enum Mode { Plaintext, SingleQuoted, DoubleQuoted };

		static const XMLEscapeTable &PlaintextTable() noexcept
		{
			static XMLEscapeTable table(Mode::Plaintext);
			return table;
		}

		static const XMLEscapeTable &SingleQuotedTable() noexcept
		{
			static XMLEscapeTable table(Mode::SingleQuoted);
			return table;
		}

		static const XMLEscapeTable &DoubleQuotedTable() noexcept
		{
			static XMLEscapeTable table(Mode::DoubleQuoted);
			return table;
		}

		// Map of allowed/disallowed characters for XML output
		// 128 indices, one for each ASCII char in the set
		const char *map[0x100];

	private:
		
		// Initialize map to nullptr
		// Characters with nullptr mapping pass through unmodified
		// Characters with *empty* mapping are deleted
		explicit XMLEscapeTable(Mode mode) noexcept : map()
		{
			unsigned idx = 0;
			// Control characters are  skipped
			for(idx; idx < 0x09; ++idx)
			{
				map[idx] = "";
			}

			// For plain text, tab and linefeed pass through 
			if(mode != Mode::Plaintext)
			{
				map[0x09] = "&#9;";
				map[0x0A] = "&#10;";
			}

			// Control characters
			map[0x0B] = "";
			map[0x0C] = "";
			// Carriage return; always escaped
			map[0x0D] = "&#13;";

			// These are also control characters
			for(idx = 0x0E; idx < 0x20; ++idx)
				map[idx] = "";

			// Everything else 
			//for(idx; idx <= 0x7E; ++idx)
			//	map[idx] = nullptr;

			// XML critical characters
			map['&'] = "&amp;";
			map['<'] = "&lt;";
			map['>'] = "&gt;";

			switch(mode)
			{
				case Mode::SingleQuoted:
					map['\''] = "&apos;";
					break;
				case Mode::DoubleQuoted:
					map['"'] = "&quot;";
					break;
			}

			// Control character
			map[0x7F] = "";

			// Extended ASCII is not valid for UTF-8
			// TODO: Replace this with UTF-8 handling
			for(idx = 0x80; idx < 0x100; ++idx)
				map[idx] = "";
		}
	};

	inline const XMLEscapeTable &tableForMode(XMLEscapeTable::Mode mode) noexcept
	{
		switch(mode)
		{
		case XMLEscapeTable::Mode::Plaintext:
			return XMLEscapeTable::PlaintextTable();
		case XMLEscapeTable::Mode::SingleQuoted:
			return XMLEscapeTable::SingleQuotedTable();
		case XMLEscapeTable::Mode::DoubleQuoted:
			// To keep the compiler from complaining about return paths
			break;
		}
		return XMLEscapeTable::DoubleQuotedTable();
	}

	inline std::string sanitizeForXML(PARTEST_STRING_PARAM src, XMLEscapeTable::Mode mode)
	{
		const XMLEscapeTable &table = tableForMode(mode);
		std::string out;
		out.reserve(src.length());
		// TODO: Optimize this. Most strings have no modifications
		//   Scan first, stop if mapping is found. Otherwise return source string.
		//   *then* reserve out, and start iterating from that point.
		//   From there, runs of clean bytes separated by single escaped ones.
		//   Append by run instead of by character
		for(size_t idx = 0; idx < src.length(); ++idx)
		{
			char sourceChar = src[idx];
			const char *mapping = table.map[(unsigned char)sourceChar];
			if(!mapping)
			{
				out += sourceChar;
			}
			else
				out += mapping;
		}
		return out;
	}


	inline bool gmtime_pt(const std::time_t &time, std::tm &calendarTime)
	{
#if defined(_MSC_VER)
		errno_t err = gmtime_s(&calendarTime, &time);
		return err == 0;
#elif defined(__linux__)   || defined(__APPLE__)  || defined(__FreeBSD__) || \
		  defined(__NetBSD__)  || defined(__OpenBSD__) || defined(__ANDROID__)
		return gmtime_r(&time, &calendarTime) != nullptr;
#else
		// Create a mutex to protect gmtime, which is not thread-safe
		static std::mutex mut;

		std::lock_guard<std::mutex> lock(mut);
		// Fallback to gmtime, which is not thread-safe
		std::tm *result = std::gmtime(&time);

		if(!result)
			return false;
		calendarTime = *result;
		return true;
#endif
	}

	// Standard datetime expected by JUnit
	inline std::string toIso8601(std::chrono::system_clock::time_point timePoint)
	{
		time_t time = std::chrono::system_clock::to_time_t(timePoint);

		std::tm calendarTime;
		if(!gmtime_pt(time, calendarTime))
			return "";

		std::ostringstream out;
		out << std::put_time(&calendarTime, "%Y-%m-%dT%H:%M:%SZ");
		return out.str();
	}
}

#endif