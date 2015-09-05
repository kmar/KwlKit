
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

inline Int BitStream::ReadBit()
{
	if ( KWLKIT_LIKELY( raccumPos >= 1 ) ) {
		// fast path if possible
		Int res = (Int)raccum & 1;
		raccumPos--;
		raccum >>= 1;
		return res;
	}
	UInt tmp;
	if ( KWLKIT_UNLIKELY( !ReadBits(tmp, 1) ) ) {
		return -1;
	}
	return (Int)tmp;
}

// read bits, no more than 16, assuming buffer holds enough data!
inline bool BitStream::ReadBitsFast( UInt &u, Byte count )
{
	KWLKIT_ASSERT( count > 0 && count <= 16 );

	if ( KWLKIT_UNLIKELY( raccumPos < 16 ) ) {
		// we'll fast-fetch from buffer
		KWLKIT_ASSERT( buffPtr+1 < buffTop );
		raccum |= ((RAccum)buffPtr[0] + ((RAccum)buffPtr[1]<<8)) << raccumPos;
		raccumPos += 16;
		buffPtr += 2;
		KWLKIT_ASSERT( raccumPos <= 8*sizeof(raccum) );
	}
	KWLKIT_ASSERT( raccumPos >= count );
	u = ((UInt)raccum & (((UInt)1 << count)-1));
	raccumPos -= count;
	raccum >>= count;
	return 1;
}

// read (peek) bits, no more than 16, assuming buffer holds enough data!
inline UInt BitStream::PeekBitsFast( Byte count )
{
	KWLKIT_ASSERT( count > 0 && count <= 16 );

	if ( KWLKIT_UNLIKELY( raccumPos < 16 ) ) {
		// we'll fast-fetch from buffer
		KWLKIT_ASSERT( buffPtr+1 < buffTop );
		raccum |= ((RAccum)buffPtr[0] + ((RAccum)buffPtr[1]<<8)) << raccumPos;
		raccumPos += 16;
		buffPtr += 2;
		KWLKIT_ASSERT( raccumPos <= 8*sizeof(raccum) );
	}
	return ((UInt)raccum & (((UInt)1 << count)-1));
}

inline void BitStream::PopBitsFast( Byte count )
{
	KWLKIT_ASSERT( raccumPos >= count );
	raccumPos -= count;
	raccum >>= count;
}

// return bits to accumulator (read only!)
// returns 0 if accumulator is full
inline bool BitStream::ReturnBits( UInt u, Byte bits )
{
	if ( KWLKIT_UNLIKELY( (UInt)raccumPos + bits > 8*sizeof(raccum) ) ) {
		return 0;
	}
	ReturnBitsFast( u, bits );
	return 1;
}

// fast version (assuming accumulator has space to hold bits)
inline void BitStream::ReturnBitsFast( UInt u, Byte bits )
{
	KWLKIT_ASSERT( (UInt)raccumPos + bits <= 8*sizeof(raccum) );
	KWLKIT_ASSERT( !(u & ~(((UInt)1 << bits)-1) ) );
	raccum <<= bits;
	raccum |= u;
	raccumPos += bits;
}
