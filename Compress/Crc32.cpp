
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Crc32.h"
#include "../Base/Assert.h"
#include "../Base/Templates.h"
#include "../Base/Endian.h"
#include "../Base/Likely.h"

namespace KwlKit
{

// use fast slicing with 4 tables
// reference: http://create.stephan-brumme.com/crc32/ (original version by Intel)
#define KWLKIT_USE_CRC32_SLICING	1

// pre-calculated CRC-32 table
UInt crc32Table[256];

#if KWLKIT_USE_CRC32_SLICING

UInt crc32Table1[256];
UInt crc32Table2[256];
UInt crc32Table3[256];

#endif

UInt GetCrc32( const void *buf, size_t sz, UInt crc )
{
	KWLKIT_ASSERT( buf );
	if ( KWLKIT_UNLIKELY(!buf) ) {
		return crc;
	}
	crc = ~crc;
	const Byte *b = static_cast<const Byte *>(buf);

#if KWLKIT_USE_CRC32_SLICING
	UInt align = (4u - ((UInt)(UIntPtr)b & 3)) & 3u;
	if ( sz >= 4 + align ) {
		sz -= align;
		while ( align-- ) {
			crc = (crc >> 8) ^ crc32Table[ (crc ^ *b++) & 255 ];
		}
		KWLKIT_ASSERT( !((UIntPtr)b & 3 ) );
		const UInt *ab = reinterpret_cast< const UInt * >( b );
		b += sz & ~(size_t)3;
		if ( Endian::IsLittle() ) {
			do {
				crc ^= *ab++;
				crc =
					crc32Table[ crc >> 24 ] ^
					crc32Table1[ (crc >> 16) & 255 ] ^
					crc32Table2[ (crc >> 8) & 255 ] ^
					crc32Table3[ crc & 255 ];
			} while ( (sz -= 4) >= 4 );
		} else {
			do {
				Endian::ByteSwap(crc);
				crc ^= *ab++;
				crc =
					crc32Table3[ crc >> 24 ] ^
					crc32Table2[ (crc >> 16) & 255 ] ^
					crc32Table1[ (crc >> 8) & 255 ] ^
					crc32Table[ crc & 255 ];
			} while ( (sz -= 4) >= 4 );
		}
	}
#endif

	while ( sz-- ) {
		crc = (crc >> 8) ^ crc32Table[ (crc ^ *b++) & 255 ];
	}
	return ~crc;
}

void ComputeCrc32Table( UInt *table, UInt poly )
{
	for ( UInt i=0; i<256; i++ ) {
		UInt t = i;
		for ( UInt j=0; j<8; j++ ) {
			t = (t >> 1) ^ (poly * (t & 1));
		}
		table[i] = t;
	}
}

void ComputeCrc32SlicingTables()
{
	ComputeCrc32Table( crc32Table );
#if KWLKIT_USE_CRC32_SLICING
	for ( Int i=0; i<256; i++ ) {
		crc32Table1[i] = (crc32Table[i] >> 8) ^ crc32Table[ crc32Table[i] & 255 ];
		crc32Table2[i] = (crc32Table1[i] >> 8) ^ crc32Table[ crc32Table1[i] & 255 ];
		crc32Table3[i] = (crc32Table2[i] >> 8) ^ crc32Table[ crc32Table2[i] & 255 ];
	}
#endif
}

}
