
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Stream.h"
#include "Memory.h"
#include "Templates.h"

namespace KwlKit
{

// Stream

Stream::Stream() {
}

Stream::~Stream()
{
}

bool Stream::Flush() {
	return 1;
}

bool Stream::Close() {
	return 1;
}

bool Stream::Rewind() {
	return 0;
}

bool Stream::Read( void *buf, Int size, Int &nread )
{
	(void)buf;
	(void)size;
	(void)nread;
	return 0;
}

// skip bytes (read)
bool Stream::SkipRead( Long bytes )
{
	Byte buf[8192];
	while ( bytes > 0 ) {
		Int tmp = (Int)Min<Long>( bytes, sizeof(buf) );
		if ( !Read( buf, tmp ) ) {
			return 0;
		}
		bytes -= tmp;
	}
	return 1;
}

}
