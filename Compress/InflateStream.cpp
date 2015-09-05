
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "InflateStream.h"

namespace KwlKit
{

// InflateStream

InflateStream::InflateStream() : stream(0) {
	infl.SetFormat( INF_RAW );
}

InflateStream::InflateStream( InflateFormat fmt ) : stream(0) {
	infl.SetFormat( fmt );
}

InflateStream::InflateStream( Stream &refs, InflateFormat fmt, bool owned ) : stream(0)
{
	if ( owned ) {
		stream = &refs;
	}
	infl.SetInput( refs );
	infl.SetFormat( fmt );
}

InflateStream::~InflateStream() {
	infl.Close();
	delete stream;
}

bool InflateStream::SetStream( Stream &refs, bool owned )
{
	delete stream;
	stream = 0;
	if ( owned ) {
		stream = &refs;
	}
	return infl.SetInput( refs );
}

bool InflateStream::Rewind()
{
	return infl.Rewind();
}

}
