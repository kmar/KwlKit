
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "WavFile.h"
#include "../Resample/Resampler.h"

namespace KwlKit
{

class WavRead
{
public:
	explicit WavRead();
	~WavRead();

	// open wav file
	bool Open( Stream &s, bool owned = 0 );
	// open owned
	bool OpenOwn( Stream *s );

	// set looping (default: 0)
	void SetLooping( bool looping = 0 );

	bool SetSampleRate( Int srate );
	bool SetFormat( UInt fmt, Int nchannels );

	// note: if not enough samples can be filled, the rest is filled with zeros upon success
	// however, nread still returns number of samples read from wav file
	bool ReadSamples( void *buffer, Int samples, Int &nread );

	// rewind (for loop-streaming)
	bool Rewind();

	bool Close();

	inline bool IsOpen() const {
		return isOpen;
	}

	// get original wav file info
	Int GetWavNumChannels() const;
	Int GetWavSampleRate() const;

	// get length in seconds (currently this only works for kwl input)
	// returns -1 on error
	Float GetLength() const;
	// get current playback position
	Float GetPosition() const;
	// playback done?
	bool IsDone() const;
	// is looping?
	bool IsLooping() const;

	// (very slow, can break playback!)
	bool SeekPosition( Float pos );

private:
	// current playback position
	Long position;
	WavFile wf;
	Resampler *resampler;
	LinearResampler linResampler;
	Int sampleRate;
	UInt sampleFormat;
	Int numChannels;
	bool resInit;
	bool doLoop;
	bool isOpen;
	bool doneFlag;

	bool RewindInternal();
};

}
