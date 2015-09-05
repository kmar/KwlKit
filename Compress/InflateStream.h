
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Inflate.h"
#include "../Base/Stream.h"

namespace KwlKit
{

class InflateStream : public Stream
{
public:
	KWLKIT_INJECT_STREAM()

	InflateStream();
	explicit InflateStream( InflateFormat fmt );
	InflateStream( Stream &refs, InflateFormat fmt = INF_RAW, bool owned = 0 );
	~InflateStream();

	bool SetStream( Stream &refs, bool owned );

	inline bool SetFormat( InflateFormat fmt ) {
		return infl.SetFormat(fmt);
	}

	inline void SetZipCrc( UInt zcrc ) {
		infl.SetZipCrc( zcrc );
	}

	inline bool Read( void *buf, Int size, Int &nread ) {
		return infl.Read( buf, size, nread );
	}

	inline bool IsEof() {
		return infl.IsEof();
	}

	inline BitStream &GetBitStream() {
		return infl.GetStream();
	}

	inline Inflate &GetInflate() {
		return infl;
	}

	bool Rewind();

private:
	// owned stream (if any)
	Stream *stream;
	Inflate infl;
};

}
