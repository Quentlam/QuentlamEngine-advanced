workspace "QuentlamEngine"
	architecture "x64"
	characterset "Unicode"
	buildoptions { "/utf-8" }
	startproject "QL-Editor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

	IncludeDir = {}
	IncludeDir["GLFW"] = "QuentlamEngine/vendor/GLFW/include"
	IncludeDir["Glad"] = "QuentlamEngine/vendor/Glad/include"
	IncludeDir["ImGui"] = "QuentlamEngine/vendor/imgui"
	IncludeDir["ImGuizmo"] = "QuentlamEngine/vendor/ImGuizmo"
	IncludeDir["glm"] = "QuentlamEngine/vendor/glm"
	IncludeDir["entt"] = "QuentlamEngine/vendor/entt/src"
	IncludeDir["stb_image"] = "QuentlamEngine/vendor/stb_image"
	IncludeDir["assimp"] = "QuentlamEngine/vendor/assimp/include"
	IncludeDir["assimp_build"] = "QuentlamEngine/vendor/assimp/build/include"
	IncludeDir["assimp_rapidjson"] = "QuentlamEngine/vendor/assimp/contrib/rapidjson/include"
	IncludeDir["Box2D"] = "QuentlamEngine/vendor/Box2D/include"
	IncludeDir["JoltPhysics"] = "QuentlamEngine/vendor/JoltPhysics"
	IncludeDir["miniaudio"] = "QuentlamEngine/vendor/miniaudio"
	IncludeDir["lua"] = "QuentlamEngine/vendor/lua/src"
	IncludeDir["sol2"] = "QuentlamEngine/vendor/sol2/include"

	CoreProjectIncludeDirs =
	{
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.assimp_build}",
		"%{IncludeDir.assimp_rapidjson}",
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.miniaudio}",
		"%{IncludeDir.sol2}",
		"%{IncludeDir.lua}"
	}

	CoreProjectDefines =
	{
		"ENTT_INCLUDE_NATVIS"
	}

	include "QuentlamEngine/vendor/GLFW"
	include "QuentlamEngine/vendor/Glad"
	include "QuentlamEngine/vendor/ImGui"
	include "QuentlamEngine/vendor/ImGuizmo"

	project "Box2D"
		location "QuentlamEngine/vendor/Box2D"
		kind "Staticlib"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		files
		{
			"QuentlamEngine/vendor/Box2D/src/**.cpp",
			"QuentlamEngine/vendor/Box2D/include/**.h"
		}

		includedirs
		{
			"QuentlamEngine/vendor/Box2D/include",
			"QuentlamEngine/vendor/Box2D/src"
		}

		filter "system:windows"
			systemversion "latest"

		filter "configurations:Debug"
			runtime "Debug"

		filter "configurations:Release"
			runtime "Release"

		filter "configurations:Dist"
			runtime "Release"

	project "JoltPhysics"
		location "QuentlamEngine/vendor/JoltPhysics"
		kind "Staticlib"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		files
		{
			"QuentlamEngine/vendor/JoltPhysics/Jolt/**.cpp",
			"QuentlamEngine/vendor/JoltPhysics/Jolt/**.h",
			"QuentlamEngine/vendor/JoltPhysics/Jolt/**.inl"
		}

		includedirs
		{
			"QuentlamEngine/vendor/JoltPhysics"
		}

		defines
		{
			"JPH_PROFILE_ENABLED",
			"JPH_DEBUG_RENDERER"
		}

		buildoptions { "/FS" }

		filter "system:windows"
			systemversion "latest"

		filter "configurations:Debug"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Debug"

		filter "configurations:Release"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Release"

		filter "configurations:Dist"
			defines {}
			runtime "Release"

	project "Lua"
		location "QuentlamEngine/vendor/lua"
		kind "StaticLib"
		language "C"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		files
		{
			"QuentlamEngine/vendor/lua/src/lapi.c",
			"QuentlamEngine/vendor/lua/src/lauxlib.c",
			"QuentlamEngine/vendor/lua/src/lbaselib.c",
			"QuentlamEngine/vendor/lua/src/lcode.c",
			"QuentlamEngine/vendor/lua/src/lcorolib.c",
			"QuentlamEngine/vendor/lua/src/lctype.c",
			"QuentlamEngine/vendor/lua/src/ldblib.c",
			"QuentlamEngine/vendor/lua/src/ldebug.c",
			"QuentlamEngine/vendor/lua/src/ldo.c",
			"QuentlamEngine/vendor/lua/src/ldump.c",
			"QuentlamEngine/vendor/lua/src/lfunc.c",
			"QuentlamEngine/vendor/lua/src/lgc.c",
			"QuentlamEngine/vendor/lua/src/liolib.c",
			"QuentlamEngine/vendor/lua/src/llex.c",
			"QuentlamEngine/vendor/lua/src/lmathlib.c",
			"QuentlamEngine/vendor/lua/src/lmem.c",
			"QuentlamEngine/vendor/lua/src/loadlib.c",
			"QuentlamEngine/vendor/lua/src/lobject.c",
			"QuentlamEngine/vendor/lua/src/lopcodes.c",
			"QuentlamEngine/vendor/lua/src/loslib.c",
			"QuentlamEngine/vendor/lua/src/lparser.c",
			"QuentlamEngine/vendor/lua/src/lstate.c",
			"QuentlamEngine/vendor/lua/src/lstring.c",
			"QuentlamEngine/vendor/lua/src/lstrlib.c",
			"QuentlamEngine/vendor/lua/src/ltable.c",
			"QuentlamEngine/vendor/lua/src/ltablib.c",
			"QuentlamEngine/vendor/lua/src/ltm.c",
			"QuentlamEngine/vendor/lua/src/lundump.c",
			"QuentlamEngine/vendor/lua/src/lutf8lib.c",
			"QuentlamEngine/vendor/lua/src/lvm.c",
			"QuentlamEngine/vendor/lua/src/lzio.c",
			"QuentlamEngine/vendor/lua/src/linit.c",
			"QuentlamEngine/vendor/lua/src/lua.h",
			"QuentlamEngine/vendor/lua/src/luaconf.h",
			"QuentlamEngine/vendor/lua/src/lualib.h",
			"QuentlamEngine/vendor/lua/src/lua.hpp",
			"QuentlamEngine/vendor/lua/src/lauxlib.h",
			"QuentlamEngine/vendor/lua/src/lapi.h",
			"QuentlamEngine/vendor/lua/src/llimits.h",
			"QuentlamEngine/vendor/lua/src/lobject.h",
			"QuentlamEngine/vendor/lua/src/lstate.h",
			"QuentlamEngine/vendor/lua/src/ldo.h",
			"QuentlamEngine/vendor/lua/src/lfunc.h",
			"QuentlamEngine/vendor/lua/src/lgc.h",
			"QuentlamEngine/vendor/lua/src/lstring.h",
			"QuentlamEngine/vendor/lua/src/ltable.h",
			"QuentlamEngine/vendor/lua/src/ltm.h",
			"QuentlamEngine/vendor/lua/src/lzio.h",
			"QuentlamEngine/vendor/lua/src/llex.h",
			"QuentlamEngine/vendor/lua/src/lcode.h",
			"QuentlamEngine/vendor/lua/src/lparser.h",
			"QuentlamEngine/vendor/lua/src/ldebug.h",
			"QuentlamEngine/vendor/lua/src/lopcodes.h",
			"QuentlamEngine/vendor/lua/src/lopnames.h",
			"QuentlamEngine/vendor/lua/src/ljumptab.h",
			"QuentlamEngine/vendor/lua/src/lprefix.h",
			"QuentlamEngine/vendor/lua/src/lundump.h",
			"QuentlamEngine/vendor/lua/src/lvm.h",
			"QuentlamEngine/vendor/lua/src/lmem.h",
			"QuentlamEngine/vendor/lua/src/lctype.h"
		}

		includedirs
		{
			"QuentlamEngine/vendor/lua/src"
		}

		defines
		{
			"LUA_COMPAT_5_3"
		}

		filter "system:windows"
			systemversion "latest"
			buildoptions { "/wd4005" }  -- suppress macro-redefinition warnings for lua headers

		filter "configurations:Debug"
			runtime "Debug"

		filter "configurations:Release"
			runtime "Release"

		filter "configurations:Dist"
			runtime "Release"

	project "QuentlamEngine"
		location "QuentlamEngine"
		kind "Staticlib"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		pchheader "qlpch.h"
		pchsource "QuentlamEngine/src/qlpch.cpp"
		forceincludes "qlpch.h"

		files
		{
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
			"%{prj.name}/vendor/glm/glm/**.hpp",
			"%{prj.name}/vendor/glm/glm/**.inl",
			"%{prj.name}/vendor/stb_image/**.h",
			"%{prj.name}/vendor/stb_image/**.cpp",
			"%{prj.name}/vendor/miniaudio/miniaudio.h",
			"%{prj.name}/vendor/miniaudio/miniaudio.cpp"
		}

		defines
		{
			"_CRT_SECURE_NO_WARNINGS"
		}

		includedirs
		{
			"%{prj.name}/src",
			"%{prj.name}/vendor/spdlog/include",
			"%{IncludeDir.GLFW}",
			"%{IncludeDir.Glad}",
			"%{IncludeDir.ImGui}",
			"%{IncludeDir.ImGuizmo}",
			"%{IncludeDir.glm}",
			"%{IncludeDir.stb_image}",
			"%{IncludeDir.entt}",
			"%{IncludeDir.assimp}",
			"%{IncludeDir.assimp_build}",
			"%{IncludeDir.assimp_rapidjson}",
			"%{IncludeDir.sol2}",
			"%{IncludeDir.Box2D}",
			"%{IncludeDir.JoltPhysics}",
			"%{IncludeDir.miniaudio}",
			"%{IncludeDir.sol2}",
			"%{IncludeDir.lua}"
		}

		links
		{
			"GLFW",
			"Glad",
			"ImGui",
			"ImGuizmo",
			"Box2D",
			"JoltPhysics",
			"Lua",
			"opengl32.lib"
		}

		filter "system:windows"
			staticruntime "on"
			systemversion "latest"

			defines
			{
				"QL_PLATFORM_WINDOWS",
				"QL_BUILD_DLL",
				"_WINDLL",
				"GLFW_INCLUDE_NONE"
			}

		filter "configurations:Debug"
			defines "QL_DEBUG"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Debug"
			symbols "on"
			links { "QuentlamEngine/vendor/assimp/build/lib/Debug/assimp-vc143-mtd.lib", "QuentlamEngine/vendor/assimp/build/contrib/zlib/Debug/zlibstaticd.lib" }

		filter "configurations:Release"
			defines "QL_RELEASE"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Release"
			optimize "on"
			links { "QuentlamEngine/vendor/assimp/build/lib/Release/assimp-vc143-mt.lib", "QuentlamEngine/vendor/assimp/build/contrib/zlib/Release/zlibstatic.lib" }

		filter "configurations:Dist"
			defines "QL_DIST"
			runtime "Release"
			optimize "on"
			links { "QuentlamEngine/vendor/assimp/build/lib/Release/assimp-vc143-mt.lib", "QuentlamEngine/vendor/assimp/build/contrib/zlib/Release/zlibstatic.lib" }

	project "Sandbox"
		location "Sandbox"
		kind "ConsoleAPP"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"
		debugdir "Sandbox"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		pchheader "qlpch.h"
		pchsource "QuentlamEngine/src/qlpch.cpp"
		forceincludes "qlpch.h"

		files
		{
			"QuentlamEngine/src/qlpch.h",
			"QuentlamEngine/src/qlpch.cpp",
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
		}

		includedirs
		{
			"QuentlamEngine/vendor/spdlog/include",
			"QuentlamEngine/src",
			"QuentlamEngine/vendor",
			"%{IncludeDir.glm}",
			"%{IncludeDir.ImGui}",
			"%{IncludeDir.Glad}",
			"%{IncludeDir.entt}",
			"%{IncludeDir.ImGuizmo}",
			"%{IncludeDir.assimp}",
			"%{IncludeDir.assimp_build}",
			"%{IncludeDir.assimp_rapidjson}",
			"%{IncludeDir.sol2}",
			"%{IncludeDir.lua}"
		}

		externalincludedirs
		{
			table.unpack(CoreProjectIncludeDirs)
		}

		links
		{
			"QuentlamEngine",
			"Box2D"
		}

		filter "system:windows"
			staticruntime "On"
			systemversion "latest"

		defines
		{
			"QL_PLATFORM_WINDOWS",
			table.unpack(CoreProjectDefines)
		}

		filter "configurations:Debug"
			defines "QL_DEBUG"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines "QL_RELEASE"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Release"
			optimize "on"
			symbols "on"

		filter "configurations:Dist"
			defines "QL_DIST"
			runtime "Release"
			optimize "on"
			symbols "on"

	project "QL-Editor"
		location "QL-Editor"
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++20"
		staticruntime "on"
		debugdir "QL-Editor"

		targetdir("bin/" ..outputdir.. "/%{prj.name}")
		objdir("bin-int/" ..outputdir.. "/%{prj.name}")

		pchheader "qlpch.h"
		pchsource "QuentlamEngine/src/qlpch.cpp"
		forceincludes "qlpch.h"

		files
		{
			"QuentlamEngine/src/qlpch.h",
			"QuentlamEngine/src/qlpch.cpp",
			"%{prj.name}/src/**.h",
			"%{prj.name}/src/**.cpp",
		}

		includedirs
		{
			"QuentlamEngine/vendor/spdlog/include",
			"QuentlamEngine/src",
			"QuentlamEngine/vendor",
			"%{IncludeDir.GLFW}",
			"%{IncludeDir.glm}",
			"%{IncludeDir.ImGui}",
			"%{IncludeDir.entt}",
			"%{IncludeDir.ImGuizmo}",
			"%{IncludeDir.Glad}",
			"%{IncludeDir.Box2D}",
			"%{IncludeDir.assimp}",
			"%{IncludeDir.assimp_build}",
			"%{IncludeDir.assimp_rapidjson}",
			"%{IncludeDir.sol2}",
			"%{IncludeDir.lua}"
		}

		externalincludedirs
		{
			table.unpack(CoreProjectIncludeDirs)
		}

		links
		{
			"QuentlamEngine",
			"Box2D"
		}

		filter "system:windows"
			staticruntime "On"
			systemversion "latest"

		defines
		{
			"QL_PLATFORM_WINDOWS",
			table.unpack(CoreProjectDefines)
		}

		filter "configurations:Debug"
			defines "QL_DEBUG"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Debug"
			symbols "on"

		filter "configurations:Release"
			defines "QL_RELEASE"
			defines { "JPH_PROFILE_ENABLED", "JPH_DEBUG_RENDERER" }
			runtime "Release"
			optimize "on"
			symbols "on"

		filter "configurations:Dist"
			defines "QL_DIST"
			runtime "Release"
			optimize "on"
			symbols "on"
