
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "WavFile.h"
#include "../Base/Memory.h"
#include "../Base/Endian.h"
#include "../Sample/SampleUtil.h"

namespace KwlKit
{

// WavFile::FormatChunk

void WavFile::FormatChunk::FromLittle()
{
	Endian::FromLittle( compression );
	Endian::FromLittle( numChannels );
	Endian::FromLittle( sampleRate );
	Endian::FromLittle( avgBytesPerSecond );
	Endian::FromLittle( blockAlign );
	Endian::FromLittle( bitsPerSample );
	Endian::FromLittle( extraBytes );
}

// WavFile

WavFile::WavFile() : stream(0), ownedStream(0), silentSamples(0), bytesLeft(0), dataBytesLeft(0),
	kwl(0), wavSamFormat(0)
{
	MemSet( &format, 0, sizeof(format) );
}

WavFile::~WavFile() {
	Close();
}

bool WavFile::Open( Stream &s, bool owned )
{
	if ( !Close() ) {
		return 0;
	}
	stream = &s;
	if ( owned ) {
		ownedStream = &s;
	}
	return ParseHeader();
}

bool WavFile::Close()
{
	if ( !stream ) {
		return 1;
	}
	delete kwl;
	kwl = 0;
	stream = 0;
	delete ownedStream;
	ownedStream = 0;
	ResetState();
	return 1;
}

void WavFile::ResetState()
{
	MemSet( &format, 0, sizeof(format) );
	bytesLeft = 0;
	dataBytesLeft = 0;
	silentSamples = 0;
	wavSamFormat = 0;
}

// rewind (for loop-streaming)
bool WavFile::Rewind()
{
	if ( !stream ) {
		return 0;
	}
	if ( kwl ) {
		return kwl->Rewind();
	}
	if ( !stream->Rewind() ) {
		return 0;
	}
	ResetState();
	return ParseHeader();
}

struct RiffHeader
{
	Byte id[4];
	UInt size;
	Byte format[4];
};

struct ChunkHeader
{
	Byte id[4];
	UInt size;
};

bool WavFile::ParseHeader()
{
	KWLKIT_ASSERT( stream );
	RiffHeader rhdr;
	if ( !stream->Read( &rhdr, sizeof(rhdr) ) ) {
		return 0;
	}
	if ( MemCmp( rhdr.id, "RIFF", 4 ) != 0 ) {
		// try to parse kwl
		if ( !kwl ) {
			kwl = new KwlFile;
		}
		KWLKIT_RET_FALSE( stream->Rewind() );
		bool res = kwl->Open( *stream );
		if ( res ) {
			format.sampleRate = (UInt)kwl->GetSampleRate();
			format.numChannels = (UShort)kwl->GetNumChannels();
		}
		return res;
	}
	if ( MemCmp( rhdr.format, "WAVE", 4 ) != 0 ) {
		return 0;
	}

	KWLKIT_ASSERT( format.compression == 0 && dataBytesLeft == 0 && silentSamples == 0 );

	Endian::FromLittle( rhdr.size );
	bytesLeft = rhdr.size - 4;
	while ( bytesLeft > 0 ) {
		// parsing sub-chunks
		ChunkHeader chhdr;
		if ( !stream->Read( &chhdr, sizeof(chhdr) ) ) {
			return 0;
		}
		if ( (bytesLeft -= sizeof(chhdr)) < 0 ) {
			return 0;
		}
		Endian::FromLittle( chhdr.size );
		if ( MemCmp( chhdr.id, "fmt\x20", 4 ) == 0 ) {
			// parse fmt chunk
			Long chBytesLeft = chhdr.size;

			if ( chhdr.size < 16 ) {
				return 0;
			}
			format.extraBytes = 0;
			Int fsz = 16 + 2*(chhdr.size >= 18);
			if ( !stream->Read( &format, fsz ) ) {
				return 0;
			}
			if ( (bytesLeft -= fsz) < 0 ) {
				return 0;
			}
			chBytesLeft -= fsz;
			format.FromLittle();
			if ( !format.sampleRate || !format.numChannels || !format.bitsPerSample ) {
				// bad format
				return 0;
			}
			if ( format.compression != FORMAT_COMPRESSION_PCM && format.compression != FORMAT_COMPRESSION_FLOAT  ) {
				// not PCM => unsupported
				return 0;
			}
			if ( !stream->SkipRead( format.extraBytes ) ) {
				return 0;
			}
			if ( (bytesLeft -= format.extraBytes) < 0 ) {
				return 0;
			}
			if ( (chBytesLeft -= format.extraBytes) < 0 ) {
				return 0;
			}
			// got fmt chunk, done
			// initialize as necessary
			wavSamFormat = 0;
			if ( format.compression == FORMAT_COMPRESSION_PCM ) {
				// FIXME: support more!
				switch( format.bitsPerSample ) {
				case 8:
					wavSamFormat = SAMPLE_FORMAT_8U;
					break;
				case 16:
					wavSamFormat = SAMPLE_FORMAT_16S;
					break;
				case 24:
					wavSamFormat = SAMPLE_FORMAT_24S;
					break;
				}
				if ( format.bitsPerSample != 8 && format.bitsPerSample != 16 && format.bitsPerSample != 24 ) {
					return 0;
				}
			}
			if ( format.compression == FORMAT_COMPRESSION_FLOAT ) {
				if ( format.bitsPerSample != 32 ) {
					return 0;
				}
				wavSamFormat = SAMPLE_FORMAT_32F;
			}
			if ( (bytesLeft -= chBytesLeft) < 0 ) {
				return 0;
			}
			return stream->SkipRead( chBytesLeft );
		}
		if ( !stream->SkipRead( chhdr.size ) ) {
			return 0;
		}
		if ( (bytesLeft -= chhdr.size) < 0 ) {
			return 0;
		}
	}
	return 0;
}

struct ImaChunkHeader
{
	Short sample;
	Byte index;
	Byte pad;

