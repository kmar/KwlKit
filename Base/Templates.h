
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"
#include "Assert.h"

// useful templates

namespace KwlKit
{

// minimum value
template< typename T > static inline T Min( const T &x, const T &y ) {
	return x < y ? x : y;
}

// maximum value
template< typename T > static inline T Max( const T &x, const T &y ) {
	return y < x ? x : y;
}

// clamp value
template< typename T> static inline T Clamp( const T &value, const T &minv, const T &maxv ) {
	return Min( maxv, Max( value, minv ) );
}

// lerp value
template< typename T > static inline T Lerp( const T &x, const T &y, Float alpha ) {
	return x + (y-x)*alpha;
}

// absolute value
template< typename T > static inline T Abs( const T &x ) {
	return x < static_cast<T>(0) ? -x : x;
}

// squared value
template< typename T > static inline T Sqr( const T &x ) {
	return x*x;
}

// returns true if val is power of two
template< typename T > static inline bool IsPowerOfTwo( const T &val ) {
	return (val & (val-1)) == 0;
}

// swap template
template< typename T > static inline void Swap( T &x, T &y )
{
	T tmp = x;
	x = y;
	y = tmp;
}

// note: 2**result may be less that v!
// actually, this is MSBit index
template< typename T > static inline UShort Log2Int( T v )
{
	KWLKIT_ASSERT( v > 0 );
	// FIXME: better! can use bit scan on x86/x64
	UShort res = 0;
	while ( v > 0 )
	{
		res++;
		v >>= 1;
	}
	return res-1;
}

// this returns x such that (1 << x) >= v
// don't pass
template< typename T > static inline UShort Log2Size( T v )
{
	UShort res = Log2Int( v );
	return res += ((T)1 << res) < v;
}

// sign function, returns -1 if negative, 0 if zero and 1 if positive
template< typename T > static inline T Sign( T x ) {
	return (T)(0 < x) - (T)(x < 0);
}

// just avoid CppCheck warnings
template< typename T >
inline T CastTo( void *b ) {
	return reinterpret_cast<T>(b);
}
template< typename T >
inline T CastTo( const void *b ) {
	return reinterpret_cast<T>(b);
}

}
