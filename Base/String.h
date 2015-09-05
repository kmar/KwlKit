
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>

namespace KwlKit
{

// String wrapper; currently using std::string
// feel free to change to some internal class
class String : public std::string
{
public:
	inline String() {}
	explicit String( const char *s ) : std::string(s) {}
	void Clear() {
		clear();
	}
};

}
