
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <limits>
#include "Limits.h"

namespace KwlKit
{

const Float FLOAT_INFINITY = std::numeric_limits<Float>::infinity();
const Float FLOAT_NAN = std::numeric_limits<Float>::quiet_NaN();
const Double DOUBLE_INFINITY = std::numeric_limits<Double>::infinity();
const Double DOUBLE_NAN = std::numeric_limits<Double>::quiet_NaN();

}
