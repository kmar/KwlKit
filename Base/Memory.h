
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"

namespace KwlKit
{

void MemSet( void *dst, int value, size_t count );
// src and dst must not overlap (=no aliasing, restrict)
void MemCpy( void *dst, const void *src, size_t count );
// src and dst may overlap
void MemMove( void *dst, const void *src, size_t count );
int MemCmp( const void *src0, const void *src1, size_t count );

}
