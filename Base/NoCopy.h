
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

namespace KwlKit
{

class NoCopy
{
	NoCopy( const NoCopy & ) {
	}
	NoCopy &operator =( const NoCopy & ) {
		return *this;
	}
public:
	inline NoCopy() {}
};

}
