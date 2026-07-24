#ifndef PARTEST_FILEIO_H
#define PARTEST_FILEIO_H

#include <system_error>
#include <stdexcept>
#include <cerrno>
#include <string>
#include <partest/common.h>

#if PARTEST_CPP_VERSION >= 17
#include <filesystem>

namespace partest
{
	inline constexpr bool canGetWorkingDirectory() noexcept { return true; }

	inline std::string getWorkingDirectory()
	{
		std::filesystem::path cwd = std::filesystem::current_path();
		return cwd.string();
	}

	inline std::string makeAbsolutePath(PARTEST_STRING_PARAM relativePath)
	{
		if(relativePath.empty())
			return getWorkingDirectory();
		std::filesystem::path fullPath = relativePath;
		return std::filesystem::absolute(fullPath).string();
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
	inline constexpr bool canGetWorkingDirectory() noexcept
	{
	#if defined(PARTEST_GETCWD_WINDOWS) || defined(PARTEST_GETCWD_POSIX)
		return true;
	#else
		return false;
	#endif
	}
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
	#if defined(PARTEST_GETCWD_WINDOWS)
		char separator = '\\';
	#else
		char separator = '/';
	#endif

		// Add separator if no trailing slash exists
		if(!cwd.empty() && cwd.back() != separator)
			cwd += separator;

		return cwd + relativePath;
	}
}

#endif // PARTEST_CPP_VERSION >= 17
#endif // PARTEST_FILEIO_H