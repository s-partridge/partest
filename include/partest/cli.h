#ifndef PARTEST_CLI_H
#define PARTEST_CLI_H

#include <iostream>
#include <cstring>	// For strcmp

#include <partest/stringops.h>
#include <partest/fileops.h>

namespace partest
{
	using TestNameURL = std::vector<std::string>;

	inline bool isFlag(const char *arg) noexcept;

	class ValidArgs
	{
		bool m_filtered = false;
		bool m_output = false;
		bool m_concurrent = false;

		bool m_validState = true;
		std::vector<TestNameURL> m_testNames;
		std::string m_outputFile;
	public:
		ValidArgs() = default;
		bool filtered() const noexcept { return m_filtered; }
		void setFiltered(bool value) noexcept { m_filtered = value; }
		bool output() const noexcept { return m_output; }
		void setOutput(bool value) noexcept { m_output = value; }
		bool concurrent() const noexcept { return m_concurrent; }
		void setConcurrent(bool value) noexcept { m_concurrent = value; }

		// This is currently always true.
		// Future additions will set this to false if the argument list makes test execution impossible.
		bool validState() const noexcept { return m_validState; }

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

		int setOutputFileFromArgs(int argc, const char **argv, int currentArg)
		{
			if(currentArg < argc && !isFlag(argv[currentArg]))
			{
				m_outputFile = argv[currentArg];
				++currentArg;
			}
			return currentArg;
		}

		const std::vector<TestNameURL> &getTestNames() const noexcept { return m_testNames; }
		const std::string &getOutputFile() const noexcept { return m_outputFile; }
	};


	inline bool isFlag(const char *arg) noexcept
	{
		return std::strlen(arg) && arg[0] == '-';
	}

	// Parse command line arguments into a collection
	ValidArgs parseArgs(int argc, const char **argv)
	{
		ValidArgs args;
		for(int currentArg = 1; currentArg < argc; ++currentArg)
		{
			if(isFlag(argv[currentArg]))
			{
				if(std::strcmp(argv[currentArg], "-f") == 0 || std::strcmp(argv[currentArg], "--filter") == 0)
				{
					args.setFiltered(true);
					currentArg = args.setTestNamesFromArgs(argc, argv, currentArg + 1);
				}
				else if(std::strcmp(argv[currentArg], "-o") == 0 || std::strcmp(argv[currentArg], "--output") == 0)
				{
					args.setOutput(true);
					currentArg = args.setOutputFileFromArgs(argc, argv, currentArg + 1);
				}
				else if(std::strcmp(argv[currentArg], "-c") == 0 || std::strcmp(argv[currentArg], "--concurrent") == 0)
				{
					args.setConcurrent(true);
				}
				else
				{
					std::cout << "Unrecognized flag: '" << argv[currentArg] << "" << std::endl;
				}
			}
			else
			{
				std::cout << "Unknown argument: '" << argv[currentArg] << "'" << std::endl;
			}
		}
		return args;
	}
}

#endif