
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Memory.h"
#include <memory.h>
#include <cstring>

namespace KwlKit
{

// important note: it's undefined if memset/memcpy/memcpy (and I suppose memove as well)
// is being called with null pointer even if size is zero!

void MemSet( void *dst, int value, size_t count ) {
	memset( dst, value, count );
}

void MemCpy( void *dst, const void *src, size_t count ) {
	memcpy( dst, src, count );
}

void MemMove( void *dst, const void *src, size_t count ) {
	memmove( dst, src, count );
}

int MemCmp( const void *src0, const void *src1, size_t count ) {
	return memcmp( src0, src1, count );
}

}
