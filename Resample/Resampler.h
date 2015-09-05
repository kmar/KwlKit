
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Array.h"
#include "../Sample/SampleFormat.h"

namespace KwlKit
{

class Resampler
{
public:
	inline Resampler() : sampleFormat(0), numChannels(0) {}

	virtual void SetFormat( UInt samFmt, Int nchannels ) = 0;
	virtual void SetInputSampleRate( Int newSampleRate ) = 0;
	virtual void SetOutputSampleRate( Int newSampleRate ) = 0;

	virtual void Reset() = 0;
	// compute number of needed input samples (note: may return 0!)
	virtual Int ComputeNeededSamples( Int outSamples ) const = 0;
	// get input buffer (fill with needed samples)
	virtual void *GetInputBuffer( Int samples ) = 0;
	// get resampled result
	virtual void Resample( void *buf, Int samples ) = 0;
	// get sample size in bytes
	inline Int GetSampleSize() const {
		return (sampleFormat & SAMPLE_FORMAT_SIZE_MASK) * numChannels;
	}
	inline UInt GetSampleFormat() const {
		return sampleFormat;
	}
	inline Int GetNumChannels() const {
		return numChannels;
	}

	// compute needed output samples to roughly match input (this is the reverse task)
	// note that real inSamples may be less!
	virtual Int ComputeNeededOutputSamples( Int inSamples ) const = 0;
protected:
	UInt sampleFormat;
	Int numChannels;
};

class LinearResampler : public Resampler
{
public:
	using Resampler::GetSampleSize;
	using Resampler::GetSampleFormat;
	using Resampler::GetNumChannels;

	LinearResampler();

	void SetFormat( UInt samFmt, Int nchannels );
	void SetInputSampleRate( Int newSampleRate );
	void SetOutputSampleRate( Int newSampleRate );
	// use lowpass filter for downsampling? (default is off as linear resampler should be fast)
	void SetFilter( bool enable );

	void Reset();
	// compute number of needed input samples (note: may return 0!)
	Int ComputeNeededSamples( Int outSamples ) const;
	// get input buffer (fill with needed samples)
	void *GetInputBuffer( Int samples );
	// get resampled result
	void Resample( void *buf, Int samples );

	// compute needed output samples to roughly match input (this is the reverse task)
	// note that real inSamples may be less!
	Int ComputeNeededOutputSamples( Int inSamples ) const;
private:
	// special case when downsampling
	void Downsample( Byte *b, Int samples );

	Int inSampleRate, outSampleRate;
	Int upSampleRate;		// for downsampling
	Array<Byte> inputBuffer;
	Int bufferedSamples;
	UInt posHi, stepHi;
	UShort posLo, stepLo;

	// for downsampling!
	Array< Int > downSum;
	Array< Float > downSumFloat;
	UInt downStepHi;
	UInt downDiv;
	UShort downStepLo;

	// constants (unity build)
	static const bool USE_ADVANCED_DOWNSAMPLING;
};

}
