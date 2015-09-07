
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Types.h"
#include "../Base/Math.h"

namespace KwlKit
{

// reference: http://www.xiph.org/vorbis/doc/Vorbis_I_spec.html
inline Float KWLKIT_VISIBLE VorbisWindow( Int x, Int N )
{
	Float tmp = SinRad( ((Float)x + 0.5f) * PI / N );
	return SinRad( 0.5f * PI * Sqr(tmp) );
}

}
