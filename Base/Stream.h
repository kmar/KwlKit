
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "Types.h"
#include "Assert.h"
#include "Array.h"
#include "NoCopy.h"
#include "Likely.h"

namespace KwlKit
{

// helper macro
#define KWLKIT_INJECT_STREAM() using KwlKit::Stream::Read;

class Stream : public NoCopy
{
public:
	Stream();
	virtual ~Stream();

	// if buf is null or size <0, read is undefined (may crash!)
	// should read fail, nread contents are undefined
	virtual bool Read( void *buf, Int size, Int &nread );

	// require_success_version
	// only succeeds if size bytes are read
	inline bool Read( void *buf, Int size ) {
		Int nr;	return Read( buf, size, nr ) && nr == size;
	}

	// not really needed
	virtual bool Flush();

	virtual bool Close();

	virtual bool Rewind();

	// helper (skips nbytes forward)
	bool SkipRead( Long bytes );
};

}
