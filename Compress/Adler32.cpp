
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Adler32.h"
#include "../Base/Templates.h"

namespace KwlKit
{

UInt GetAdler32( const void *buf, size_t size, UInt crc )
{
	KWLKIT_ASSERT( buf );
	const Byte *b = static_cast<const Byte *>(buf);
	UInt hi = (crc >> 16);
	UInt lo = crc & 0xffff;

	while ( size >= 5552 ) {
		// do it fast
		for ( UInt i=5552; i>0; i-- ) {
			lo += *b++;
			hi += lo;
		}
		lo %= 65521;
		hi %= 65521;
		size -= 5552;
	}

	while ( size-- ) {
		lo += *b++;
		hi += lo;
	}

	lo %= 65521;
	hi %= 65521;

	return lo | (hi << 16);
}


}
