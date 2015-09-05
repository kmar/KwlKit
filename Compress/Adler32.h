
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Types.h"
#include "../Base/Likely.h"

namespace KwlKit
{

static const UInt ADLER32_INIT = 1;

UInt GetAdler32( const void *buf, size_t size, UInt crc = ADLER32_INIT );

inline UInt GetAdler32Byte( Byte b, UInt crc = ADLER32_INIT )
{
	UInt hi = (crc >> 16);
	UInt lo = crc & 0xffff;

	lo += b;
	if ( KWLKIT_UNLIKELY( lo >= 65521 ) ) {
		lo -= 65521;
	}
	hi += lo;
	if ( KWLKIT_UNLIKELY( hi >= 65521 ) ) {
		hi -= 65521;
	}
	return lo | (hi << 16);
}

}
