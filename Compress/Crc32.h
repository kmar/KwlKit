
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Types.h"

namespace KwlKit
{

extern UInt crc32Table[];
static const UInt CRC32_INIT = 0;

// calculate CRC-32 value of a buffer
UInt GetCrc32( const void *buf, size_t sz, UInt crc = CRC32_INIT );

inline UInt GetCrc32Byte( Byte b, UInt crc = CRC32_INIT )
{
	crc = ~crc;
	crc = (crc >> 8) ^ crc32Table[ (crc ^ b) & 255 ];
	return ~crc;
}

// direct version (without bit flipping - required for Zip encryption)
inline UInt GetCrc32ByteDirect( Byte b, UInt crc = CRC32_INIT ) {
	return (crc >> 8) ^ crc32Table[ (crc ^ b) & 255 ];
}

// compute CRC-32 lookup table
// default poly: zip-compatible
void ComputeCrc32Table( UInt *table, UInt poly = 0xedb88320u );

// compute slicing tables for fast CRC-32 computation
void ComputeCrc32SlicingTables();

}
