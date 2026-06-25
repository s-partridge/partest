-- ParTest build script

-- Base set of C++ standards for all build environments
local standards = { "c14", "c17", "c20" }

-- Add C++11 if build system is known to still support it
-- Visual Studio doesn't support specifying C++11
if _ACTION and _ACTION:match("^gmake") then
	table.insert(standards, 1, "C11")
end

local configs = {}

-- Generate debug config strings first
for _, std in ipairs(standards) do
	table.insert(configs, "dbg-" .. std)
end

-- Then release
for _, std in ipairs(standards) do
	table.insert(configs, "rel-" .. std)
end

workspace "partest"
	configurations(configs)
	platforms { "x86", "x64" }
	language "C++"
	warnings "Default"

	filter "platforms:x86"
		architecture "x86"

	filter "platforms:x64"
		architecture "x86_64"

	filter "*c11"
		cppdialect "C++11"

	filter "*c14"
		cppdialect "C++14"

	filter "*c17"
		cppdialect "C++17"

	filter "*c20"
		cppdialect "C++20"

	filter "configurations:dbg*"
		defines { "_DEBUG" }
		symbols "On"
		optimize "Off"

	filter "configurations:rel*"
		defines { "NDEBUG" }
		optimize "On"

	filter "toolset:gcc or toolset:clang"
		buildoptions { "-pedantic" }

project "partest"
	kind "ConsoleApp"
	location "build"
	targetdir "%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"
	objdir "%{wks.location}/obj/%{cfg.platform}/%{cfg.buildcfg}"

    files { "source/**.h", "source/**.cpp" }
    includedirs { "source" }

    vpaths {
        ["Header Files"] = { "**.h" },
        ["Source Files"] = { "**.cpp" }
    }