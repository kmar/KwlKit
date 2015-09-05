
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Math.h"

namespace KwlKit
{

Float HalfToFloat( UShort h )
{
	// TODO: optimize later
	Int s, e, m;
	s = h >> 15;
	e = (h >> 10) & 31;
	m = h & 1023;

	Int sign = 1 - (s << 1);
	const Float mscl = 1.0f/1024.0f;

	if ( e == 31 ) {
		// handle special exponent...
		// m == 0 => infinity
		//   != 0 => NaN
		return m == 0 ? Limits<Float>::Infinity()*sign : Limits<Float>::NaN();
	}
	if ( e == 0 ) {
		// zero or denormal
		return (1.0f / (1 << 14)) * sign * (m*mscl);
	}
	// normalized
	if ( e >= 15 ) {
		return (Float)(1 << (e-15)) * sign * (1.0f + m*mscl );
	}
	return (1.0f / (1 << (15-e))) * sign * (1.0f + m*mscl);
}

// not required by kwl decoder but keeping for completeness
UShort FloatToHalf( Float f )
{
	// handle extreme cases first
	if ( Limits<Float>::IsNan(f) ) {
		return 0x7fff;
	}
	UShort res = (UShort)(f < 0) << 15;
	f = Abs(f);
	if ( f > 65504.0f ) {
		return res | 0x7c00;
	}
	// flush denormals to zero
	if ( f < (1.0f / (1<<14)) ) {
		return res;
	}
	Int e;
	Float m = frexp( f, &e );
	// adjust from <0.5..1) to <1..2)
	e--;
	m *= 2.0f;
	KWLKIT_ASSERT( e > -15 && e <= 15 );
	res |= (UShort)((e+15) << 10);

	/*
	2^e * m = -1^sgn * 2^(pe-15) * (1 + m/1024)
	e = pe - 15;
	pe = e+15;
	fm = 1 + pm/1024
	pm/1024 = fm-1
	pm = (fm-1) * 1024
	*/
	res |= (UShort)FloatToInt( (m - 1.0f) * 1024.0f );
	return res;
}


}
