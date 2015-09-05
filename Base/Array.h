
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <vector>
#include "Assert.h"
#include "Memory.h"

namespace KwlKit
{

// dynamic array wrapper; currently using std::vector
// feel free to change to some internal container
template< typename T >
class Array : public std::vector<T>
{
public:
	inline T *GetData() {
		KWLKIT_ASSERT( !this->empty() );
		return &this->front();
	}
	inline const T *GetData() const {
		KWLKIT_ASSERT( !this->empty() );
		return &this->front();
	}
	inline void Clear() {
		this->clear();
	}
	inline bool IsEmpty() const {
		return this->empty();
	}
	void Resize( Int nsize ) {
		this->resize( nsize );
	}
	void Reserve( Int nsize ) {
		this->reserve( nsize );
	}
	inline Int GetSize() const {
		return Int(this->size());
	}
	inline Int Add( const T &v ) {
		Int res = Int(this->size());
		this->push_back(v);
		return res;
	}
	void Fill( const T &v ) {
		for ( size_t i=0; i<this->size(); i++ ) {
			(*this)[i] = v;
		}
	}
	void MemSet( int v ) {
		KwlKit::MemSet( GetData(), v, this->size()*sizeof(T) );
	}
};

}
