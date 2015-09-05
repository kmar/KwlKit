
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Fft.h"

// MDCT using FFT reference: http://www.musicdsp.org/showone.php?id=270 (Shuhua Zhang)

namespace KwlKit
{

template< typename T >
struct Mdct
{
	typedef T (*WindowFunc)( Int i, Int n );

	// note on scales: vorbis uses 2/n as prescale and 0.5 as postscale
	// in fact 4/n and 0.5 in windowed mode to maintain appropriate volume
	Mdct( Int n_, const T &pre = (T)-1, const T &post = (T)-1, const T *windowData = 0 )
		: m( Log2Int(n_) ), n(n_), fft(n/4)
	{
		KWLKIT_ASSERT( n > 0 && !(n % 4) );
		KWLKIT_ASSERT( n == (1 << m) );

		// default scaling for windowed MDCT (non-windowed should use 1/n for IMDCT)
		prescale = pre < (T)0 ? (T)1 : pre;
		postscale = post < (T)0 ? (T)2 / n : post;

		// prepare scaled twiddle factors
		twiddle.Resize( n/4 );
		T a = (T)(D_PI * 2 / (8*n));
		T o = (T)(D_PI * 2 / n);
		for ( Int i=0; i<n/4; i++ ) {
			twiddle[i].Expi( -(T)(a + o*i) );
		}
		fftData.Resize( n/4 );
		window.Resize( n );
		SetWindow( windowData );
	}

	// set window data, pass null to set all ones (=no window)
	void SetWindow( const T *windowData ) {
		if ( !windowData ) {
			window.Fill( (T)1 );
			return;
		}
		for ( Int i=0; i<n; i++ ) {
			window[i] = windowData[i];
		}
	}

	// set window using generator function
	void SetWindowFunc( WindowFunc wf ) {
		for ( Int i=0; i<n; i++ ) {
			window[i] = wf( i, n );
		}
	}

	// manually set pre/postscale
	// prescale for MDCT, postscale for IMDCT
	void SetPrescale( const T &pre ) {
		prescale = pre;
	}

	void SetPostscale( const T &post ) {
		postscale = post;
	}

	// reconstruct (overlap-add) two equal-sized chunks
	// data0 and data1 hold N samples, dataOut receives N/2 reconstructed samples
	void OverlapAdd( const T *data0, const T *data1, T *dataOut ) {
		Int n2 = n >> 1;
		for ( Int i=0; i<n2; i++ ) {
			dataOut[i] = data0[ i + n2 ] + data1[ i ];
		}
	}

	// note: not required for decompression but keeping for the sake of completeness
	// outputs n/2 => mdctData
	void DoMdct( const T *data, T *mdctData )
	{
		Int n4 = n >> 2;
		Int n2 = 2*n4;
		Int n34 = 3*n4;
		Int n54 = 5*n4;

		// even/odd folding and pre-twiddle
		Int i;
		for ( i=0; i<n4; i+= 2 ) {
			Complex<T> &c = fftData[i >> 1];
			c = Complex<T>( Get(data, n34 - 1 - i) + Get(data, n34 + i),
				Get(data, n4 + i) - Get(data, n4 - 1 - i) );
			c *= twiddle[i >> 1];
		}

		for ( ; i<n2; i += 2 ) {
			Complex<T> &c = fftData[i >> 1];
			c = Complex<T>( Get( data, n34 - 1 - i ) - Get( data, i - n4 ),
				Get( data, n4 + i ) + Get( data, n54 - 1 - i ) );
			c *= twiddle[i >> 1];
		}

		// do complex fft
		fft.DoFft( fftData.GetData() );

		// post-twiddle
		for ( i=0; i<n2; i += 2 ) {
			// reference - to avoid copying
			Complex<T> &c = fftData[i >> 1];
			c *= twiddle[i >> 1];
			c *= prescale;
			mdctData[i] = -c.re;
			mdctData[n2 - 1 - i ] = c.im;
		}
	}

	// outputs n => data
	void DoIMdct( const T *mdctData, T *data )
	{
		Int n4 = n >> 2;
		Int n2 = 2*n4;
		Int n34 = 3*n4;
		Int n54 = 5*n4;

		// pre-twiddle
		Int i;
		for ( i=0; i<n2; i += 2 ) {
			Complex<T> c( mdctData[i], mdctData[n2 - 1 - i] );
			c *= twiddle[i >> 1];
			c *= (T)-2;
			fftData[i >> 1] = c;
		}

		// do complex fft
		fft.DoFft( fftData.GetData() );

		// odd/even expanding and post-twiddle
		for ( i=0; i<n4; i += 2 ) {
			// reference - avoid copying
			Complex<T> &c = fftData[i >> 1];
			c *= twiddle[i >> 1];
			c *= postscale;
			Set( data, n34 - 1 - i, c.re );
			Set( data, n34 + i, c.re );
			Set( data, n4 + i, -c.im );
			Set( data, n4 - 1 - i, c.im );
		}
		for ( ; i<n2; i += 2 ) {
			Complex<T> &c = fftData[i >> 1];
			c *= twiddle[i >> 1];
			c *= postscale;
			Set( data, n34 - 1 - i, c.re );
			Set( data, i - n4, -c.re );
			Set( data, n4 + i, -c.im );
			Set( data, n54 - 1 - i, -c.im );
		}
	}

	inline Int GetN() const {
		return n;
	}

private:
	inline T Get( const T *data, Int index ) const {
		return data[ index ] * window[ index ];
	}
	inline void Set( T *data, Int index, const T &value ) {
		data[ index ] = value * window[ index ];
	}

	T prescale;
	T postscale;
	Int m, n;
	Fft<T> fft;
	Array< Complex<T> > fftData;
	Array< Complex<T> > twiddle;
	Array< T > window;
};

}
