
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Platform.h"

#define KWLKIT_RET_FALSE_UNLIKELY(x) do { if ( KWLKIT_UNLIKELY( !(x) ) ) return 0; } while(0)
#define KWLKIT_RET_TRUE_UNLIKELY(x) do { if ( KWLKIT_UNLIKELY( (x) ) ) return 0; } while(0)
#define KWLKIT_RET_FALSE_LIKELY(x) do { if ( KWLKIT_LIKELY( !(x) ) ) return 0; } while(0)
#define KWLKIT_RET_TRUE_LIKELY(x) do { if ( KWLKIT_LIKELY( (x) ) ) return 0; } while(0)

#if KWLKIT_COMPILER_MSC
#	pragma warning(disable:4127)	// disable silly conditional expression is constant warning
#endif

#if !defined(KWLKIT_LIKELY)
#	if KWLKIT_COMPILER_MSC
#		define KWLKIT_LIKELY(x) x
#		define KWLKIT_UNLIKELY(x) x
#	else
#		define KWLKIT_LIKELY(x) __builtin_expect(!!(x),1)
#		define KWLKIT_UNLIKELY(x) __builtin_expect(!!(x), 0)
#	endif
#endif

#define KWLKIT_RET_FALSE(x) KWLKIT_RET_FALSE_UNLIKELY(x)
