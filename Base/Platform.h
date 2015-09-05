
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

// here we: detect compiler (msc vs gcc vs clang vs unknown)
// detect operating system (windows, linux, osx, bsd)
// detect 32 vs 64 bit version

namespace KwlKit
{

#if defined(__APPLE__)
#	include <TargetConditionals.h>
#endif

#if (defined(__GNUC__) && (defined(__LP64__) || defined(__x86_64__) || defined(__aarch64__))) ||\
	(defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64)))
#	define KWLKIT_64BIT				1
#else
	// FIXME: just guessing here....
#	define KWLKIT_32BIT				1
#endif

// CPU
#if (defined(__GNUC__) && defined(__x86_64__)) || (defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64)))
#	define KWLKIT_CPU_AMD64			1
#	define KWLKIT_CPU_X86				1
#elif (defined(__GNUC__) && (defined(__aarch64__) || defined(__arm64__) || defined(__arm__) || defined(__thumb__))) || \
	(defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARMT)))
#	define KWLKIT_CPU_ARM				1
#	if defined(__GNUC__) && (defined(__aarch64__) || defined(__arm64__))
#		define KWLKIT_CPU_ARM64		1
#	endif
#elif (defined(__GNUC__) && (defined(__i386__) || defined(__i386))) || (defined(_MSC_VER) && defined(_M_IX86))	\
	|| defined(_X86_) || defined(__X86__) || defined(__I86__)
#	define KWLKIT_CPU_X86				1
#endif

#if defined(_MSC_VER)
#	define KWLKIT_COMPILER_MSC		1
#	if defined(__INTEL_COMPILER)
#		define KWLKIT_COMPILER_ICC	1
#	endif
#elif defined(__GNUC__)
#	define KWLKIT_COMPILER_GCC		1
#else
#	define KWLKIT_COMPILER_UNKNOWN	1
#endif

#if defined(__MINGW32_VERSION) || defined(__MINGW32_MAJOR_VERSION)
#	define KWLKIT_COMPILER_MINGW		1
#endif

#if defined(__clang__)
#	define KWLKIT_COMPILER_CLANG		1
#endif

#if defined(_WIN32)
#	define KWLKIT_OS_WINDOWS			1
#elif defined(__linux__)
#	define KWLKIT_OS_LINUX			1
#elif defined(__FreeBSD__)
#	define KWLKIT_OS_BSD				1
#elif defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR)
#	define KWLKIT_OS_IOS				1
#elif defined(__APPLE__)
#	define KWLKIT_OS_OSX				1
#elif defined(__ANDROID__)
#	define KWLKIT_OS_ANDROID			1
#else
#	define KWLKIT_OS_UNKNOWN			1
#endif

#if defined(__EMSCRIPTEN__)
#	define KWLKIT_PLATFORM_EMSCRIPTEN	1
#endif

// platform-dependent macros for 64-bit long integer constants (assuming 32+-bit compiler so no need for 32-bit version)
#define KWLKIT_CONST_LONG(x) x##ll
#define KWLKIT_CONST_ULONG(x) x##ull

#if KWLKIT_COMPILER_GCC
	// this fixes annoying clang warnings
#	define KWLKIT_VISIBLE __attribute__((visibility("default")))
#else
#	define KWLKIT_VISIBLE
#endif

}
