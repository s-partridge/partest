#ifndef PARTEST_CLI_H
#define PARTEST_CLI_H

#include <iostream>

#include <partest/stringops.h>
#include <partest/fileops.h>

namespace partest
{
	using TestNameURL = std::vector<std::string>;

	class ValidArgs
	{
		bool m_filtered = false;
		std::vector<TestNameURL> m_testNames;
	public:
		ValidArgs() = default;
		bool filtered() const noexcept { return m_filtered; }
		void filtered(bool value) noexcept { m_filtered = value; }

		/**
		* Convert a string into a period-delimited vector of substrings, representing the hierarchical structure of a test name.
		* 
		* @param arg The string to convert.
		* @return A TestNameURL vector containing the substrings of the input string, with periods removed.
		*/
		static TestNameURL stringToTestNameURL(const std::string &arg)
		{
			const char delimiter = '.';

			TestNameURL result;

			// Split the string by the delimiter and store each part in the result vector
			size_t start = 0;
			// stream the string until the end
			while (start < arg.length())
			{
				size_t end = arg.find(delimiter, start);
				if (end == std::string::npos)
				{
					end = arg.length();
				}
				result.push_back(arg.substr(start, end - start));
				start = end + 1; // Skip the delimiter.
			}
			return result;
		}

		/**
		* Convert a TestNameURL vector back into a period-delimited string.
		* 
		* @param url The TestNameURL vector to convert.
		* @return A string representing the hierarchical structure of the test name, with periods separating the substrings.
		*/
		static std::string testNameURLToString(const TestNameURL &url)
		{
			std::string result;
			for(size_t i = 0; i < url.size(); ++i)
			{
				if(i > 0)
					result += '.';
				result += url[i];
			}
			return result;
		}

		/**
		* Extract test names from command line arguments and store them in the m_testNames vector,
		* stopping at the next flag in the argument list.
		* 
		* @param argc The number of command line arguments.
		* @param argv The array of command line argument strings.
		* @param currentArg The index of the current argument to start processing from.
		* 
		* @returns The index of the next argument to process after the test names have been extracted.
		*/
		int setTestNamesFromArgs(int argc, const char **argv, int currentArg)
		{
			while(currentArg < argc)
			{
				if(!isFlag(argv[currentArg]))
				{
					m_testNames.push_back(stringToTestNameURL(argv[currentArg]));
					++currentArg;
				}
				else
				{
					break;
				}
			}
			return currentArg;
		}

		const std::vector<TestNameURL> &getTestNames() const noexcept { return m_testNames; }
	};


	inline bool isFlag(const std::string &arg) noexcept
	{
		return !arg.empty() && arg[0] == '-';
	}

	// Filter command line arguments into a collection
	ValidArgs filterArgs(int argc, const char **argv)
	{
		ValidArgs args;
		for(int i = 1; i < argc; ++i)
		{
			if(isFlag(argv[argc]))
			{
				if(strcmp(argv[argc], "-f") || strcmp(argv[argc], "--filter"))
				{
					args.filtered(true);

					i = args.setTestNamesFromArgs(argc, argv, i + 1);
					continue;
				}
				else
				{
					std::cout << "Unrecognized flag: '" << argv[i] << "" << std::endl;
				}
			}
			else
			{
				std::cout << "Unknown argument: '" << argv[i] << "'" << std::endl;
			}
		}
		return args;
	}
}

#endif