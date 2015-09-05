
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Types.h"

namespace KwlKit
{

// sample format (bit) flags
static const UInt SAMPLE_FORMAT_INVALID	=	0;
static const UInt SAMPLE_FORMAT_8S		=	1;
static const UInt SAMPLE_FORMAT_16S		=	2;	// native order
static const UInt SAMPLE_FORMAT_24S		=	3;	// LE order (always 3 bytes)
// native order (always float, we don't support 32-bit int samples)
static const UInt SAMPLE_FORMAT_32F		=	4;
static const UInt SAMPLE_FORMAT_SIZE_MASK	=	7;

// unsigned format(s) for import only:

// use this to change default signedness, integer samples only! undefined when used with float samples!
static const UInt SAMPLE_FORMAT_UNSIGNED	=	8;
static const UInt SAMPLE_FORMAT_MASK		=	15;
static const UInt SAMPLE_FORMAT_8U		=	1 | SAMPLE_FORMAT_UNSIGNED;
static const UInt SAMPLE_FORMAT_16U		=	2 | SAMPLE_FORMAT_UNSIGNED;	// native order
static const UInt SAMPLE_FORMAT_24U		=	3 | SAMPLE_FORMAT_UNSIGNED;	// LE order (always 3 bytes)

}
