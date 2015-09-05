
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "KwlKit.h"
#include "Compress/Crc32.h"

// define KWLKIT_SEPARATE to build as a static library
#if !KWLKIT_SEPARATE
#	include "Base/BitStream.cpp"
#	include "Base/Limits.cpp"
#	include "Base/Math.cpp"
#	include "Base/Memory.cpp"
#	include "Base/Stream.cpp"
#	include "Compress/Adler32.cpp"
#	include "Compress/Crc32.cpp"
#	include "Compress/Inflate.cpp"
#	include "Compress/InflateStream.cpp"
#	include "Kwl/KwlFile.cpp"
#	include "Resample/Resampler.cpp"
#	include "Sample/SampleUtil.cpp"
#	include "Wav/WavFile.cpp"
#	include "Wav/WavRead.cpp"
#endif

namespace KwlKit
{

void Init() {
	ComputeCrc32SlicingTables();
}

}
