
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "../Base/Endian.h"
#include "../Base/Memory.h"
#include "../Base/Stream.h"
#include "../Base/Math.h"
#include "../Base/Templates.h"
#include "../Base/Likely.h"
#include "../Mdct/DspWindows.h"

#include "../Compress/InflateStream.h"

#include "../Sample/SampleFormat.h"
#include "../Sample/SampleUtil.h"
#include "KwlFile.h"

namespace KwlKit
{

// KwlFile::Header

void KwlFile::Header::FromLittle()
{
	Endian::FromLittle( version );
	Endian::FromLittle( flags );
	Endian::FromLittle( sampleRate );
	Endian::FromLittle( blockSize );
	Endian::FromLittle( numFrames );
	Endian::FromLittle( lastFrameSamples );
	Endian::FromLittle( powScl );
	Endian::FromLittle( numSamples );
}

// KwlFile

// 0.2f (old, new is 0.425f)
static const Float KWL_POW_SCL = 0.2f;

KwlFile::KwlFile() : stream(0), ownedStream(0), powScl(KWL_POW_SCL), remSamples(0),
	outMdct(0), inflate(0), outBuffPtr(0), outBase(0), outXor(0), mdctNorm(0) {
	MemSet( &hdr, 0, sizeof(hdr) );
}

KwlFile::~KwlFile() {
	Close();
}

// open for reading
bool KwlFile::Open( Stream &s, bool owned )
{
	KWLKIT_RET_FALSE( Close() );
	stream = &s;
	if ( owned ) {
		ownedStream = &s;
	}
	return ParseHeader();
}

bool KwlFile::ParseHeader()
{
	KWLKIT_ASSERT( stream );
	KWLKIT_RET_FALSE( stream->Read( &hdr, sizeof(hdr) ) );
	hdr.FromLittle();
	KWLKIT_RET_FALSE( MemCmp( hdr.magic, "kwl\x1a", 4 ) == 0 );
	KWLKIT_RET_FALSE( hdr.version == 0x100 );
	KWLKIT_RET_FALSE( hdr.blockSize && (1 << Log2Size((Int)hdr.blockSize)) == hdr.blockSize );
	KWLKIT_RET_FALSE( hdr.numChannels > 0 );
	KWLKIT_RET_FALSE( hdr.sampleRate > 0 );
	if ( hdr.flags & KWL_NORMALIZED ) {
		remSamples = hdr.numSamples;
	}
	// TODO: check all parameters for validity
	outQBuffer.Resize( hdr.blockSize );
	bool old = !(hdr.flags & KWL_NORMALIZED);
	if ( !outMdct || (outMdct->GetN() != hdr.blockSize*2 || mdctNorm == old) ) {
		Float two_n = 2.0f / (hdr.blockSize*2);
		delete outMdct;
		outMdct = new Mdct<Float>( hdr.blockSize*2, old ? 1.0f : 2.0f*two_n, old ? two_n : 0.5f );
		mdctNorm = !old;
	}
	outMdct->SetWindowFunc( VorbisWindow );

	Int channels = hdr.numChannels;
	outFloatBuf.Resize( (Int)hdr.numChannels * hdr.blockSize * 4 );

	outBase = outXor = (Int)hdr.blockSize * 2;

	outMdctBuf.Resize( hdr.blockSize );
	outFloatBuf.MemSet( 0 );
	outMdctBuf.MemSet( 0 );
	finalOut.Resize( channels * hdr.blockSize );
	finalOut.MemSet( 0 );

	if ( !inflate ) {
		inflate = new InflateStream;
	}
	KWLKIT_RET_FALSE( inflate->SetStream( *stream, 0 ) );
	inflate->SetFormat( INF_ZLIB );
	inflate->Rewind();
	inflate->GetInflate().GetStream().GetStream()->SkipRead( sizeof(Header) );

	powScl = KWL_POW_SCL;
	if (hdr.powScl) {
		powScl = Float(hdr.powScl)/65536.0f;
	}
	InitDequantTable();

	if ( hdr.numFrames ) {
		// prime output
		KWLKIT_RET_FALSE( DecompressFrame() );
	}
	outBuffPtr = hdr.blockSize;

	return 1;
}

bool KwlFile::Close()
{
	if ( !stream ) {
		return 1;
	}
	bool res = 1;
	delete outMdct;
	outMdct = 0;
	delete inflate;
	inflate = 0;
	stream = 0;
	delete ownedStream;
	ownedStream = 0;
	ResetState();
	return res;
}

void KwlFile::ResetState()
{
	remSamples = 0;
	outBuffPtr = hdr.blockSize;
	outFloatBuf.MemSet( 0 );
	outMdctBuf.MemSet( 0 );
}

bool KwlFile::ReadFloat( Stream &s, Float &f ) const
{
	if ( hdr.flags & KWL_HALF_FLOAT ) {
		UShort hf;
		KWLKIT_RET_FALSE( s.Read( &hf, sizeof(UShort) ) );
		Endian::FromLittle( hf );
		f = HalfToFloat( hf );
		return 1;
	}
	KWLKIT_RET_FALSE( s.Read( &f, sizeof(Float) ) );
	Endian::FromLittle( f );
	return 1;
}

bool KwlFile::DecompressFrame()
{
	for ( Int i=0; i<hdr.numChannels; i++ ) {
		Float scl;
		KWLKIT_RET_FALSE( ReadFloat(*inflate, scl) );
		Int delta = (hdr.flags & KWL_DC_OFFSET) != 0;
		outQBuffer[0] = 0;
		KWLKIT_RET_FALSE( inflate->Read( outQBuffer.GetData()+delta, outQBuffer.GetSize()-delta ) );
		Decompress( outQBuffer.GetData(),
			outMdctBuf.GetData(), hdr.blockSize, scl );
		if ( hdr.flags & KWL_DC_OFFSET ) {
			KWLKIT_RET_FALSE( ReadFloat(*inflate, outMdctBuf[0] ) );
		}
		outMdct->DoIMdct( outMdctBuf.GetData(),
			outFloatBuf.GetData() + 4*i*hdr.blockSize + outBase
		);
		outMdct->OverlapAdd( outFloatBuf.GetData() + 4*i*hdr.blockSize + (outBase ^ outXor),
			outFloatBuf.GetData() + 4*i*hdr.blockSize + outBase, finalOut.GetData() + i*hdr.blockSize );
	}
	// shift buffers
	outBase ^= outXor;
	outBuffPtr = 0;
	return 1;
}

template<typename T>
struct ConvSampleFromFloat
{
};

template<>
struct ConvSampleFromFloat<Short>
{
	static inline Short Convert( Float sam ) {
		return (Short)Clamp( RoundFloatToInt( sam * 32768.0f), -32768, 32767 );
	}
};

template<>
struct ConvSampleFromFloat<Float>
{
	static inline Float Convert( Float sam ) {
		return sam;
	}
};

template< typename T >
void KwlFile::ConvSamplesFastPath( Int minChan, Int numChannels, Int rem, Int dstBps, Byte *&bout )
{
	Int j;
	for ( j=0; j<minChan; j++ ) {
		const Float *src = finalOut.GetData() + j * hdr.blockSize + outBuffPtr;
		T *dst = CastTo<T *>(bout) + j;
		for ( Int i=0; i<rem; i++ ) {
			*dst = ConvSampleFromFloat<T>::Convert(*src++);
			dst += numChannels;
		}
	}
	if ( j == 1 && j < numChannels ) {
		// handle mono=>stereo expansion
		T *dst = CastTo<T *>(bout) + j;
		for ( Int i=0; i<rem; i++ ) {
			*dst = dst[-1];
			dst += numChannels;
		}
		j++;
	}
	for ( ; j<numChannels; j++ ) {
		T *dst = CastTo<T *>(bout) + j;
		for ( Int i=0; i<rem; i++ ) {
			*dst = 0;
			dst += numChannels;
		}
	}
	outBuffPtr += rem;
	bout += rem * dstBps;
}

bool KwlFile::ReadSamples( void *buf, Int numSamples, Int numChannels, Int &numSamplesRead, UInt samFormat )
{
	numSamplesRead = 0;
	if ( KWLKIT_UNLIKELY( !stream || numSamples < 0 || numChannels <= 0 ) ) {
		return 0;
	}
	if ( KWLKIT_UNLIKELY( numSamples == 0 ) ) {
		return 1;
	}
	if ( KWLKIT_UNLIKELY( !buf ) ) {
		return 0;
	}
	Int minChan = Min( (Int)hdr.numChannels, numChannels );
	Byte *bout = (Byte *)buf;
	Int dstBps = (samFormat & SAMPLE_FORMAT_SIZE_MASK) * numChannels;
	while ( numSamples > 0 ) {
		Int rem = hdr.blockSize - outBuffPtr;
		if ( KWLKIT_UNLIKELY( rem <= 0 ) ) {
			bool res = DecompressFrame();
			if ( !res ) {
				break;
			}
			rem = hdr.blockSize;
		}
		rem = Min( rem, numSamples );
		numSamplesRead += rem;
		numSamples -= rem;

		// use fast path for two most common cases (16-bit signed and float)
		if ( samFormat == SAMPLE_FORMAT_16S ) {
			ConvSamplesFastPath<Short>( minChan, numChannels, rem, dstBps, bout );
			continue;
		}

		if ( samFormat == SAMPLE_FORMAT_32F ) {
			ConvSamplesFastPath<Float>( minChan, numChannels, rem, dstBps, bout );
			continue;
		}

		// otherwise use slow path
		while ( rem-- > 0 ) {
			// unpack n float samples
			Int j;
			Float unpSam[ 256 ];
			for ( j=0; j<minChan; j++ ) {
				unpSam[ j ] = finalOut[ j * hdr.blockSize + outBuffPtr ];
			}
			for ( ; j < numChannels; j++ ) {
				// handle mono=>stereo expansion
				unpSam[j] = (j==2) ? unpSam[0] : 0;
			}
			// now convert to output (slow!)
			SampleConv::Convert( SAMPLE_FORMAT_32F, numChannels, samFormat, numChannels, unpSam, bout, 1 );
			bout += dstBps;
			outBuffPtr++;
		}
	}
	if ( hdr.flags & KWL_NUM_SAMPLES ) {
		if ( (UInt)numSamplesRead > remSamples ) {
			numSamplesRead = (Int)remSamples;
		}
		remSamples -= numSamplesRead;
	}
	return 1;
}

void KwlFile::Decompress( const Byte *qbuf, Float *buf, Int size, Float scl )
{
	Int msk = (1 << hdr.quantBits)-1;
	for ( Int i=0; i<size; i++ ) {
		buf[i] = dequantTbl[ qbuf[i] & msk ] * scl;
	}
}

// rewind (for loop-streaming)
bool KwlFile::Rewind()
{
	KWLKIT_RET_FALSE( stream->Rewind() );
	ResetState();
	return ParseHeader();
}

void KwlFile::InitDequantTable()
{
	Int qsize = 1 << hdr.quantBits;
	dequantTbl.Resize( qsize );
	Int qbase = qsize >> 1;
	Int qmax = qbase - 1;
	Float invqofs = 1.0f / ((Float)qmax + 0.5f*!(hdr.flags & KWL_NO_QBIAS));
	for ( Int i=0; i<qsize; i++ ) {
		Int sb = i - qbase;
		Float sam = sb * invqofs;
		sam = Pow( Abs(sam), 1.0f/powScl ) * Sign(sam);
		dequantTbl[ i ] = sam;
	}
}

Float KwlFile::GetLength() const
{
	if ( hdr.flags & KWL_NUM_SAMPLES ) {
		return Float( Double(hdr.numSamples) / hdr.sampleRate );
	}
	Float totalSam = (Float)hdr.blockSize * hdr.numFrames;
	return totalSam /= hdr.sampleRate;
}

}
