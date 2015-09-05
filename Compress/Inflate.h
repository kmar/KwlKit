
// (c) Martin Sedlak (mar) 2015
// distributed under the Boost Software License, version 1.0
// (see accompanying file License.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "../Base/Stream.h"
#include "../Base/BitStream.h"
#include "../Base/Array.h"
#include "../Base/Limits.h"
#include "../Base/String.h"

namespace KwlKit
{

// Inflate algorithm...

enum InflateFormat
{
	INF_RAW,				// this is the default format
	INF_ZIP,				// Zip stream, for CRC-32 verification
	INF_ZLIB,				// ZLib stream
	INF_GZIP,				// GZip stream
};

class Inflate
{
public:
	Inflate();
	explicit Inflate( Stream &sin );

	// if supportRewind is true, remember stream position for rewinding
	bool SetInput( Stream &sin );
	bool SetFormat( InflateFormat fmt = INF_RAW );

	// get input bit stream
	inline BitStream &GetStream() {
		return inbit;
	}

	// is EOF?
	inline bool IsEof() const {
		return state == INF_STATE_EOS;
	}

	// return number of bytes decompressed so far
	inline Long GetOutputSize() const {
		return totalOutputSize;
	}

	// get GZip name
	inline const String &GetName() const {
		return name;
	}

	// get GZip comment
	inline const String &GetComment() const {
		return comment;
	}

	// get GZip Unix time (0 = unknown)
	inline UInt GetUnixTime() const {
		return unixTime;
	}

	// set Zip CRC for verification
	void SetZipCrc( UInt zcrc ) {
		zipCrc = zcrc;
	}

	// read uncompressed bytes
	// note: read (obviously) ignores outLimit
	bool Read( void *buf, Int count, Int &nread );

	// rewind
	bool Rewind();

	// close
	bool Close();
private:
	enum InflateState
	{
		INF_STATE_BEGIN,						// waiting for input
		INF_STATE_ZLIB_HEADER,					// ZLib header
		INF_STATE_GZIP_HEADER,					// GZip header
		INF_STATE_BLOCK_HEADER,					// reading block header
		INF_STATE_COMPRESSED,					// within compressed block
		INF_STATE_UNCOMPRESSED_HEADER,			// uncompressed block header
		INF_STATE_UNCOMPRESSED,					// within uncompressed block
		INF_STATE_EOS_FINALIZE,					// finalize
		INF_STATE_EOS,							// end of stream reached - no more data for us
		INF_STATE_ERROR							// an error occurred
	};

	Stream *input;
	BitStream inbit;
	static const Byte extraLenBits[29];	// [code-257]
	static const UShort lenBase[29];
	static const Byte extraDistBits[30];	// [code]
	static const UShort distBase[30];
	static const Byte codeOrder[19];

	// current decoder state
	InflateState state;
	// state to switch to when current block is finished
	InflateState nextBlockState;

	// sliding dictionary buffer, most likely a local separate 32K circular buffer
	Array< Byte > dictionary;
	// cyclic index
	UInt dictIndex;
	// cyclic flush index
	UInt dictFlushIndex;

	// limit output to this
	Long outLimit;

	// total output size
	Long totalOutputSize;

	// output single byte
	inline void OutByte( Byte b );
	// output rep
	void OutRep( Int len, Int dist );

	// handle uncompressed block header
	bool UncompressedHeader( UInt &len );
	// copy uncompressed bytes
	bool CopyBytes( Int len );
	// reads code lengths
	bool ReadCodeLengths( Byte *clens, UInt count );
	// dynamic Huffman header (reads clen Huffman)
	bool DynamicHeader();
	// initialize
	void Init();

	// get number of buffered bytes that can be read
	inline Int GetBufferedBytes() const;
	// produce as much buffered output as fits in dictionary
	bool PrefetchOutput();
	// finalize stream: handles CRC check if necessary
	bool FinalizeStream();

	// decode length from lit/len code
	template< bool fast > Int DecodeLen( Int lit );
	// decode distance from dist code
	template< bool fast > Int DecodeDist( Int code );

	class Huffman
	{
	public:
		struct Code
		{
			UShort code;		// code
			Byte len;			// length
			Byte count;		// (if first code: count-1 repcodes of same length)
		};
		struct Node
		{
			Int leaf;			// decoded code (symbol), -1 => no leaf
			Short nodes[2];	// left[0]/right[1]
		};

		Array< Code > codes;	// indexed directly by values!
		Array< Node > nodes;
		// direct LUT size is 2048 entries (could be less or more)
		// direct LUT: >= 0 => code, -1 => invalid
		Array< Short >
			directLut;
		Byte maxBits;			// maximum bits (=longest code, clamped to max LUT bits)

		void Init( Int maxCodes );
		bool AddNode( UInt code, Int len, Int sym );

		bool Reconstruct( const Byte *codeLengths, Int count );
	};

	Huffman huf[2];			// literal/length, distance/code (reused)

	// decode Huffman value from stream, returns max int on error
	template< Byte hidx, bool fast > Int Decode();

	template< Byte hidx> bool BuildHuffman(const Byte *codeLengths, Int count );
	void BuildFixedLitLenHuffman();
	void BuildFixedDistHuffman();

	// handle compressed block (common)
	// len = number of output bytes to decompress
	bool CompressedBlock( Int &len );
	// flush output buffer
	bool Flush();
	// update CRC on unpacked dictionary data
	void UpdateCrc();
	// reset state (necessary for Rewind)
	void ResetState();

	// state processing
	UInt uncLen;		// uncompressed block length in bytes

	// ZLib/GZip stream processing
	InflateFormat format;
	UInt crc;			// pending CRC
	UInt zipCrc;		// stored zip CRC (for verification, INF_ZIP format only)
	UInt (*crcFunction)( const void *buf, size_t len, UInt chsum );
	// GZip name/comment/time (Unix)
	String name, comment;
	UInt unixTime;

	// read ZLib/GZip headers
	bool ReadZLibHeader();
	bool ReadGZipHeader();
	// read zero-terminated string (because of GZip)
	bool ReadString( String &str, UInt &hcrc );

	// constants (for unity build)
	static const Int DICTIONARY_SIZE;
	static const Int DIRECT_LUT_BITS;
	static const Int DIRECT_LUT_SIZE;
};

}