	void FromLittle() {
		Endian::FromLittle( sample );
	}
	void ToLittle() {
		Endian::ToLittle( sample );
	}
};

bool WavFile::UnpackData( Byte *&buf, Int &numSamples, Int numChannels, Int &numSamplesRead, UInt samFormat )
{
	KWLKIT_ASSERT( stream );

	Int srcsbytes = GetFormatBytes( wavSamFormat );

	if ( format.compression == FORMAT_COMPRESSION_PCM || format.compression == FORMAT_COMPRESSION_FLOAT ) {
		Int bytesPerSample = format.bitsPerSample/8 * format.numChannels;
		Int wantBytes = (Int)Min( (Long)(numSamples * bytesPerSample), dataBytesLeft );
		Int samRead = wantBytes / bytesPerSample;
		if ( format.bitsPerSample == 16 && samFormat == SAMPLE_FORMAT_16S && numChannels == format.numChannels ) {
			// direct streaming possible
			if ( wantBytes > 0 && !stream->Read( buf, wantBytes ) ) {
				return 0;
			}
			if ( (dataBytesLeft -= wantBytes ) < 0 ) {
				return 0;
			}
			if ( (bytesLeft -= wantBytes ) < 0 ) {
				return 0;
			}
			buf += wantBytes;
			numSamples -= samRead;
			numSamplesRead += samRead;
			return 1;
		}
		blockBuffer.Resize( wantBytes );
		if ( wantBytes > 0 && !stream->Read( blockBuffer.GetData(), wantBytes ) ) {
			return 0;
		}
		if ( srcsbytes == 1 ) {
			// convert from unsigned
			for ( Int i=0; i<blockBuffer.GetSize(); i++ ) {
				blockBuffer[i] ^= 0x80;
			}
		}
		if ( (dataBytesLeft -= wantBytes ) < 0 ) {
			return 0;
		}
		if ( (bytesLeft -= wantBytes ) < 0 ) {
			return 0;
		}

		SampleConv::Convert( wavSamFormat & ~SAMPLE_FORMAT_UNSIGNED, format.numChannels,
			samFormat, numChannels, blockBuffer.GetData(), buf, samRead );

		buf += wantBytes;
		numSamples -= samRead;
		numSamplesRead += samRead;
		return 1;
	}

	return 0;
}

bool WavFile::ReadSamples( void *buf, Int numSamples, Int numChannels, Int &numSamplesRead, UInt samFormat )
{
	if ( kwl ) {
		return kwl->ReadSamples( buf, numSamples, numChannels, numSamplesRead, samFormat );
	}
	numSamplesRead = 0;
	if ( KWLKIT_UNLIKELY( !stream || numSamples < 0 || numChannels <= 0 || format.compression == 0 ) ) {
		return 0;
	}
	if ( KWLKIT_UNLIKELY( numSamples == 0 ) ) {
		return 1;
	}
	if ( KWLKIT_UNLIKELY( !buf ) ) {
		return 0;
	}

	Byte *b = (Byte *)buf;
	Int sbytes = GetFormatBytes( samFormat );
	KWLKIT_ASSERT( sbytes == 2 );

	while ( numSamples > 0 ) {
		Int silence = (Int)Min( silentSamples, (Long)numSamples );
		if ( silence ) {
			for ( Int i=0; i<silence * numChannels * sbytes; i++ ) {
				*b++ = 0;
			}
			silentSamples -= silence;
			numSamples -= silence;
			numSamplesRead += silence;
			if ( !numSamples ) {
				break;
			}
		}
		if ( dataBytesLeft > 0 ) {
			if ( !UnpackData( b, numSamples, numChannels, numSamplesRead, samFormat ) ) {
				return 0;
			}
			if ( !numSamples ) {
				break;
			}
		}

		if ( bytesLeft == 0 ) {
			// no more data, done
			if ( silentSamples == 0 && dataBytesLeft == 0 ) {
				break;
			}
			continue;
		}

		ChunkHeader chhdr;
		if ( !stream->Read( &chhdr, sizeof(chhdr) ) ) {
			return 0;
		}
		if ( (bytesLeft -= sizeof(chhdr)) < 0 ) {
			return 0;
		}
		Endian::FromLittle( chhdr.size );
		Long chBytesLeft = chhdr.size;
		if ( MemCmp( chhdr.id, "slnt", 4 ) == 0 ) {
			if ( chhdr.size < 4 ) {
				return 0;
			}
			UInt silentSam;
			if ( !stream->Read( &silentSam, 4 ) ) {
				return 0;
			}
			if ( (bytesLeft -= 4) < 0 ) {
				return 0;
			}
			Endian::FromLittle( silentSam );
			silentSamples += silentSam;
			chBytesLeft -= 4;
			if ( !stream->SkipRead( chBytesLeft ) ) {
				return 0;
			}
			continue;
		}
		if ( MemCmp( chhdr.id, "data", 4 ) == 0 ) {
			// ok we have a data chunk now...
			dataBytesLeft += chhdr.size;
			continue;
		}
		if ( !stream->SkipRead( chBytesLeft ) ) {
			return 0;
		}
		if ( (bytesLeft -= chBytesLeft) < 0 ) {
			return 0;
		}
	}
	return 1;
}

Float WavFile::GetLength() const {
	return kwl ? kwl->GetLength() : 0;
}

}
