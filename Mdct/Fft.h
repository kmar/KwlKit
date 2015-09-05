
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Complex.h"
#include "../Base/Bits.h"
#include "../Base/Array.h"

namespace KwlKit
{

// in-place radix 2 (I)FFT
template<typename T>
struct Fft
{
private:
	struct SwapData
	{
		UInt i0, i1;
	};
public:

	// 2**m = n
	explicit Fft( Int n_ ) : m(Log2Int(n_)), n(n_)
	{
		KWLKIT_ASSERT( n > 0 && 1 << m == n );
		twiddle.Resize( m );
		for ( Int p=0; p<m; p++ ) {
			Int tp = 1 << p;
			twiddle[p].Expi( (T)(-D_PI / tp) );
		}
		for ( Int i=0; i<n; i++ ) {
			UInt ri = (UInt)i;
			Bits::Reverse( ri, (Byte)m );
			if ( (UInt)i >= ri ) {
				continue;
			}
			SwapData swd;
			swd.i0 = i;
			swd.i1 = ri;
			swapData.Add( swd );
		}
	}

	template< Int p >
	void DoFftLoop( Complex<T> *data )
	{
		Int step = 1 << p;
		Int step2 = step * 2;
		Complex<T> s(1);
		// this micro-optimization has no effect
		const Complex<T> &w = twiddle[p];
		for ( Int i=0; i<step; i++ ) {
			for ( Int j=i; j<n; j += step2 ) {
				Int j2 = j + step;
				Complex<T> tmp = data[j2] * s;
				data[j2] = data[j] - tmp;
				data[j] += tmp;
			}
			s *= w;
		}
	}

	// do in-place DIT FFT on complex data
	// (radix 2)
	void DoFft( Complex<T> *data )
	{
		KWLKIT_ASSERT( data );
		// must bit-swap data indices
		Int sz = swapData.GetSize();
		const SwapData *sd = swapData.GetData();
		for ( Int i=0; i<sz; i++ ) {
			Swap( data[sd->i0], data[sd->i1] );
			sd++;
		}

		for ( Int p=0; p<m; p++ ) {
			// this u-opt helps a bit on ARM/iOS (up to p=9 because of kwl)
			switch(p) {
			case 0:
				DoFftLoop<0>( data );
				continue;
			case 1:
				DoFftLoop<1>( data );
				continue;
			case 2:
				DoFftLoop<2>( data );
				continue;
			case 3:
				DoFftLoop<3>( data );
				continue;
			case 4:
				DoFftLoop<4>( data );
				continue;
			case 5:
				DoFftLoop<5>( data );
				continue;
			case 6:
				DoFftLoop<6>( data );
				continue;
			case 7:
				DoFftLoop<7>( data );
				continue;
			case 8:
				DoFftLoop<8>( data );
				continue;
			case 9:
				DoFftLoop<9>( data );
				continue;
			}
			Int step = 1 << p;
			Int step2 = step * 2;
			Complex<T> s(1);
			// this micro-optimization has no effect
			const Complex<T> &w = twiddle[p];
			for ( Int i=0; i<step; i++ ) {
				for ( Int j=i; j<n; j += step2 ) {
					Int j2 = j + step;
					Complex<T> tmp = data[j2] * s;
					data[j2] = data[j] - tmp;
					data[j] += tmp;
				}
				s *= w;
			}
		}
	}

	void DoIFft( Complex<T> *data )
	{
		// inverse FFT is simply FFT on complex conjugates post-divided by N
		KWLKIT_ASSERT( data );
		for ( Int i=0; i<n; i++ ) {
			data[i].Conjugate();
		}
		DoFft(data);
		T mul = (T)1 / (T)n;
		for ( Int i=0; i<n; i++ ) {
			data[i].Conjugate();
			data[i] *= mul;
		}
	}

private:
	Int m, n;
	// precomputed twiddle factors for <0..m)
	// almost zero improvement
	Array< Complex<T> > twiddle;
	// precomputed bit-reversal swap info
	Array< SwapData > swapData;
};


}
