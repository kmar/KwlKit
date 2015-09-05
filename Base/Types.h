
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Platform.h"

#include <cstddef>

#if !KWLKIT_COMPILER_MSC
	// instead of cstdint which has some problems with MinGW?
#	include <stdint.h>
#endif

namespace KwlKit
{

#if KWLKIT_COMPILER_MSC
typedef signed char SByte;
typedef unsigned char Byte;
typedef signed short Short;
typedef unsigned short UShort;
typedef signed int Int;
typedef unsigned int UInt;
typedef signed __int64 Long;
typedef unsigned __int64 ULong;
#else
typedef int8_t SByte;
typedef uint8_t Byte;
typedef int16_t Short;
typedef uint16_t UShort;
typedef int32_t Int;
typedef uint32_t UInt;
typedef int64_t Long;
typedef uint64_t ULong;
#endif
typedef bool Bool;
typedef intptr_t IntPtr;
typedef uintptr_t UIntPtr;
typedef float Float;
typedef double Double;

}
