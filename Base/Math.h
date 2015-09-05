
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Templates.h"
#include "Limits.h"
#include "Likely.h"
#include "Endian.h"
#include <cmath>
#if KWLKIT_COMPILER_MSC && KWLKIT_CPU_X86
#	include <intrin.h>
#endif

namespace KwlKit
{

// Constants

// Float constants
static const Float PI = (Float)3.1415926535897932384626433832795;
static const Float RAD_TO_DEG = (Float)(180.0 / 3.1415926535897932384626433832795);
static const Float DEG_TO_RAD = (Float)(3.1415926535897932384626433832795 / 180.0);

// Double constants
static const Double D_PI = 3.1415926535897932384626433832795;
static const Double D_RAD_TO_DEG = 180.0 / 3.1415926535897932384626433832795;
static const Double D_DEG_TO_RAD = 3.1415926535897932384626433832795 / 180.0;

// Fast Abs() for Float/Double
static inline Float Abs( const Float &x ) {
	return fabs(x);
}

static inline Double Abs( const Double &x ) {
	return fabs(x);
}

// arctangent of 2d vector in degrees
template< typename T > static inline T Atan2( T y, T x ) {
	return atan2(y, x) * (T)D_RAD_TO_DEG;
}

// cosine of angle in degrees
template< typename T > static inline T Cos( T x ) {
	return cos( x * (T)D_DEG_TO_RAD );
}

// sine of angle in degrees
template< typename T > static inline T Sin( T x ) {
	return sin( x * (T)D_DEG_TO_RAD );
}

// tangent of angle in degrees
template< typename T > static inline T Tan( T x ) {
	return tan( x * (T)D_DEG_TO_RAD );
}

// acos
template< typename T > static inline T ACos( T x ) {
	return acos( Clamp( x, (T)-1, (T)1 ) ) * (T)D_RAD_TO_DEG;
}

// asin
template< typename T > static inline T ASin( T x ) {
	return asin( Clamp( x, (T)-1, (T)1 ) ) * (T)D_RAD_TO_DEG;
}

// arctangent of 2d vector in radians
template< typename T > static inline Float Atan2Rad( T y, T x ) {
	return atan2(y, x);
}

// cosine of angle in radians
template< typename T > static inline T CosRad( T x ) {
	return cos( x );
}

// sine of angle in radians
template< typename T> static inline T SinRad( T x ) {
	return sin( x );
}

// tangent of angle in radians
template< typename T > static inline T TanRad( T x ) {
	return tan( x );
}

// acos
template< typename T > static inline T ACosRad( T x ) {
	return acos( Clamp( x, (T)-1, (T)1 ) );
}

// asin
template< typename T > static inline T ASinRad( T x ) {
	return asin( Clamp( x, (T)-1, (T)1 ) );
}

template< typename T > static inline T Sqrt( T x ) {
	return (T)sqrt( x );
}

template< typename T > static inline T FMod( T f, T m ) {
	return fmod( f, m );
}

template< typename T > static inline T Exp( T x ) {
	return exp( x );
}

template< typename T > static inline T Pow( T x, T y ) {
	return pow( x, y );
}

// natural logarithm (base e)
template< typename T > static inline T Ln( T x ) {
	return log( x );
}

// base-2 logarithm
template< typename T > static inline T Log2( T x ) {
	return Ln( x ) * (T)1.442695040888963407359924681001892137426645954152985934135449;
}

// base-10 logarithm
template< typename T > static inline T Log10( T x ) {
	return Ln( x ) * (T)0.434294481903251827651128918916605082294397005803666566114453;
}

// Special: float conversions

// floor (rounds down, floor(-2.1) = -3
template< typename T > static inline T Floor( T f ) {
	return (T)floor(f);
}

// ceil (rounds up, ceil(-2.1) = -2
template< typename T > static inline T Ceil( T f ) {
	return (T)ceil(f);
}

// return integral part (=truncate)
template< typename T > static inline T Trunc( T f ) {
	return f < (T)0 ? (T)ceil(f) : (T)floor(f);
}

// round
template< typename T > static inline T Round( T f ) {
	return Floor( f + (T)0.5 );
}

template< typename T > static inline T Frac( T f )
{
	// return fractional part
	return f - Floor(f);
}

// modulo
template< typename T > static inline T Mod( T x, T y ) {
	return (T)fmod( x, y );
}

// NOTE: truncation/rounding may not handle 0.5 as expected (performance reasons)
// this truncates float to integer
static inline Int FloatToInt( Float f ) {
	return (Int)Floor(f);
}

// half float support
Float HalfToFloat( UShort h );
UShort FloatToHalf( Float f );

static inline Int RoundFloatToInt( Float f )
{
#if 1
	// reference: http://stereopsis.com/sree/fpu2006.html
	// seems 6x faster than default in msc
	// question is what if internal FPU rounding is set to non-default? will it still work?
	// => must investigate, also should measure performance diff on ARM
	// ok, 1.8x faster on ARM (without check), with check it's actually 1.1% slower
	KWLKIT_ASSERT( Abs(f) <= Float(Limits<Int>::MAX) );
	/*if ( KWLKIT_UNLIKELY( Abs(f) >= Float(Limits<Int>::MAX) ) ) {
		return Limits<Int>::MAX * ((f>0)*2 - 1);
	}*/
	Double tmp = Double(f) + 6755399441055744.0;
	return CastTo<Int *>(&tmp)[Endian::IsBig()];
#else
	return FloatToInt(f + 0.5f);
#endif
}

static inline Float IntToFloat( Int i ) {
	return (Float)i;
}

// this truncates double to integer
static inline Int DoubleToInt( Double d )
{
	// TODO: better
	return (Int)Floor(d);
}

static inline Int RoundDoubleToInt( Double d ) {
	return DoubleToInt(d + 0.5);
}

static inline Double IntToDouble( Int i ) {
	return (Double)i;
}

static inline bool IsAlmostZero( Float f, Float eps = Limits<Float>::EpsilonDenormal() ) {
	return Abs(f) <= eps;
}

static inline bool IsAlmostZero( Double d, Double eps = Limits<Double>::EpsilonDenormal() ) {
	return Abs(d) <= eps;
}

static inline bool IsAlmostEqual( Float f, Float g, Float eps = Limits<Float>::EpsilonDenormal() ) {
	return Abs(f-g) <= eps;
}

static inline bool IsAlmostEqual( Double d, Double e, Float eps = Limits<Float>::EpsilonDenormal() ) {
	return Abs(d-e) <= eps;
}

static inline bool IsMax( Float f ) {
	return f != f || Abs(f) >= Limits<Float>::Max();
}

static inline bool IsMax( Double d ) {
	return d != d || Abs(d) >= Limits<Double>::Max();
}

}
