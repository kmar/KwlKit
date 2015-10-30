
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "Inflate.h"
#include "../Base/Assert.h"
#include "../Base/Stream.h"
#include "../Base/Memory.h"
#include "../Base/Bits.h"
#include "../Base/Likely.h"
#include "../Base/Templates.h"
#include "Adler32.h"
#include "Crc32.h"

/*
	for a complete, valid Huffman tree (I'm not talking about adaptive Huffman),
	NO COMBINATION OF INPUT BITS can ever be invalid, a path is always ensured that leads to a symbol
	anyway it won't help me at all (just checking for completeness)
*/

namespace KwlKit
{

// Inflate

const Int Inflate::DICTIONARY_SIZE = 32768;
const Int Inflate::DIRECT_LUT_BITS = 11;
const Int Inflate::DIRECT_LUT_SIZE = 1 << DIRECT_LUT_BITS;

// Huffman

void Inflate::Huffman::Init( Int maxCodes )
{
	// pre-allocating here
	codes.Reserve( maxCodes );
	nodes.Reserve( 2*maxCodes-1 );
	directLut.Reserve( DIRECT_LUT_SIZE );
}


bool Inflate::Huffman::AddNode( UInt code, Int len, Int sym )
{
	Int ni = 0;
	if ( KWLKIT_UNLIKELY( nodes.IsEmpty() ) ) {
		nodes.Resize(1);
		Node &nn = nodes[0];
		nn.nodes[0] = nn.nodes[1] = -1;
		nn.leaf = -1;
	}
	for ( Int bit = 0; bit < len; bit++ ) {
		UInt b = code & (1 << bit);
		Node &n = nodes[ni];
		KWLKIT_RET_FALSE_UNLIKELY( n.leaf < 0 );
		Int nni = n.nodes[ b != 0 ];
		if ( nni < 0 ) {
			nni = nodes.Add( Node() );
			KWLKIT_ASSERT( nni < 32768 );
			n.nodes[ b != 0 ] = Short(nni);
			Node &nn = nodes[ nni ];
			nn.leaf = -1;
			nn.nodes[0] = nn.nodes[1] = -1;
		}
		if ( bit+1 == len ) {
			nodes[nni].leaf = sym;
			return 1;
		}
		ni = nni;
	}
	return 0;
}

bool Inflate::Huffman::Reconstruct( const Byte *codeLengths, Int count )
{
	if ( count <= 0 ) {
		return 0;
	}
	codes.Resize(count);
	nodes.Clear();

	// assuming 0..15
	UShort countPerLength[16] = {0};
	UInt nextCode[16] = {0};
	Int ncodes = 0;
	maxBits = 0;
	for ( Int i=0; i<count; i++ ) {
		Byte clen = codeLengths[i];
		if ( KWLKIT_UNLIKELY( clen > 15 ) ) {
			return 0;
		}
		if ( clen > maxBits ) {
			maxBits = clen;
		}
		countPerLength[ clen ]++;
	}
	UInt code = 0;
	countPerLength[0] = 0;
	for ( Int bit = 1; bit <= maxBits; bit++ ) {
		UShort count = countPerLength[bit-1];
		code += count;
		code <<= 1;
		nextCode[bit] = code;
		if ( countPerLength[bit] > 0 ) {
			ncodes++;
		}
	}
	if ( KWLKIT_UNLIKELY( !ncodes ) ) {
		return 0;
	}

	Byte prevLen = 255;
	Int startCode = -1;
	for ( Int i=0; i<count; i++ ) {
		Code &c = codes[i];
		c.count = 0;
		c.len = codeLengths[i];
		if ( !c.len ) {
			c.code = 0;
			prevLen = 255;
			continue;
		}
		if ( prevLen != c.len ) {
			prevLen = c.len;
			startCode = i;
		} else if ( prevLen != 255 && codes[startCode].code < 255 ) {
			codes[startCode].count++;
		}
		KWLKIT_ASSERT( c.len > 0 && c.len < 16 );
		UInt code = nextCode[ c.len ]++;
		if ( KWLKIT_UNLIKELY( code > 32767 || (code & ~(((UInt)1 << c.len)-1) ) ) ) {
			// bad code, unfortunately this doesn't catch all problems
			return 0;
		}

		Bits::Reverse(code, c.len);
		c.code = (UShort)code;

		if ( KWLKIT_UNLIKELY( !AddNode( code, c.len, i ) ) ) {
			return 0;
		}
	}

	// FIXME: Ken Silverman's pngout produces incomplete trees
	// ... so no completeness check here

	// build direct lut
	Int dbits = Min( (Int)maxBits, DIRECT_LUT_BITS );
	maxBits = (Byte)dbits;
	Int dsize = 1 << dbits;
	directLut.Resize( dsize );
	directLut.Fill(-1);
	for ( Int i=0; i<codes.GetSize(); i++ ) {
		const Code &c = codes[i];
		if ( !c.len || c.len > dbits ) {
			continue;
		}
		UShort rev = c.code;
		KWLKIT_ASSERT(dbits - c.len < 256);
		Byte remBits = Byte(dbits - c.len);
		for ( Int j=0; j<((Int)1 << remBits); j++ ) {
			Int idx = j << c.len;
			directLut[rev+idx] = (Short)i;
		}
	}
	return 1;
}

// decode value from stream, returns max int on error
template< Byte hidx, bool fast > Int Inflate::Decode()
{
	if ( fast ) {
		Byte tsize = huf[hidx].maxBits;
		UInt tmp = inbit.PeekBitsFast(tsize);
		Int res = huf[hidx].directLut[(Int)tmp];
		if ( KWLKIT_LIKELY( res >= 0 ) ) {
			Byte bits = huf[hidx].codes[res].len;
			inbit.PopBitsFast( bits );
			return res;
		}
		// at this point, we got tsize bits but we need more
		inbit.PopBitsFast( tsize );
		Int inode = 0;
		while ( tsize-- ) {
			if ( KWLKIT_UNLIKELY( inode < 0 ) ) {
				return Limits<Int>::MAX;
			}
			const Huffman::Node &n = huf[hidx].nodes[inode];
			KWLKIT_ASSERT( n.leaf < 0 );
			inode = n.nodes[ tmp & 1 ];
			tmp >>= 1;
		}
		while ( inode >= 0 ) {
			Int leaf = huf[hidx].nodes[inode].leaf;
			if ( KWLKIT_UNLIKELY( leaf >= 0 ) ) {
				return leaf;
			}
			Int b = inbit.ReadBit();
			if ( KWLKIT_UNLIKELY( b < 0 ) ) {
				break;
			}
			inode = huf[hidx].nodes[inode].nodes[b];
		}
		return Limits<Int>::MAX;
	} else {
		Int inode = 0;
		do {
			Int leaf = huf[hidx].nodes[inode].leaf;
			if ( KWLKIT_UNLIKELY( leaf >= 0 ) ) {
				return leaf;
			}
			Int b = inbit.ReadBit();
			if ( KWLKIT_UNLIKELY( b < 0 ) ) {
				break;
			}
			inode = huf[hidx].nodes[inode].nodes[b];
		} while ( inode >= 0 );
		return Limits<Int>::MAX;
	}
}

// Inflate

const Byte Inflate::extraLenBits[29] = {
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
};

const UShort Inflate::lenBase[29] = {
	3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

const Byte Inflate::extraDistBits[30] = {
	0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

const UShort Inflate::distBase[30] = {
	1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097,
	6145, 8193, 12289, 16385, 24577
};

const Byte Inflate::codeOrder[19] = {
	16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

template< Byte hidx > bool Inflate::BuildHuffman( const Byte *codeLengths, Int count ) {
	return huf[hidx].Reconstruct( codeLengths, count );
}

void Inflate::BuildFixedLitLenHuffman()
{
	// these tricky constants avoid FP in PVS studio
	const Int BITSIZE_PART0 = 8;
	const Int BITSIZE_PART1 = BITSIZE_PART0+1;
	const Int BITSIZE_PART2 = BITSIZE_PART0-1;
	const Int BITSIZE_PART3 = BITSIZE_PART0;

	Byte codeLengths[288];
	MemSet( codeLengths+0, BITSIZE_PART0, 144 );
	MemSet( codeLengths+144, BITSIZE_PART1, 112 );
	MemSet( codeLengths+256, BITSIZE_PART2, 24 );
	MemSet( codeLengths+280, BITSIZE_PART3, 8 );
	BuildHuffman<0>( codeLengths, 288 );
}

void Inflate::BuildFixedDistHuffman()
{
	Byte codeLengths[32];
	MemSet( codeLengths, 5, 32 );
	BuildHuffman<1>( codeLengths, 32 );
}

void Inflate::Init()
{
	// "init" format/crcFunction just to keep some static analyzers happy
	format = INF_RAW;
	crcFunction = 0;

	huf[0].Init( 288 );
	huf[1].Init( 32 );
	zipCrc = 0;
	outLimit = Limits<Long>::MAX;
	SetFormat();
	ResetState();
}

Inflate::Inflate() : input(0)
{
	Init();
}

Inflate::Inflate( Stream &sin ) : input(&sin), inbit(sin)
{
	Init();
}

bool Inflate::SetInput( Stream &sin )
{
	input = &sin;
	return inbit.SetStream(sin);
}

bool Inflate::SetFormat( InflateFormat fmt )
{
	format = fmt;
	crcFunction = 0;
	if ( fmt == INF_ZLIB ) {
		crcFunction = GetAdler32;
	} else if ( fmt == INF_GZIP || fmt == INF_ZIP ) {
		crcFunction = GetCrc32;
	}
	return 1;
}

bool Inflate::Flush()
{
	Int delta = (Int)dictIndex - (Int)dictFlushIndex;
	if ( KWLKIT_UNLIKELY( !delta ) ) {
		return 1;
	}
	if ( delta > 0 ) {
		// no need to split
		dictFlushIndex = dictIndex;
		return (outLimit -= delta) >= 0;
	}

	// split in two parts
	KWLKIT_ASSERT( dictIndex <= dictFlushIndex );

	Int upperPartSize = DICTIONARY_SIZE - (Int)dictFlushIndex;

	dictFlushIndex = dictIndex;
	return (outLimit -= upperPartSize + (Int)dictIndex) >= 0;
}

// output single byte
inline void Inflate::OutByte( Byte b )
{
	dictionary[ dictIndex++ ] = b;
	dictIndex &= DICTIONARY_SIZE - 1;
}

// output rep
void Inflate::OutRep( Int len, Int dist )
{
	KWLKIT_ASSERT( len >= 3 && len <= 258 );
	KWLKIT_ASSERT( dist >= 1 && dist <= DICTIONARY_SIZE );
	UInt ptr = dictIndex - (UInt)dist;
	ptr &= DICTIONARY_SIZE - 1;

	if ( (Int)ptr + len <= DICTIONARY_SIZE && (Int)dictIndex + len < DICTIONARY_SIZE ) {
		Byte *dst = dictionary.GetData() + dictIndex;
		const Byte *src = dictionary.GetData() + ptr;
		dictIndex += len;
		while ( len-- > 0 ) {
			*dst++ = *src++;
		}
		return;
	}

	while ( len-- > 0 ) {
		OutByte( dictionary[ ptr++ ] );
		ptr &= DICTIONARY_SIZE - 1;
	}
}

bool Inflate::UncompressedHeader( UInt &len )
{
	UInt nlen;
	if ( !inbit.ReadBits(len, 16) || !inbit.ReadBits(nlen, 16) ) {
		return 0;
	}
	return len == (~nlen & 0xffffu);
}

bool Inflate::CopyBytes( Int len )
{
	// we'll copy data now!
	while ( len-- > 0 ) {
		Byte b;
		if ( KWLKIT_UNLIKELY( !inbit.ReadByte(b) ) ) {
			return 0;
		}
		OutByte(b);
	}
	return 1;
}

template< bool fast > Int Inflate::DecodeLen( Int lit )
{
	Int ilen = lit - 257;
	Int len = lenBase[ilen];
	Byte xlen = extraLenBits[ilen];
	if ( xlen ) {
		// read extra length bits
		UInt lenExtra;
		if ( fast ) {
			inbit.ReadBitsFast( lenExtra, xlen );
		} else {
			if ( KWLKIT_UNLIKELY( !inbit.ReadBits( lenExtra, xlen ) ) ) {
				return -1;
			}
		}
		len += (Int)lenExtra;
	}
	return len;
}

template< bool fast > Int Inflate::DecodeDist( Int code )
{
	KWLKIT_ASSERT( code >= 0 && code < 30 );
	Int dist = distBase[code];
	Byte xdist = extraDistBits[code];
	if ( xdist ) {
		// read extra dist bits
		UInt distExtra;
		if ( fast ) {
			inbit.ReadBitsFast( distExtra, xdist );
		} else {
			if ( KWLKIT_UNLIKELY( !inbit.ReadBits( distExtra, xdist ) ) ) {
				return -1;
			}
		}
		dist += (Int)distExtra;
	}
	return dist;
}

bool Inflate::CompressedBlock( Int &tlen )
{
compressedLoop:
	// maximum command (len+dist) = 48 bits = 6 bytes
	while ( tlen > 0 && inbit.GetBufferedBytes() >= 6 ) {
		Int lit;
		if ( (lit = Decode<0,1>()) < 256 ) {
			// just output literal!
			OutByte( (Byte)(lit) );
			tlen--;
			continue;
		}
		if ( KWLKIT_UNLIKELY( lit > 285 ) ) {
			return 0;
		}
		if ( KWLKIT_UNLIKELY( lit == 256 ) ) {
			// end of block code
			state = nextBlockState;
			return 1;
		}

		// have rep here!
		Int len = DecodeLen<1>( lit );
		Int distCode = Decode<1,1>();
		if ( KWLKIT_UNLIKELY( distCode >= 30 ) ) {
			return 0;
		}
		OutRep( len, DecodeDist<1>( distCode ) );
		tlen -= len;
	}
	while ( tlen > 0 && inbit.GetBufferedBytes() < 6 ) {
		Int lit;
		if ( (lit = Decode<0, 0>()) < 256 ) {
			// just output literal!
			OutByte( (Byte)(lit) );
			tlen--;
			continue;
		}
		if ( KWLKIT_UNLIKELY( lit > 285 ) ) {
			return 0;
		}
		if ( KWLKIT_UNLIKELY( lit == 256 ) ) {
			// end of block code
			state = nextBlockState;
			return 1;
		}

		// have rep here!
		Int len = DecodeLen<0>( lit );
		if ( KWLKIT_UNLIKELY( len < 0 ) ) {
			return 0;
		}
		Int distCode = Decode<1, 0>();
		if ( KWLKIT_UNLIKELY( distCode >= 30 ) ) {
			return 0;
		}
		Int dist = DecodeDist<0>( distCode );
		if ( KWLKIT_UNLIKELY( dist < 0 ) ) {
			return 0;
		}
		OutRep( len, dist );
		tlen -= len;
	}
	if ( tlen <= 0 ) {
		return 1;
	}
	goto compressedLoop;		// goto saves a level of nesting
}

// reads code lengths
bool Inflate::ReadCodeLengths( Byte *clens, UInt count )
{
	Byte currentLen = 0;
	while ( count ) {
		Int cl = Decode<1, 0>();
		if ( KWLKIT_UNLIKELY( cl >= 19 ) ) {
			return 0;
		}
		if ( cl < 16 ) {
			currentLen = (Byte)(cl & 15);
			*clens++ = currentLen;
			count--;
			continue;
		}
		if ( cl == 16 ) {
			UInt tmp;
			if ( KWLKIT_UNLIKELY( !inbit.ReadBits( tmp, 2 ) ) ) {
				return 0;
			}
			tmp += 3;
			// repeat current tmp times
			while ( tmp-- ) {
				*clens++ = currentLen;
				if ( KWLKIT_UNLIKELY( !--count ) ) {
					return tmp == 0;
				}
			}
			continue;
		}
		if ( cl == 17 ) {
			UInt tmp;
			if ( KWLKIT_UNLIKELY( !inbit.ReadBits( tmp, 3 ) ) ) {
				return 0;
			}
			tmp += 3;
			while ( tmp-- ) {
				*clens++ = 0;
				if ( KWLKIT_UNLIKELY( !--count ) ) {
					return tmp == 0;
				}
			}
			currentLen = 0;
			continue;
		}
		KWLKIT_ASSERT( cl == 18 );
		{
			UInt tmp;
			if ( KWLKIT_UNLIKELY( !inbit.ReadBits( tmp, 7 ) ) ) {
				return 0;
			}
			tmp += 11;
			while ( tmp-- ) {
				*clens++ = 0;
				if ( KWLKIT_UNLIKELY( !--count ) ) {
					return tmp == 0;
				}
			}
			currentLen = 0;
		}
	}
	return 1;
}

bool Inflate::DynamicHeader()
{
	UInt hlit, hdist, hclen;
	if ( KWLKIT_UNLIKELY( !inbit.ReadBits( hlit, 5 ) ) ) {
		return 0;
	}
	if ( KWLKIT_UNLIKELY( !inbit.ReadBits( hdist, 5 ) ) ) {
		return 0;
	}
	if ( KWLKIT_UNLIKELY( !inbit.ReadBits( hclen, 4 ) ) ) {
		return 0;
	}
	hclen += 4;
	if ( KWLKIT_UNLIKELY( hclen > 19 ) ) {
		return 0;
	}
	Byte clen[19] = {0};
	// read hclen*3 bits...
	for ( UInt i = 0; i < hclen; i++ ) {
		UInt tmp;
		if ( KWLKIT_UNLIKELY( !inbit.ReadBits( tmp, 3 ) ) ) {
			return 0;
		}
		clen[ codeOrder[i] ] = (Byte)(tmp & 7);
	}
	if ( KWLKIT_UNLIKELY( !BuildHuffman<1>( clen, 19 ) ) ) {
		return 0;
	}

	UInt allCodes = hlit + hdist + 258;
	if ( KWLKIT_UNLIKELY( allCodes > 288+32 ) ) {
		return 0;
	}
	Byte codeLengths[288 + 32] = {0};
	if ( KWLKIT_UNLIKELY( !ReadCodeLengths( codeLengths, allCodes ) ) ) {
		return 0;
	}
	// all that remains is to build the codes
	if ( KWLKIT_UNLIKELY( !BuildHuffman<0>( codeLengths, hlit + 257 ) ) ) {
		return 0;
	}
	return BuildHuffman<1>( codeLengths + hlit + 257, hdist+1 );
}

inline Int Inflate::GetBufferedBytes() const
{
	Int filled = (Int)dictIndex - (Int)dictFlushIndex;
	if ( filled < 0 ) {
		filled = DICTIONARY_SIZE - (Int)dictFlushIndex;
		filled += dictIndex;
	}
	return filled;
}

void Inflate::UpdateCrc()
{
	// and also total output size so far
	totalOutputSize += GetBufferedBytes();
	if ( format == INF_RAW ) {
		return;
	}
	Int delta = (Int)dictIndex - (Int)dictFlushIndex;
	if ( delta > 0 ) {
		// no need to split
		crc = crcFunction( dictionary.GetData() + dictFlushIndex, dictIndex - dictFlushIndex, crc );
		return;
	}
	if ( KWLKIT_UNLIKELY( !delta ) ) {
		return;
	}
	// split in two parts
	KWLKIT_ASSERT( dictIndex <= dictFlushIndex );

	Int upperPartSize = DICTIONARY_SIZE - (Int)dictFlushIndex;

	crc = crcFunction( dictionary.GetData() + dictFlushIndex, upperPartSize, crc );
	crc = crcFunction( dictionary.GetData(), dictIndex, crc );
}

bool Inflate::FinalizeStream()
{
	if ( format == INF_RAW ) {
		return 1;
	}
	// here we do crc check if necessary
	if ( format == INF_ZIP ) {
		return zipCrc == crc;
	}
	UInt storedCrc;
	// because we're reading, FlushByte never fails
	inbit.FlushByte();
	if ( format == INF_ZLIB ) {
		// fetch adler32 big endian
		if ( !inbit.ReadBits32( storedCrc, 32 ) ) {
			return 0;
		}
		Endian::FromBig( storedCrc );
		return storedCrc == crc;
	}
	KWLKIT_ASSERT( format == INF_GZIP );
	if ( !inbit.ReadBits32( storedCrc, 32 ) || storedCrc != crc ) {
		return 0;
	}
	// here we can validate file size
	UInt sizeLo = (UInt)(totalOutputSize & Limits<UInt>::MAX);
	UInt storedSize;
	return inbit.ReadBits32( storedSize, 32 ) && sizeLo == storedSize;
}

// read ZLib/GZip headers
bool Inflate::ReadZLibHeader()
{
	Byte cmf, flg;
	if ( !inbit.ReadByte(cmf) || !inbit.ReadByte(flg) ) {
		return 0;
	}
	// perform validation
	if ( (cmf & 15) != 8 || (cmf & 0xf0) > 0x70 ) {
		// compression method not deflate/invalid CINFO
		return 0;
	}
	if ( (cmf*256 + flg) % 31 ) {
		// invalid combination
		return 0;
	}
	if ( flg & 32 ) {
		// FDICT => read dictid
		UInt dictid;
		if ( !inbit.ReadBits32(dictid, 32) ) {
			return 0;
		}
		Endian::FromBig(dictid);
		// TODO/FIXME: validate later, a compliant decoder should do it but we don't
		// unfortunately the doc doesn't list the allowed values and I'm not going to dig in ZLib sources
	}
	return 1;
}

// read zero-terminated string
bool Inflate::ReadString( String &str, UInt &hcrc )
{
	str.Clear();
	Array<Byte> bstr;
	for(;;) {
		Byte b;
		if ( !inbit.ReadByte(b) ) {
			return 0;
		}
		hcrc = GetCrc32Byte( b, hcrc );
		bstr.Add( b );
		if ( !b ) {
			break;
		}
	}
	str = String( reinterpret_cast<const char *>( bstr.GetData() ) );
	return 1;
}

bool Inflate::ReadGZipHeader()
{
	UInt hcrc = 0;
	Byte id1, id2;
	if ( !inbit.ReadByte(id1) || !inbit.ReadByte(id2) ) {
		return 0;
	}
	if ( id1 != 0x1f || id2 != (Byte)0x8b ) {
		// header magic mismatch
		return 0;
	}
	Byte cm, flg;
	if ( !inbit.ReadByte(cm) || !inbit.ReadByte(flg) ) {
		return 0;
	}

	if ( cm != 8 || (flg & 0xe0) ) {
		// not deflated/reserved flags set
		return 0;
	}

	hcrc = GetCrc32Byte(id1, hcrc);
	hcrc = GetCrc32Byte(id2, hcrc);
	hcrc = GetCrc32Byte(cm, hcrc);
	hcrc = GetCrc32Byte(flg, hcrc);

	// read mtime, xfl, os
	Byte xfl, os;
	if ( !inbit.ReadBits32( unixTime, 32 ) || !inbit.ReadByte( xfl ) || !inbit.ReadByte( os ) ) {
		return 0;
	}

	hcrc = GetCrc32Byte( unixTime & 255, hcrc);
	hcrc = GetCrc32Byte( (unixTime >> 8) & 255, hcrc);
	hcrc = GetCrc32Byte( (unixTime >> 16) & 255, hcrc);
	hcrc = GetCrc32Byte( (unixTime >> 24) & 255, hcrc);
	hcrc = GetCrc32Byte(xfl, hcrc);
	hcrc = GetCrc32Byte(os, hcrc);

	if ( flg & 4 ) {
		// FEXTRA => extra block follows
		UInt xlen;
		if ( !inbit.ReadBits( xlen, 16 ) ) {
			return 0;
		}
		hcrc = GetCrc32Byte( xlen & 255, hcrc);
		hcrc = GetCrc32Byte( (xlen >> 8) & 255, hcrc);
		while ( xlen ) {
			Byte tmp;
			if ( !inbit.ReadByte( tmp ) ) {
				return 0;
			}
			hcrc = GetCrc32Byte( tmp, hcrc);
			xlen--;
		}
	}

	// FNAME => zero-terminated name follows
	if ( (flg & 8) && !ReadString( name, hcrc ) ) {
		return 0;
	}

	// FCOMMENT => zero-terminated name follows
	if ( (flg & 16) && !ReadString( comment, hcrc ) ) {
		return 0;
	}

	if ( flg & 2 ) {
		// read low 16 bits of CRC-32 (header)
		UInt crcLo;
		if ( !inbit.ReadBits( crcLo, 16 ) ) {
			return 0;
		}
		if ( crcLo != (hcrc & 65535) ) {
			// header CRC mismatch
			return 0;
		}
	}
	return 1;
}

bool Inflate::PrefetchOutput()
{
	// bytes to fill
	Int toFill;
	Int filled = GetBufferedBytes();
	toFill = DICTIONARY_SIZE - 258 - 1 - filled;	// -1 to avoid overflow
	if ( toFill <= 0 ) {
		// ok, we don't need more at the moment
		return 1;
	}
loopstate:
	switch( state )
	{
	case INF_STATE_BEGIN:
		name.Clear();
		comment.Clear();
		unixTime = 0;
		totalOutputSize = 0;			// Total size of output
		crc = format == INF_ZLIB;		// Trick to preinitialize adler32 CRC to 1
		state = INF_STATE_BLOCK_HEADER;
		if ( format == INF_ZLIB ) {
			state = INF_STATE_ZLIB_HEADER;
		} else if ( format == INF_GZIP ) {
			state = INF_STATE_GZIP_HEADER;
		}
		goto loopstate;
	case INF_STATE_ZLIB_HEADER:
		if ( !ReadZLibHeader() ) {
			// error reading/verifying ZLib header
			state = INF_STATE_ERROR;
			return 0;
		}
		state = INF_STATE_BLOCK_HEADER;
		goto loopstate;
	case INF_STATE_GZIP_HEADER:
		if ( !ReadGZipHeader() ) {
			// error reading/verifying GZip header
			state = INF_STATE_ERROR;
			return 0;
		}
		state = INF_STATE_BLOCK_HEADER;
		// fall through
	case INF_STATE_BLOCK_HEADER:
		{
			UInt bfinal;
			if ( !inbit.ReadBits(bfinal, 3) ) {
				state = INF_STATE_ERROR;
				return 0;
			}
			UInt btype = bfinal >> 1;
			bfinal &= 1;
			nextBlockState = bfinal ? INF_STATE_EOS_FINALIZE : INF_STATE_BLOCK_HEADER;
			switch( btype )
			{
			case 0:
				// uncompressed
				// we're reading => FlushByte never fails
				inbit.FlushByte();
				state = INF_STATE_UNCOMPRESSED_HEADER;
				goto loopstate;
			case 1:
				// fixed Huffman
				BuildFixedLitLenHuffman();
				BuildFixedDistHuffman();
				state = INF_STATE_COMPRESSED;
				// fall through to next state
				break;
			case 2:
				// dynamic Huffman
				if ( !DynamicHeader() ) {
					state = INF_STATE_ERROR;
					return 0;
				}
				state = INF_STATE_COMPRESSED;
				// fall through to next state
				break;
			default:
				// invalid block type
				state = INF_STATE_ERROR;
				return 0;
			}
		}
	case INF_STATE_COMPRESSED:
		// must follow INF_STATE_BLOCK_HEADER
		if ( !CompressedBlock( toFill ) ) {
			// failed to read input stream
			state = INF_STATE_ERROR;
			return 0;
		}
		if ( toFill > 0 ) {
			goto loopstate;
		}
		// we don't need more at the moment => done for now
		break;
	case INF_STATE_UNCOMPRESSED_HEADER:
		if ( !UncompressedHeader(uncLen) ) {
			// invalid header
			state = INF_STATE_ERROR;
			return 0;
		}
		state = INF_STATE_UNCOMPRESSED;
		// fall through
	case INF_STATE_UNCOMPRESSED:
		{
			Int loopLen = Min( toFill, (Int)uncLen );
			if ( !CopyBytes( loopLen ) ) {
				// failed to read input stream
				state = INF_STATE_ERROR;
				return 0;
			}
			toFill -= loopLen;
			uncLen -= loopLen;
			if ( !uncLen ) {
				state = nextBlockState;
			}
			if ( toFill > 0 ) {
				goto loopstate;
			}
			// we don't need more at the moment => done for now
			break;
		}
	case INF_STATE_EOS_FINALIZE:
		UpdateCrc();
		if ( !FinalizeStream() ) {
			// CRC failure
			state = INF_STATE_ERROR;
			return 0;
		}
		state = INF_STATE_EOS;
		// fall through
	case INF_STATE_EOS:
		return 1;
	default:
		// invalid case
		return 0;
	}
	// here we should update CRC if needed rather than in Flush which may never get called due to Read!
	UpdateCrc();
	return 1;
}

void Inflate::ResetState()
{
	totalOutputSize = 0;
	crc = 0;
	uncLen = 0;
	unixTime = 0;
	state = INF_STATE_BEGIN;
	nextBlockState = INF_STATE_ERROR;
	dictionary.Resize( DICTIONARY_SIZE );
	dictionary.Fill(0);
	dictIndex = dictFlushIndex = 0;
}

bool Inflate::Rewind()
{
	ResetState();
	if ( !input ) {
		return 1;
	}
	if ( !inbit.SetStream(*input) ) {
		return 0;
	}
	return input->Rewind();
}

// read uncompressed bytes
bool Inflate::Read( void *buf, Int count, Int &nread )
{
	KWLKIT_ASSERT( buf && count >= 0 );
	nread = 0;
	Byte *b = static_cast<Byte *>(buf);
	do {
		Int filled = GetBufferedBytes();
		Int toCopy = Min( count, filled );
		nread += toCopy;
		count -= toCopy;
		KWLKIT_ASSERT( count >= 0 );
		while ( toCopy > 0  ) {
			// split and memcpy!
			Int part = DICTIONARY_SIZE - dictFlushIndex;
			part = Min( toCopy, part );
			KWLKIT_ASSERT( part > 0 );
			MemCpy(b, dictionary.GetData() + dictFlushIndex, part );
			dictFlushIndex += part;
			dictFlushIndex &= DICTIONARY_SIZE - 1;
			toCopy -= part;
			b += part;
		}
		if ( count <= 0 ) {
			break;
		}
		KWLKIT_ASSERT( dictIndex == dictFlushIndex );
		if ( KWLKIT_UNLIKELY( !PrefetchOutput() ) ) {
			return 0;
		}
	} while ( dictIndex != dictFlushIndex );
	return state != INF_STATE_ERROR;
}

bool Inflate::Close() {
	return inbit.Close(1);
}

}
