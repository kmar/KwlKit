
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Types.h"
#include "../Base/Array.h"
#include "../Mdct/Mdct.h"
#include "../Sample/SampleFormat.h"

namespace KwlKit
{
class Stream;
class InflateStream;

// my own simple audio compressed format
// everything is little endian
class KwlFile
{
public:
	enum KwlFlags
	{
		KWL_NORMALIZED		=	1,		// new mode: using normalized MDCT (default now)
		KWL_NUM_SAMPLES		=	2,		// number of original samples stored in header
		KWL_DC_OFFSET		=	4,		// store first value with full quality to avoid DC offset
		KWL_NO_QBIAS		=	8,		// no quantizer bias (seems a tiny bit better in terms of quality)
		KWL_HALF_FLOAT		=	16		// use half-float representation to improve compression
	};
	struct Header
	{
		Byte magic[4];				// kwl, 0x1a
		UShort version;				// 0x100 = 1.0
		UShort flags;				// bit 0: normalized
		UInt sampleRate;
		Byte numChannels;
		Byte quantBits;				// 6 is default
		UShort blockSize;			// MDCT block size, 512 is default (used to be 2048)
		UInt numFrames;				// total number of frames in file
		UShort lastFrameSamples;	// number of samples in last frame (not used)
		UShort powScl;				// fixed point fraction of power scale; 0 = use old (0.2)
		ULong numSamples;			// total number of samples in original file (if KWL_NUM_SAMPLES bit is set)

		void FromLittle();
	};
	// after 32 byte header, compressed blocks follow:
	// block is: float scale, byte block[blockSize]
	// numChannels blocks are stored following each other, together forming a frame
	// blocks are compressed using deflate (zlib format with tiny header and Adler32 checksum)
	// first frame is used to prime iMDCT

	KwlFile();
	~KwlFile();

	// open for reading
	bool Open( Stream &s, bool owned = 0 );
	bool Close();

	// read samples (only when open for reading)
	// always interleaved!
	bool ReadSamples( void *buf, Int numSamples, Int numChannels, Int &numSamplesRead,
		UInt samFormat = SAMPLE_FORMAT_16S );

	// rewind (for loop-streaming)
	bool Rewind();

	inline Int GetSampleRate() const {
		return hdr.sampleRate;
	}

	inline Int GetNumChannels() const {
		return hdr.numChannels;
	}

	// get length in seconds (r/o)
	Float GetLength() const;

private:
	Stream *stream;							// refptr
	Stream *ownedStream;					// owned (if any)
	Header hdr;
	// 0.2
	Float powScl;
	// for accurate decoding
	ULong remSamples;
	// output sample buffer
	Array< Short > outBuffer;
	Array< Float > outFloatBuf;
	Array< Float > outMdctBuf;
	// final output for decompression
	Array< Float > finalOut;
	// quantized output buffer (temporary)
	Array< Byte > outQBuffer;

	Mdct<Float> *outMdct;
	InflateStream *inflate;

	Array< Float > dequantTbl;

	Int outBuffPtr;
	// this avoids copying
	Int outBase;
	Int outXor;
	// mdct normalized mode?
	bool mdctNorm;

	bool DecompressFrame();
	void Decompress( const Byte *qbuf, Float *buf, Int size, Float scl );
	void ResetState();
	bool ParseHeader();
	void InitDequantTable();
	bool ReadFloat( Stream &s, Float &f ) const;

	template< typename T >
	void ConvSamplesFastPath( Int minChan, Int numChannels, Int rem, Int dstBps, Byte *&bout );
};

}
