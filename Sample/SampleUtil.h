
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "SampleFormat.h"
#include "../Base/Templates.h"

namespace KwlKit
{

// 16-bit sample utils
// we assume that right shift by n uses arithmetic shift, this is however implementation defined
// of course this introduces a problem that negative values are not rounded towards zero,
// but it gives a ~50% on x86 platform, I simply cannot ignore this...
struct SampleUtil
{
	// lerp two samples (alpha: 0 = 100%a, 255 = 99.99%b
	// note that this is perfectly ok because we don't want 100% b
	static inline Short LerpSamples( Short a, Short b, Byte alpha ) {
		return (Short)( a + ((b - a)*alpha >> 8) );
	}

	// 16-bit version, 0..65535 (we have to drop LSBit to avoid overflow)
	static inline Short LerpSamples16( Short a, Short b, UShort alpha ) {
		return (Short)( a + ((b - a)*(alpha>>1) >> 15) );
	}

	// fast int version
	// FIXME: no longer fast... because of overfow! => had to do the same
	static inline Int LerpSamples16Int( Int a, Int b, Int alpha ) {
		return a + ((b - a)*(alpha>>1) >> 15);
	}

	// scale sample (scl: 0=0%, 255=100%)
	static inline Short ScaleSample( Short s, Byte scl ) {
		return (Short)( (s * (scl+1)) >> 8 );
	}

	// scale sample (scl: 0=0%, 65535=100%)
	static inline Short ScaleSample16( Short s, UShort scl ) {
		return (Short)( (s * (scl+1)) >> 16 );
	}

	// approx scale sample, 65536 = 100%
	static inline Int ScaleSample16Approx( Int s, Int scl ) {
		return ( s * scl ) >> 16;
	}

	// branchless: add two samples with saturation
	// note: slower than branching if no clipping occurs
	static inline Short AddSamplesSaturated( Short a, Short b )
	{
		Int tmp = a+b;
		tmp += (tmp > 32767)*(32767 - tmp);
		tmp += (tmp < -32768)*(32768 - tmp);
		return (Short)tmp;
	};

	static inline Short Saturate( Int s )
	{
		s += (s > 32767)*(32767 - s);
		s += (s < -32768)*(32768 - s);
		return (Short)s;
	};

};

struct SampleConv
{
	static inline Int LoadSample24( const void *buf ) {
		const Byte *b = static_cast<const Byte *>(buf);
		Int res = b[0] | (b[1] << 8) | (b[2] << 16);
		res -= 0x1000000 * (res >= 0x800000);
		return res;
	}
	static inline void StoreSample24( Int sam, void *buf ) {
		Byte *b = static_cast<Byte *>(buf);
		b[0] = sam & 255;
		b[1] = (sam / 256) & 255;
		*CastTo<SByte *>(b+2) = (SByte)(sam / 65536);
	}
	// convert sample data (performance note: uses floats as intermediate representation; may be slow)
	// always assume signed data!
	// always assume interleaved channels!
	// must use native endianness (except for 24-bit samples which must use little endianness)
	// multi-byte samples (except for 24-bit) must be properly aligned
	// converting stereo => mono always picks left channels; neither solution is ideal,
	// ideally one would choose from L/R/Mix but this keeps things simple
	static void Convert( UInt srcFmt, Int srcChannels, UInt dstFmt, Int dstChannels,
		const void *src, void *dst, Int samples );
};

}
