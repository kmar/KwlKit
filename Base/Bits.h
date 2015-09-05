
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Endian.h"

// Bits: bit twiddling routines

namespace KwlKit
{

struct Bits
{
	// get mask for low n bits (0 = none)
	template< typename T > static inline T GetLSBMask( Byte bits )
	{
		KWLKIT_ASSERT( bits <= 8*sizeof(T) );
		return ((T)1 << bits)-1;
	}

	// generic bit reversal
	// make sure to use only unsigned integral types here!
	template< typename T > static inline void Reverse( T &x )
	{
		T tmp = 0;
		for ( size_t i=0; i<sizeof(T)*8; i++) {
			tmp <<= 1;
			tmp |= (x & ((T)1 << i)) != 0;
		}
		x = tmp;
	}

	// bit-size version
	template< typename T > static inline void Reverse( T &x, Byte bits )
	{
		KWLKIT_ASSERT( bits > 0 && !(x & ~GetLSBMask<T>(bits)) );
		Reverse(x);
		x >>= 8*sizeof(T) - bits;
	}

};

// with the help of http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64BitsDiv - too lazy

template<> inline void Bits::Reverse<Byte>( Byte &x )
{
	// swap odd and even bits
	x = ((x >> 1) & 0x55u) | ((x & 0x55u) << 1);
	// swap consecutive pairs
	x = ((x >> 2) & 0x33u) | ((x & 0x33u) << 2);
	// swap nibbles
	x = ((x >> 4) & 0x0fu) | ((x & 0x0fu) << 4);
}

template<> inline void Bits::Reverse<UShort>( UShort &x )
{
	// swap odd and even bits
	x = ((x >> 1) & 0x5555u) | ((x & 0x5555u) << 1);
	// swap consecutive pairs
	x = ((x >> 2) & 0x3333u) | ((x & 0x3333u) << 2);
	// swap nibbles
	x = ((x >> 4) & 0x0f0fu) | ((x & 0x0f0fu) << 4);
	// swap bytes
	Endian::ByteSwap(x);
}

template<> inline void Bits::Reverse<UInt>( UInt &x )
{
	// swap odd and even bits
	x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
	// swap consecutive pairs
	x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
	// swap nibbles
	x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
	// swap bytes
	Endian::ByteSwap(x);
}

template<> inline void Bits::Reverse<ULong>( ULong &x )
{
	// swap odd and even bits
	x = ((x >> 1) & KWLKIT_CONST_ULONG(0x5555555555555555)) | ((x & KWLKIT_CONST_ULONG(0x5555555555555555)) << 1);
	// swap consecutive pairs
	x = ((x >> 2) & KWLKIT_CONST_ULONG(0x3333333333333333)) | ((x & KWLKIT_CONST_ULONG(0x3333333333333333)) << 2);
	// swap nibbles
	x = ((x >> 4) & KWLKIT_CONST_ULONG(0x0f0f0f0f0f0f0f0f)) | ((x & KWLKIT_CONST_ULONG(0x0f0f0f0f0f0f0f0f)) << 4);
	// swap bytes
	Endian::ByteSwap(x);
}

}
