
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "WavRead.h"
#include "../Base/Memory.h"

namespace KwlKit
{

// WavRead

WavRead::WavRead() : position(0), resampler(&linResampler), sampleRate(44100), sampleFormat(SAMPLE_FORMAT_16S),
	numChannels(2), resInit(0), doLoop(0), isOpen(0), doneFlag(0) {
}

WavRead::~WavRead() {
}

// open wav file
bool WavRead::Open( Stream &s, bool owned )
{
	bool res = wf.Open( s, owned );
	isOpen = res;
	doneFlag = !res;
	resInit = 0;
	return res;
}

// open owned
bool WavRead::OpenOwn( Stream *s )
{
	KWLKIT_ASSERT( s );
	return Open( *s, 1 );
}

bool WavRead::SetSampleRate( Int srate )
{
	if ( sampleRate == srate ) {
		return 1;
	}
	if ( srate <= 0 ) {
		return 0;
	}
	sampleRate = srate;
	resInit = 0;
	return 1;
}

bool WavRead::SetFormat( UInt fmt, Int nchannels )
{
	if ( fmt == sampleFormat && nchannels == numChannels ) {
		return 1;
	}
	if ( nchannels < 1 ) {
		return 0;
	}
	sampleFormat = fmt;
	numChannels = nchannels;
	resInit = 0;
	return 1;
}

// set looping (default: 0)
void WavRead::SetLooping( bool looping )
{
	doLoop = looping;
}

bool WavRead::Rewind()
{
	doneFlag = 0;
	return RewindInternal();
}

bool WavRead::RewindInternal()
{
	KWLKIT_RET_FALSE( isOpen );
	position = 0;
	return wf.Rewind();
}

bool WavRead::Close()
{
	isOpen = 0;
	doneFlag = 1;
	position = 0;
	return wf.Close();
}

bool WavRead::ReadSamples( void *buffer, Int samples, Int &nread )
{
	KWLKIT_RET_FALSE( isOpen );
	Byte *b = static_cast<Byte *>(buffer);
	Int samSz = (sampleFormat & SAMPLE_FORMAT_SIZE_MASK);
	bool doResample = wf.GetSampleRate() != sampleRate;
	if ( doResample ) {
		if ( !resInit ) {
			resampler->SetInputSampleRate( wf.GetSampleRate() );
			resampler->SetOutputSampleRate( sampleRate );
			resampler->SetFormat( sampleFormat, numChannels );
			resInit = 1;
		}
		// compute how many samples we need
		Int needSam = resampler->ComputeNeededSamples( samples );

		Byte *b = static_cast<Byte *>(resampler->GetInputBuffer(needSam));
		if ( !wf.ReadSamples( b, needSam, numChannels, nread, sampleFormat ) ) {
			return 0;
		}
		position += nread;
		if ( nread < needSam ) {
			b += nread * samSz * numChannels;
			doneFlag = 1;
			if ( doLoop ) {
				Int nr2;
				if ( RewindInternal() && wf.ReadSamples( b, needSam - nread, numChannels, nr2, sampleFormat ) ) {
					nread += nr2;
					b += nr2 * samSz * numChannels;
					position += nr2;
				}
			}
			// zero-fill the rest
			MemSet( b, 0, (needSam - nread) * samSz * numChannels );
		}
		resampler->Resample( buffer, samples );
		nread = samples;
		return 1;
	}
	if ( !wf.ReadSamples( buffer, samples, numChannels, nread, sampleFormat ) ) {
		return 0;
	}
	position += nread;
	if ( nread < samples ) {
		b += nread * samSz * numChannels;
		doneFlag = 1;
		if ( doLoop ) {
			Int nr2;
			if ( RewindInternal() && wf.ReadSamples( b, samples - nread, numChannels, nr2, sampleFormat ) ) {
				nread += nr2;
				position += nr2;
				b += nr2 * samSz * numChannels;
			}
		}
		// zero-fill the rest
		MemSet( b, 0, (samples - nread) * samSz * numChannels );
	}
	return 1;
}

Int WavRead::GetWavNumChannels() const {
	return wf.GetNumChannels();
}

Int WavRead::GetWavSampleRate() const {
	return wf.GetSampleRate();
}

Float WavRead::GetLength() const {
	return wf.GetLength();
}

Float WavRead::GetPosition() const {
	return Float(position)/wf.GetSampleRate();
}

bool WavRead::IsDone() const {
	return doneFlag;
}

bool WavRead::IsLooping() const {
	return doLoop;
}

bool WavRead::SeekPosition( Float pos )
{
	if ( !isOpen ) {
		return 0;
	}
	Long targetPosition = Long(RoundFloatToInt(pos)) * wf.GetSampleRate();
	if ( targetPosition < position ) {
		RewindInternal();
	}
	const Int bsize = 4096;
	Array< Short > rbuf;
	rbuf.Resize( bsize * wf.GetNumChannels() );
	Long togo = targetPosition - position;
	while ( togo > 0 ) {
		Int nread;
		Int toread = bsize;
		if ( togo < toread ) {
			toread = Int(togo);
		}
		if ( !wf.ReadSamples( rbuf.GetData(), toread, wf.GetNumChannels(), nread, SAMPLE_FORMAT_16S ) ) {
			return 0;
		}
		position += nread;
		togo -= nread;
	}
	return 1;
}

}
