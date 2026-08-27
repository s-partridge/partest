#ifndef PARTEST_FILEOPS_H
#define PARTEST_FILEOPS_H

#include <system_error>
#include <stdexcept>
#include <cerrno>
#include <string>
#include <fstream>
#include <partest/common.h>

#if PARTEST_CPP_VERSION >= 17
#include <filesystem>

namespace partest
{
	inline constexpr bool canGetWorkingDirectory() noexcept { return true; }

	/**
	* Get the current working directory of the process.
	* 
	* @returns The current working directory as a string
	*/
	inline std::string getWorkingDirectory()
	{
		std::filesystem::path cwd = std::filesystem::current_path();
		return cwd.string();
	}

	/**
	* Find the absolute path of a file given a relative path.
	* 
	* @param relativePath The relative path to the file
	* @returns The absolute path to the file
	*/
	inline std::string makeAbsolutePath(PARTEST_STRING_PARAM relativePath)
	{
		if(relativePath.empty())
			return getWorkingDirectory();
		std::filesystem::path fullPath = relativePath;
		return std::filesystem::absolute(fullPath).string();
	}

	/**
	* Strip the directory path from a file path and return only the filename.
	* 
	* @param filePath The full path to the file
	* @returns The filename extracted from the path
	*/
	inline std::string getFilename(PARTEST_STRING_PARAM filePath)
	{
		std::filesystem::path fullPath = filePath;

		return fullPath.filename().string();
	}
}
#else
#include <cctype>

	#if defined(_WIN32)
		#define PARTEST_GETCWD_WINDOWS
		#include <direct.h>
		// MAX_PATH is defined as 260 in windows.h on every Windows distribution
		constexpr unsigned PARTEST_PATH_MAX = 260;

	#elif defined(__linux__)   || defined(__APPLE__)  || defined(__FreeBSD__) || \
		  defined(__NetBSD__)  || defined(__OpenBSD__) || defined(__ANDROID__)
		#define PARTEST_GETCWD_POSIX
		#include <unistd.h>
		#include <limits.h>

		#ifdef PATH_MAX
		constexpr unsigned PARTEST_PATH_MAX = PATH_MAX;
		#else
		constexpr unsigned PARTEST_PATH_MAX = 1024;
		#endif // PATH_MAX

	#else
		#define PARTEST_GETCWD_NONE
	#endif

namespace partest
{
#if defined(PARTEST_GETCWD_WINDOWS)
	static constexpr char separator = '\\';
#else
	static constexpr char separator = '/';
#endif
	
	/**
	* Check if the current platform supports getting the current working directory.
	* 
	* @returns true if the platform supports getting the current working directory, false otherwise
	*/
	inline constexpr bool canGetWorkingDirectory() noexcept
	{
	#if defined(PARTEST_GETCWD_WINDOWS) || defined(PARTEST_GETCWD_POSIX)
		return true;
	#else
		return false;
	#endif
	}

	/**
	* Get the current working directory of the process.
	* 
	* @returns The current working directory as a string
	*/
	inline std::string getWorkingDirectory()
	{
		char buffer[PARTEST_PATH_MAX];

	#if defined(PARTEST_GETCWD_WINDOWS)
		if(_getcwd(buffer, sizeof(buffer)) == nullptr)
	#elif defined(PARTEST_GETCWD_POSIX)
		if(getcwd(buffer, sizeof(buffer)) == nullptr)
	#else
		throw std::runtime_error("Platform does not support getWorkingDirectory()");
	#endif
		{
			throw std::system_error(errno, std::generic_category(), "Could not get current working directory.");
		}
		return std::string(buffer);
	}

	/**
	* Find the absolute path of a file given a relative path.
	* 
	* @param relativePath The relative path to the file
	* @returns The absolute path to the file
	*/
	inline std::string makeAbsolutePath(PARTEST_STRING_PARAM relativePath)
	{
		// Check for empty path
		if(relativePath.empty())
			return getWorkingDirectory();

		// Check for already absolute paths
	#if defined(PARTEST_GETCWD_WINDOWS)
		// Windows absolute paths begin with either a drive letter for local files or `\\` for network shares
		if(relativePath.size() >= 2
			&& ( (std::isalpha((unsigned char)relativePath[0]) && relativePath[1] == ':') ||
				 (relativePath[0] == '\\' && relativePath[1] == '\\') ))
			return relativePath;
	#elif defined(PARTEST_GETCWD_POSIX)
		if(relativePath[0] == '/')
			return relativePath;
	#else
		throw std::runtime_error("Platform does not support makeAbsolutePath()");
	#endif

		// Concatenate and return
		std::string cwd = getWorkingDirectory();

		// Add separator if no trailing slash exists
		if(!cwd.empty() && cwd.back() != separator)
			cwd += separator;

		return cwd + relativePath;
	}

	/**
	* Strip the directory path from a file path and return only the filename.
	* 
	* @param filePath The full path to the file
	* @returns The filename extracted from the path
	*/
	inline std::string getFilename(const std::string &filePath)
	{
		// If no string or the string ends with a separator (directory with no filename), return empty string
		if(filePath.empty() || filePath.back() == separator)
			return "";

		size_t found = filePath.length();

		// Walk back from the end of the string.
		
		size_t idx;
		for(idx = found; idx > 0; --idx)
		{
			//Check whether previous char is a path separator
			if(filePath[idx - 1] == separator)
			{
				found = idx;
				break;
			}
		}
		
		// If idx is 0, no separator was found, so the entire string is the filename.
		if(idx == 0)
			return filePath;
		else
			return filePath.substr(found);
	}
}

#endif // PARTEST_CPP_VERSION >= 17

namespace partest
{
	/**
	* Open or create a file and immediately close it
	* 
	* @param absolutePath The full path to the file to be opened
	* @param mode File IO mode used to open the file
	* 
	* @returns true if the file was opened successfully, false otherwise
	*/
	inline bool maybeOpenFile(PARTEST_STRING_PARAM absolutePath, std::ios_base::openmode mode)
	{
		std::fstream handle(PARTEST_STRING_PARAM_TO_STRING(absolutePath), mode);

		if(!handle.is_open())
			return false;

		handle.close();
		return true;
	}
}

#endif // PARTEST_FILEOPS_H