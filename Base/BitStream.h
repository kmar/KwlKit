
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"
#include "Array.h"
#include "Assert.h"
#include "Likely.h"

namespace KwlKit
{

class Stream;

// note: LSBit first (maybe I'll add methods to support MSB first later)
// little endian if bits span multiple bytes
// note: this is a simplified read only version
class BitStream
{
public:
	BitStream();
	explicit BitStream( Stream &s );
	/*BitStream( const BitStream &o );
	BitStream &operator =( const BitStream &o );*/
	~BitStream();

	// attach underlying stream
	bool SetStream( Stream &s );
	// detach stream (do not use)
	void DetachStream();
	// read bits, no more than 24!
	bool ReadBits( UInt &u, Byte count );
	// read bits, no more than 16, assuming buffer holds enough data!
	inline bool ReadBitsFast( UInt &u, Byte count );
	// read bits, up to 32 (slower than ReadBits)
	bool ReadBits32( UInt &u, Byte count );
	// read byte
	bool ReadByte( Byte &b );
	// read bytes
	bool ReadBytes( Byte *b, Int count );

	inline Stream *GetStream() {
		return stream;
	}

	// read 1 bit
	inline Int ReadBit();

	// reset without flushing
	bool Reset();

	// flush byte
	bool FlushByte();

	// flush stream (also flushes byte!)
	bool Flush();

	// read-only: get number of buffered bytes
	inline Int GetBufferedBytes() const {
		return (Int)(buffTop - buffPtr);
	}

	// return bits to accumulator (read only!)
	// upper bits of must be zero! (assert-only-verified)
	// returns 0 if accumulator is full
	inline bool ReturnBits( UInt u, Byte bits );
	// fast version (assuming accumulator has space to hold bits)
	inline void ReturnBitsFast( UInt u, Byte bits );

	// read (peek) bits, no more than 16, assuming buffer holds enough data!
	inline UInt PeekBitsFast( Byte count );
	inline void PopBitsFast( Byte count );

	bool Close( bool force = 0 );

private:
	Array<Byte> buffer;		// internal buffer (defaults to 8k)
	Byte *buffPtr;			// ptr to next by in buffer
	Byte *buffTop;			// buffer top

	Stream *stream;			// reference ptr to underlying stream
	// read accumulator
	typedef UIntPtr RAccum;
	RAccum raccum;
	Byte raccumPos;

	void Init();
};

#include "BitStream.inl"

}
