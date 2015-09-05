
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"
#include "Templates.h"
#include "Math.h"

namespace KwlKit
{

template< typename T >
struct Complex
{
public:
	// real/imaginary part
	T re;
	T im;

	inline Complex() {}
	inline Complex( const T &nre, const T &nim = 0 ) : re(nre), im(nim) {}

	// operators...
	inline Complex operator +( const Complex &o ) const {
		return Complex( re + o.re, im + o.im );
	}

	inline Complex operator -( const Complex &o ) const {
		return Complex( re - o.re, im - o.im );
	}

	inline Complex operator *( const Complex &o ) const {
		return Complex( re * o.re - im * o.im, im * o.re + re * o.im );
	}

	inline Complex operator *( const T &o ) const {
		return Complex( re * o, im * o );
	}

	friend inline Complex operator *( const T &x, const Complex &y ) {
		return y * x;
	}

	inline Complex operator /( const Complex &o ) const {
		T div = Sqr(o.re) + Sqr(o.im);
		return Complex( (re * o.re + im * o.im) / div, (im * o.re - re * o.im) / div );
	}

	inline Complex operator /( const T &o ) const {
		return Complex( re / o, im / o );
	}

	inline Complex &operator +=( const Complex &o ) {
		re += o.re;
		im += o.im;
		return *this;
	}

	inline Complex &operator -=( const Complex &o ) {
		re -= o.re;
		im -= o.im;
		return *this;
	}

	inline Complex &operator *=( const Complex &o ) {
		T tmp = re * o.re - im * o.im;
		im = (im * o.re + re*o.im);
		Swap(re, tmp);
		return *this;
	}

	inline Complex &operator *=( const T &o ) {
		re *= o;
		im *= o;
		return *this;
	}

	inline Complex &operator /=( const Complex &o ) {
		T div = Sqr(o.re) + Sqr(o.im);
		T tmp = (re * o.re + im * o.im) / div;
		im = (im * o.re - re * o.im) / div;
		Swap( re, tmp );
		return *this;
	}

	inline Complex &operator /=( const T &o ) {
		re /= o;
		im /= o;
		return *this;
	}

	inline Complex &Conjugate() {
		im = -im;
		return *this;
	}

	inline Complex GetConjugate() const {
		return Complex( re, -im );
	}

	// Euler formula: exp(ix) = cos(x) + i*sin(x)
	// note: exp(-ix) = cos(-x) + i*sin(-x) = cos(x) - i*sin(x) = conjugate( exp(ix) )
	inline Complex &Expi( const T &x ) {
		re = CosRad(x);
		im = SinRad(x);
		return *this;
	}

	inline Complex GetExpi( const T &x ) const {
		return Complex( CosRad(x), SinRad(x) );
	}
};

}
