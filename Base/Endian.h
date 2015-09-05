
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"
#include "Platform.h"
#include "Assert.h"

#if KWLKIT_COMPILER_MSC
#	include <intrin.h>
#endif

namespace KwlKit
{

struct Endian
{
	// generic byte swap (assuming that for 1-byte types compiler will optimize this to nop)
	template< typename T > static inline void ByteSwap( T &v )
	{
		Byte * cv = reinterpret_cast<Byte *>(&v);
		for ( size_t i=0; i<sizeof(T)/2; i++ ) {
			Byte tmp = cv[i];
			cv[i] = cv[sizeof(T)-1-i];
			cv[sizeof(T)-1-i] = tmp;
		}
	}

	// returns true if host is little endian
	static inline bool IsLittle()
	{
		UShort x = 1;
		return *reinterpret_cast<const Byte *>(&x) == 1;
	}

	// returns true if host is big endian
	static inline bool IsBig() {
		return !IsLittle();
	}

	// generic function to convert from litle endian to host (native)
	template< typename T > static inline void FromLittle( T &v )
	{
		if ( IsBig() ) {
			ByteSwap( v );
		}
	}

	// generic function to convert from big endian to host (native)
	template< typename T > static inline void FromBig( T &v )
	{
		if ( IsLittle() ) {
			ByteSwap( v );
		}
	}

	// generic function to convert from host (native) to little endian
	template< typename T > static inline void ToLittle( T &v ) {
		return FromLittle( v );
	}

	// generic function to convert from host (native) to big endian
	template< typename T > static inline void ToBig( T &v ) {
		return FromBig( v );
	}

	// swap between host (native) and little endian
	template< typename T > static inline void SwapLittle( T &v ) {
		return FromLittle( v );
	}

	// swap between host (native) and big endian
	template< typename T > static inline void SwapBig( T &v ) {
		return FromBig( v );
	}
};

template<> inline void Endian::ByteSwap<Byte>( Byte & ) {
}

template<> inline void Endian::ByteSwap<SByte>( SByte & ) {
}

#if KWLKIT_COMPILER_MSC
	// FIXME: do these intrinsics require VS2012 compiler? => no problem for me!
	// specialized templates for most common elementary types
	// assuming Microsoft has sizeof long = 4
	template<> inline void Endian::ByteSwap<UShort>( UShort &v )
	{
		KWLKIT_ASSERT( sizeof(UShort) == sizeof(unsigned short) );
		v = _byteswap_ushort( v );
	}
	template<> inline void Endian::ByteSwap<Short>( Short &v )
	{
		KWLKIT_ASSERT( sizeof(Short) == sizeof(unsigned short) );
		*(UShort *)(&v) = _byteswap_ushort( *(UShort *)(&v) );
	}
	template<> inline void Endian::ByteSwap<UInt>( UInt &v )
	{
		KWLKIT_ASSERT( sizeof(UInt) == sizeof(unsigned long) );
		v = _byteswap_ulong( v );
	}
	template<> inline void Endian::ByteSwap<Int>( Int &v )
	{
		KWLKIT_ASSERT( sizeof(Int) == sizeof(unsigned long) );
		*(UInt *)(&v) = _byteswap_ulong( *(UInt *)(&v) );
	}
	template<> inline void Endian::ByteSwap<Float>( Float &v )
	{
		KWLKIT_ASSERT( sizeof(Float) == sizeof(unsigned long) );
		*(UInt *)&v = _byteswap_ulong( *(UInt *)(&v) );
	}
	template<> inline void Endian::ByteSwap<ULong>( ULong &v )
	{
		KWLKIT_ASSERT( sizeof(ULong) == sizeof(unsigned __int64) );
		v = _byteswap_uint64( v );
	}
	template<> inline void Endian::ByteSwap<Long>( Long &v )
	{
		KWLKIT_ASSERT( sizeof(Long) == sizeof(__int64) );
		*(ULong *)&v = _byteswap_uint64( *(ULong *)&v );
	}
	template<> inline void Endian::ByteSwap<Double>( Double &v )
	{
		KWLKIT_ASSERT( sizeof(Double) == sizeof(__int64) );
		*(ULong *)&v = _byteswap_uint64( *(ULong *)&v );
	}
#elif KWLKIT_COMPILER_GCC
	// specialized templates for most common elementary types
	// note __bultin_bswap16 is missing
	template<> inline void Endian::ByteSwap<UInt>( UInt &v ) {
		v = __builtin_bswap32( v );
	}
	template<> inline void Endian::ByteSwap<Int>( Int &v ) {
		*(UInt *)&v = __builtin_bswap32( *(UInt *)&v );
	}
	template<> inline void Endian::ByteSwap<Float>( Float &v )
	{
		KWLKIT_ASSERT( sizeof(Float) == sizeof(uint32_t) );
		*(UInt *)&v = __builtin_bswap32( *(UInt *)&v );
	}
	template<> inline void Endian::ByteSwap<ULong>( ULong &v ) {
		v = __builtin_bswap64( v );
	}
	template<> inline void Endian::ByteSwap<Long>( Long &v ) {
		*(ULong *)&v = __builtin_bswap64( *(ULong *)&v );
	}
	template<> inline void Endian::ByteSwap<Double>( Double &v )
	{
		KWLKIT_ASSERT( sizeof(Double) == sizeof(uint64_t) );
		*(ULong *)&v = __builtin_bswap64( *(ULong *)&v );
	}
#else
// TODO: could to better than that, like masking + shifting
#endif

}
