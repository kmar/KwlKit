
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "BitStream.h"
#include "Stream.h"
#include "Endian.h"
#include "Memory.h"
#include "Templates.h"
#include "Stream.h"

namespace KwlKit
{

// BitStream

BitStream::BitStream() : stream(0), raccum(0), raccumPos(0) {
	Init();
}

BitStream::BitStream( Stream &s ) : stream(&s), raccum(0), raccumPos(0) {
	Init();
}

void BitStream::Init()
{
	raccum = raccumPos = 0;
	buffer.Resize(8192);
	buffPtr = buffer.GetData();
	buffTop = buffPtr;			// assume read buffer
}

BitStream::~BitStream() {
	Flush();
}

void BitStream::DetachStream()
{
	stream = 0;
}

// attach underlying stream
bool BitStream::SetStream( Stream &s )
{
	if (!Flush()) {
		return 0;
	}
	stream = &s;
	Init();
	return 1;
}


// read bits, no more than 24
bool BitStream::ReadBits( UInt &u, Byte count )
{
	KWLKIT_ASSERT( count > 0 && count <= 24 );

	if ( KWLKIT_LIKELY( raccumPos >= count ) ) {
fastfetch:
		// fast path if possible
		u = ((UInt)raccum & (((UInt)1 << count)-1));
		raccumPos -= count;
		raccum >>= count;
		return 1;
	}

	if ( KWLKIT_LIKELY( buffPtr + sizeof(raccum) <= buffTop ) ) {
		// we'll fast-fetch from buffer
		while ( raccumPos < count ) {
			raccum |= (RAccum)*buffPtr++ << raccumPos;
			raccumPos += 8;
		}
		KWLKIT_ASSERT( raccumPos <= 8*sizeof(raccum) );
		KWLKIT_ASSERT( buffTop >= buffPtr );
		// this will ONLY work if raccum size is > sizeof(UInt)!!!
		// this sucks ... can't fill the whole thing
		KWLKIT_ASSERT( raccumPos >= count );
		u = ((UInt)raccum & (((UInt)1 << count)-1));
		raccumPos -= count;
		raccum >>= count;
		return 1;
	}

	// the trick here is that there may be some bytes left in buffer

	Int left = (Int)(buffTop - buffPtr);

	if ( left ) {
		// note: can't use MemCpy here because of overlap (FIXME: use MemMove)
		Byte *dst = buffer.GetData();
		for ( Int i=0; i<left; i++ ) {
			dst[i] = buffPtr[i];
		}
	}

	// here we do slow read buffer and loop
	Int nr;
	if ( KWLKIT_UNLIKELY( !stream->Read( buffer.GetData()+left, buffer.GetSize()-left, nr ) ) ) {
		return 0;
	}
	buffPtr = buffer.GetData();
	buffTop = buffPtr + nr + left;
	// we'll fast-fetch from buffer
	while ( raccumPos < count && buffPtr < buffTop ) {
		raccum |= (RAccum)*buffPtr++ << raccumPos;
		raccumPos += 8;
	}
	// this will ONLY work if raccum size is > sizeof(UInt)!!!
	if ( KWLKIT_LIKELY( raccumPos >= count ) ) {
		goto fastfetch;
	}
	return 0;
}

// read bits, up to 32 (slower than ReadBits)
bool BitStream::ReadBits32( UInt &u, Byte count )
{
	KWLKIT_ASSERT( count > 0 && count <= 32 );
	Byte part = Min(count, (Byte)24);
	if ( KWLKIT_UNLIKELY(!ReadBits(u, part)) ) {
		return 0;
	}
	count -= part;
	if ( count ) {
		UInt tmp;
		if ( KWLKIT_UNLIKELY(!ReadBits(tmp, count)) ) {
			return 0;
		}
		u |= tmp << part;
	}
	return 1;
}

// read byte
bool BitStream::ReadByte( Byte &b )
{
	UInt tmp = 0;
	bool res = ReadBits( tmp, 8 );
	b = (Byte)(tmp & 255);
	return res;
}

// read bytes
bool BitStream::ReadBytes( Byte *b, Int count )
{
	KWLKIT_ASSERT( b && count > 0 );
	while ( count-- ) {
		if ( !ReadByte(*b++) ) {
			return 0;
		}
	}
	return 1;
}

// flush byte (both read and write)
bool BitStream::FlushByte()
{
	raccum >>= raccumPos & 7;
	raccumPos &= ~7;
	return 1;
}

// flush stream (also flushes byte!)
bool BitStream::Flush() {
	return stream ? FlushByte() && stream->Flush() : 1;
}

// reset without flushing
bool BitStream::Reset()
{
	raccum = raccumPos = 0;
	buffPtr = buffTop = buffer.GetData();
	return stream->Rewind();
}

bool BitStream::Close( bool force )
{
	bool res = Flush();
	if ( force || res ) {
		stream = 0;
	}
	return 1;
}

}
