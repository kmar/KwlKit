
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "SampleUtil.h"
#include "../Base/Assert.h"
#include "../Base/Templates.h"
#include "../Base/Endian.h"
#include "../Base/Memory.h"
#include "../Base/Math.h"

namespace KwlKit
{

// SampleConv

// FIXME: this is currently rather messy and slow; rewrite
void SampleConv::Convert( UInt srcFmt, Int srcChannels, UInt dstFmt, Int dstChannels,
		const void *src, void *dst, Int samples )
{
	KWLKIT_ASSERT( !(srcFmt & SAMPLE_FORMAT_UNSIGNED) && !(dstFmt & SAMPLE_FORMAT_UNSIGNED) );
	KWLKIT_ASSERT( samples > 0 && src && dst && srcChannels > 0 && dstChannels > 0 );
	Int srcBps = srcFmt & SAMPLE_FORMAT_SIZE_MASK;
	Int dstBps = dstFmt & SAMPLE_FORMAT_SIZE_MASK;
	KWLKIT_ASSERT( srcBps > 0 && dstBps > 0 );

	if ( srcFmt == dstFmt && srcChannels == dstChannels ) {
		// fast-copy
		MemCpy( dst, src, samples * srcChannels * srcBps );
		return;
	}

	Byte *d = static_cast<Byte *>(dst);
	const Byte *s = static_cast<const Byte *>(src);
	Int commonChannels = Min( srcChannels, dstChannels );
	while ( samples-- > 0 ) {
		for ( Int i=0; i<commonChannels; i++, s += srcBps, d += dstBps ) {
			if ( srcFmt == dstFmt ) {
				// same format, fast
				for ( Int j=0; j<srcBps; j++ ) {
					d[j] = s[j];
				}
				continue;
			}
			// convert one sample
			// we use normalized Floats as intermediate representation
			Int isam;
			Float fsam;
			switch( srcBps ) {
			case 1:
				fsam = (Float)(*CastTo<const SByte *>(s))*(1.0f/128.0f);
				break;
			case 2:
				fsam = (*CastTo<const Short *>(s)) * (1.0f/32768.0f);
				break;
			case 3:
				isam = s[0] + s[1]*256 + s[2]*65536;
				isam -= 0x1000000 * (isam >= 0x800000);
				fsam = (Float)isam * (1.0f / 0x800000);
				break;
			case 4:
				fsam = *CastTo<const Float *>(s);
				break;
			default:
				fsam = 0;
			}
			// FIXME: can avoid clamping?!
			switch( dstBps ) {
			case 1:
				*CastTo<SByte *>(d) = (SByte)Clamp( RoundFloatToInt(fsam*128), -128, 127 );
				break;
			case 2:
				*CastTo<Short *>(d) = (Short)Clamp( RoundFloatToInt(fsam*32768), -32768, 32767 );
				break;
			case 3:
				isam = (Int)Clamp( RoundFloatToInt(fsam*0x800000), -0x800000, 0x7fffff );
				d[0] = isam & 255;
				d[1] = (isam / 256) & 255;
				*CastTo<SByte *>(d+2) = (SByte)(isam / 65536);
				break;
			case 4:
				*CastTo<Float *>(d) = fsam;
				break;
			}
		}
		s += srcBps * (srcChannels - commonChannels);
		for ( Int i = commonChannels; i < dstChannels; i++ ) {
			// dst is tricky, we want to replicate mono samples into both channels, all other are currently left empty;
			// this may change in the future...
			if ( i == 1 ) {
				// copy previous sample
				for ( Int j=0; j<dstBps; j++ ) {
					*d = d[-dstBps];
					d++;
				}
				continue;
			}
			// zero-fill otherwise
			for ( Int j=0; j<dstBps; j++ ) {
				*d++ = 0;
			}
		}
	}
}

}
