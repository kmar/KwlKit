
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Stream.h"
#include "../Base/NoCopy.h"
#include "../Base/Array.h"
#include "../Sample/SampleFormat.h"
#include "../Kwl/KwlFile.h"

namespace KwlKit
{

class WavFile : public NoCopy
{
public:
	enum CompressionFormat
	{
		FORMAT_COMPRESSION_UNKNOWN = 0,
		FORMAT_COMPRESSION_PCM = 1,
		FORMAT_COMPRESSION_FLOAT = 3,

		FORMAT_COMPRESSION_KWL = 0x10000
	};

	static inline Byte GetFormatBytes( UInt samFormat ) {
		return (Byte)(samFormat & SAMPLE_FORMAT_SIZE_MASK);
	}

	WavFile();
	~WavFile();
	// open for reading
	bool Open( Stream &s, bool owned = 0 );

	bool Close();

	// read samples (only wavs open for reading)
	// always interleaved!
	bool ReadSamples( void *buf, Int numSamples, Int numChannels, Int &numSamplesRead,
		UInt samFormat = SAMPLE_FORMAT_16S );

	// write samples (only wavs open for writing)
	// always interleaved!
	bool WriteSamples( const void *buf, Int numSamples, Int numChannels, Int &numSamplesWritten,
		UInt samFormat = SAMPLE_FORMAT_16S );

	// rewind (for loop-streaming)
	bool Rewind();

	inline Int GetNumChannels() const {
		return format.numChannels;
	}

	inline Int GetSampleRate() const {
		return format.sampleRate;
	}

	struct FormatChunk
	{
		UShort compression;
		UShort numChannels;
		UInt sampleRate;
		UInt avgBytesPerSecond;
		UShort blockAlign;
		UShort bitsPerSample;
		UShort extraBytes;

		void FromLittle();
	};

	// currently this only works when reading kwl files
	// return -1 on error
	Float GetLength() const;
private:
	bool ParseHeader();
	bool UnpackData( Byte *&buf, Int &numSamples, Int numChannels, Int &numSamplesRead,
		UInt samFormat = SAMPLE_FORMAT_16S );
	void ResetState();

	Stream *stream;							// refptr
	Stream *ownedStream;					// owned (if any)
	FormatChunk format;

	// for decoding:
	Long silentSamples;
	Long bytesLeft;							// bytes left wrt current stream position
	Long dataBytesLeft;						// data chunk bytes left
	Array< Byte > blockBuffer;

	// kwl support
	KwlFile *kwl;

	// sample format for wave (no meaning for compressed formats such as ADPCM)
	UInt wavSamFormat;
};

}
