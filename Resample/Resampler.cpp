
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Resampler.h"
#include "../Base/Memory.h"
#include "../Base/Math.h"
#include "../Sample/SampleUtil.h"

namespace KwlKit
{

// LinearResampler

const bool LinearResampler::USE_ADVANCED_DOWNSAMPLING = 1;

LinearResampler::LinearResampler() : inSampleRate(44100), outSampleRate(44100), upSampleRate(44100),
	bufferedSamples(0), posHi(0), stepHi(0), posLo(0), stepLo(0), downStepHi(0), downDiv(1),
	downStepLo(0)
{
}

void LinearResampler::SetFormat( UInt fmt, Int nchannels )
{
	KWLKIT_ASSERT( fmt && !(fmt & SAMPLE_FORMAT_UNSIGNED) && nchannels > 0 );
	sampleFormat = fmt;
	numChannels = nchannels;
	Reset();
}

void LinearResampler::SetInputSampleRate( Int newSampleRate )
{
	inSampleRate = newSampleRate;
	Reset();
}

void LinearResampler::SetOutputSampleRate( Int newSampleRate )
{
	outSampleRate = newSampleRate;
	Reset();
}

typedef Double PosFloat;

// compute needed output samples to match input (this is the reverse task)
Int LinearResampler::ComputeNeededOutputSamples( Int inSamples ) const
{
	if ( inSampleRate == outSampleRate ) {
		return inSamples;
	}
	// should be able to solve this analytically
	// problem is we don't know lpLo
	Int lpHi = inSamples - 2 + bufferedSamples;
	lpHi = Max( 0, lpHi );

	// lpHi = (x-1) * stepHi + posHi + ((x-1)*stepLo + posLo)/65536;
	// lpHi = (x-1) * step + pos
	// lpHi - pos = (x-1) * step
	// (lpHi - pos)/step = x-1
	// (lpHi - pos)/step - 1 = x
	// x = (lp - pos)/step - 1

	PosFloat d = (PosFloat)lpHi - ((PosFloat)posHi + (PosFloat)posLo/65536.0f);
	d /= (PosFloat)downStepHi + ((PosFloat)downStepLo/65536.0f);
	d -= 1;
	Int res = Max(0, RoundDoubleToInt(d/downDiv));
	while ( res > 0 && ComputeNeededSamples(res) > inSamples ) {
		res--;
	}
	return res;
}

// compute number of needed samples
Int LinearResampler::ComputeNeededSamples( Int outSamples ) const
{
	if ( inSampleRate == outSampleRate ) {
		return outSamples;
	}
	KWLKIT_ASSERT( outSamples > 0 );
	UInt lastPosHi = (outSamples*downDiv-1) * downStepHi + posHi;
	ULong lastPosLo = (ULong)(outSamples*downDiv-1) * downStepLo + posLo;
	lastPosHi += (UInt)(lastPosLo / 65536);
	// must have lastPosHi available
	return Max( 0, (Int)( lastPosHi + 2 - bufferedSamples ) );
}

// fill with input samples
void *LinearResampler::GetInputBuffer( Int samples )
{
	Int sampleSize = (sampleFormat & SAMPLE_FORMAT_SIZE_MASK) * numChannels;
	inputBuffer.Resize( sampleSize * (bufferedSamples + samples ) );
	return inputBuffer.GetData() + bufferedSamples * sampleSize;
}

// get resampled result
void LinearResampler::Resample( void *buf, Int samples )
{
	Int sampleSize = sampleFormat & SAMPLE_FORMAT_SIZE_MASK;
	Int blockSize = sampleSize * numChannels;

	if ( inSampleRate == outSampleRate ) {
		// just copy
		MemCpy( buf, inputBuffer.GetData(), samples * blockSize );
		return;
	}

	Byte *b = static_cast<Byte *>(buf);
	if ( USE_ADVANCED_DOWNSAMPLING && inSampleRate > outSampleRate ) {
		KWLKIT_ASSERT( stepHi > 1 || stepLo > 0 );
		Downsample( b, samples );
		samples = 0;
	}
	while ( samples-- > 0 ) {
		Int idx = (Int)(posHi * blockSize);
		for ( Int ch=0; ch < numChannels; ch++, idx += sampleSize ) {
			switch( sampleFormat )
			{
			case SAMPLE_FORMAT_8S:
				{
					SByte s0 = *CastTo<const SByte *>(&inputBuffer[idx]);
					SByte s1 = *CastTo<const SByte *>(&inputBuffer[idx + blockSize]);
					s0 = SByte(s0 + (s1 - s0)*posLo/65536);
					*CastTo<SByte *>(b) = s0;
					b++;
				}
				break;
			case SAMPLE_FORMAT_16S:
				{
					Short s0 = *CastTo<const Short *>(&inputBuffer[idx]);
					Short s1 = *CastTo<const Short *>(&inputBuffer[idx + blockSize]);
					s0 = SampleUtil::LerpSamples16( s0, s1, posLo );
					*CastTo<Short *>(b) = s0;
					b += 2;
				}
				break;
			case SAMPLE_FORMAT_24S:
				{
					Int s0 = SampleConv::LoadSample24( &inputBuffer[idx] );
					Int s1 = SampleConv::LoadSample24( &inputBuffer[idx + blockSize] );
					s0 += (s1 - s0) * posLo / 65536;
					SampleConv::StoreSample24( s0, b );
					b += 3;
				}
				break;
			case SAMPLE_FORMAT_32F:
				{
					Float s0 = *CastTo<const Float *>(&inputBuffer[idx]);
					Float s1 = *CastTo<const Float *>(&inputBuffer[idx + blockSize]);
					s0 += (s1 - s0)*(Float)posLo*(1.0f/65536.0f);
					*CastTo<Float *>(b) = s0;
					b += 4;
				}
				break;
			}
		}
		// advance...
		posHi += stepHi;
		UInt tmp = posLo + stepLo;
		posHi += tmp >> 16;
		posLo = tmp & 65535;
	}
	// ok done now, adjust buffer
	Int sizeBlocks = inputBuffer.GetSize() / blockSize;
	if ( posLo == 0 ) {
		if ( posHi > 0 && (Int)posHi*blockSize < inputBuffer.GetSize() ) {
			for ( Int i = (Int)posHi; i<sizeBlocks; i++ ) {
				MemCpy( inputBuffer.GetData() + (i-(Int)posHi)*blockSize,
					inputBuffer.GetData() + i*blockSize, blockSize );
			}
			bufferedSamples = sizeBlocks - posHi;
		} else {
			bufferedSamples = 0;
		}
		posHi = 0;
		return;
	}
	UInt ph = posHi - downStepHi;
	Int pl = posLo - downStepLo;
	ph -= pl < 0;
	if ( ph > 0 ) {
		for ( Int i=(Int)ph; i<sizeBlocks; i++ ) {
			MemCpy( inputBuffer.GetData() + (i-(Int)ph)*blockSize, inputBuffer.GetData() + i*blockSize, blockSize );
		}
		inputBuffer.Resize( (sizeBlocks - ph) * blockSize );
		posHi -= ph;
	}
	bufferedSamples = inputBuffer.GetSize() / blockSize;
	return;
}

void LinearResampler::Downsample( Byte *b, Int samples )
{
	KWLKIT_ASSERT( stepHi > 0 );
	Int sampleSize = sampleFormat & SAMPLE_FORMAT_SIZE_MASK;
	Int blockSize = sampleSize * numChannels;

	while ( samples-- > 0 ) {
		for ( UInt s = 0; s < downDiv; s++ ) {
			Int idx = (Int)(posHi * blockSize);
			for ( Int ch=0; ch < numChannels; ch++, idx += sampleSize ) {
				switch( sampleFormat )
				{
				case SAMPLE_FORMAT_8S:
					{
						SByte s0 = *(const SByte *)&inputBuffer[idx];
						SByte s1 = *(const SByte *)&inputBuffer[idx + blockSize];
						s0 = SByte(s0 + (s1 - s0)*posLo/65536);
						downSum[ch] += s0;
					}
					break;
				case SAMPLE_FORMAT_16S:
					{
						Short s0 = *(const Short *)&inputBuffer[idx];
						Short s1 = *(const Short *)&inputBuffer[idx + blockSize];
						s0 = SampleUtil::LerpSamples16( s0, s1, posLo );
						downSum[ch] += s0;
					}
					break;
				case SAMPLE_FORMAT_24S:
					{
						Int s0 = SampleConv::LoadSample24( &inputBuffer[idx] );
						Int s1 = SampleConv::LoadSample24( &inputBuffer[idx + blockSize] );
						s0 += (s1 - s0) * posLo / 65536;
						downSum[ch] += s0;
					}
					break;
				case SAMPLE_FORMAT_32F:
					{
						Float s0 = *(const Float *)&inputBuffer[idx];
						Float s1 = *(const Float *)&inputBuffer[idx + blockSize];
						s0 += (s1 - s0)*(Float)posLo*(1.0f/65536.0f);
						downSumFloat[ch] += s0;
					}
					break;
				}
			}
			// advance...
			posHi += downStepHi;
			UInt tmp = posLo + downStepLo;
			posHi += tmp >> 16;
			posLo = tmp & 65535;
		}
		// store average...
		KWLKIT_ASSERT( downDiv != 0 );
		for ( Int ch=0; ch < numChannels; ch++ ) {
			switch( sampleFormat )
			{
			case SAMPLE_FORMAT_8S:
				*CastTo<SByte *>(b) = (SByte)(downSum[ch] / (Int)downDiv);
				b++;
				downSum[ch] = 0;
				break;
			case SAMPLE_FORMAT_16S:
				*CastTo<Short *>(b) = (Short)(downSum[ch] / (Int)downDiv);
				b += 2;
				downSum[ch] = 0;
				break;
			case SAMPLE_FORMAT_24S:
				SampleConv::StoreSample24( downSum[ch] / (Int)downDiv, b );
				b += 3;
				downSum[ch] = 0;
				break;
			case SAMPLE_FORMAT_32F:
				*CastTo<Float *>(b) = downSumFloat[ch] / downDiv;
				b += 4;
				downSumFloat[ch] = 0;
				break;
			}
		}
	}
}

void LinearResampler::Reset()
{
	inputBuffer.Clear();
	bufferedSamples = 0;
	posHi = 0;
	posLo = 0;
	ULong l = (ULong)inSampleRate * 65536 / outSampleRate;
	stepLo = l & 65535;
	stepHi = (UInt)(l / 65536);
	downStepHi = stepHi;
	downStepLo = stepLo;
	downDiv = 1;
	upSampleRate = inSampleRate;
	if ( USE_ADVANCED_DOWNSAMPLING && inSampleRate > outSampleRate ) {
		KWLKIT_ASSERT( outSampleRate > 0 );
		upSampleRate = ( inSampleRate + outSampleRate - 1 ) / outSampleRate * outSampleRate;
		// downsampling!
		downSum.Resize( numChannels );
		downSumFloat.Resize( numChannels );
		downSum.Fill(0);
		downSumFloat.Fill(0);

		UInt div = upSampleRate / outSampleRate;
		ULong ul = (ULong)stepHi * 65536 + stepLo;
		ul /= div;
		downStepHi = (UInt)(ul / 65536);
		downStepLo = (UInt)ul & 65535;

		KWLKIT_ASSERT( div > 1 );
		downDiv = div;
		KWLKIT_ASSERT( downDiv <= 255 );
	}
}

}
