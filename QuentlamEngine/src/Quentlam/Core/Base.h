#pragma once
#include <memory>
#include <glm/glm.hpp>


#ifdef _WIN32
	#ifdef _WIN64

		#define QL_PALTFORM_WINDOWS
		#ifndef WIN32_LEAN_AND_MEAN
			#define WIN32_LEAN_AND_MEAN
		#endif
		#ifndef NOMINMAX
			#define NOMINMAX
		#endif
		#include <windows.h>
		#ifdef State
			#undef State
		#endif

	#else

	#error "x86 Builds are not supported!"

	#endif

#elif	defined(__APPLE__) || defined(__MACH__)
	#include<TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define QL_PALTFORM_IOS
		#error "IOS is not supported!"
	#elif TAGET_OS_MAC == 1
		#define QL_PALTFORM_MACOS
		#error "Mac OS is not supported!"
	#else
		#error "Unkonwn Apple platform!"
   #endif
	#elif defined(__ANDROID__)
		#define QL_PLATFORM_ANDROID
		#error "Android is not supported!"
	#elif define(__linux__)
		#define QL_PLATFORM_LINUX
		#error "Linux is not supported!"
    #else
		#error "Unkonwn platform!"
#endif



#ifdef QL_PLATFORM_WINDOWS
#if QL_DYNAMIC_LINK
	#ifdef QL_BUILD_DLL
		#define QUENTLAM_API __declspec(dllexport)
		#define IMGUI_API 	__declspec(dllexport)
	#else
		#define QUENTLAM_API __declspec(dllimport)
		#define IMGUI_API 	__declspec(dllimport)
	#endif
#else
	#define QUENTLAM_API
#endif

#else
	#error Quentlam only support Windows!
#endif

#ifdef QL_DEBUG
	#define QL_ENABLE_ASSERT
#endif

#ifdef QL_ENABLE_ASSERT
#define QL_ASSERT(x,...) {if(!(x)) {QL_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak();}}
#define QL_CORE_ASSERT(x,...) {if(!(x)) {QL_CORE_ERROR("Assertion Failed: {0}",__VA_ARGS__); __debugbreak();}}
#else
#define QL_ASSERT(x,...)
#define QL_CORE_ASSERT(x,...)
#endif


#define BIT(x)    (1 << x)

#define QL_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Quentlam
{

	template<typename T>
	using Scope = std::unique_ptr<T>;

	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T,typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}



}
