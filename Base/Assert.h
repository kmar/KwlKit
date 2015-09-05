
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if !defined(NDEBUG) && !defined(DEBUG) && !defined(_DEBUG)
#	define NDEBUG
#endif

#include <cassert>
#define KWLKIT_ASSERT(x) assert(x)
// compile-time assert
#define KWLKIT_COMPILE_ASSERT(cond) do { struct CompileTimeAssert { char arr[(cond) ? 1 : -1]; } tmp; tmp.arr[0]=0;(void)tmp; } while(0)
