// no license applies to this file (public domain)

// silence silly msc warnings
#define _CRT_SECURE_NO_WARNINGS
// I prefer old streams rather than <iostream>
#include <cstdio>

#include "../KwlKit.h"

// this injects full KwlKit (instead of linking statically with a library)
#include "../KwlKit.cpp"


// we need to create a Stream wrapper
class MyStream : public KwlKit::Stream
{
public:
	explicit MyStream(FILE *nfile) : file(nfile) {
	}

	~MyStream() {
		// important: we have to call Close() in dtor to clean up!
		// (this cannot be called from Stream::~Stream()
		// because at that point vtbl already points to Stream!
		Close();
	}

	bool Read( void *buf, int size, int &numRead )
	{
		if ( !file ) {
			numRead = 0;
			return 0;
		}
		numRead = int(fread( buf, 1, size, file ));
		return 1;
	}

	bool Rewind() {
		return file && fseek( file, 0, SEEK_SET ) == 0;
	}

	bool Close() {
		return file && fclose(file) == 0;
	}

private:
	FILE *file;
};

int main( int argc, char **argv )
{
	if ( argc != 3 ) {
		printf("usage: KwlToRaw <infile.kwl> <outfile.raw>\n");
		return 0;
	}

	// initialize: necessary to call once at startup
	KwlKit::Init();

	FILE *f2 = fopen(argv[2], "rb");
	if ( f2 ) {
		fclose(f2);
		printf("output file %s already exists => aborting\n", argv[2]);
		return -1;
	}

	FILE *f = fopen(argv[1], "rb");
	if ( !f ) {
		fprintf( stderr, "Cannot open %s", argv[1] );
		return -2;
	}
	// wrapper closes f automatically when done
	MyStream *myStream = new MyStream(f);

	KwlKit::WavRead wr;
	// you can also open non-owned stream using Open(), but this demonstrates
	// the most common case
	if ( !wr.OpenOwn(myStream) ) {
		fprintf( stderr, "Cannot parse %s", argv[1] );
		return -3;
	}

	f2 = fopen(argv[2], "wb");
	if ( !f2 ) {
		fprintf( stderr, "Cannot create %s", argv[2] );
		return -4;
	}

	// set desired sample rate and format; wav reader will automatically convert/resample as necessary
	wr.SetSampleRate( wr.GetWavSampleRate() );
	// note: KwlKit only supports up to 2 channels (stereo)
	// sample format, number of channels
	wr.SetFormat( KwlKit::SAMPLE_FORMAT_16S, 2 );
	wr.SetLooping( 0 );

	// ready to go...
	while(1) {
		short buffer[1024*2];
		int nread;
		if ( !wr.ReadSamples( buffer, 1024, nread ) ) {
			break;
		}
		size_t nw = nread*4;
		if ( fwrite( buffer, 1, nw, f2 ) != nw ) {
			fclose(f2);
			fprintf( stderr, "Cannot write %s", argv[2] );
			return -5;
		}
		// note: IsDone check is only necessary when resampling
		if ( nread != 1024 || wr.IsDone() ) {
			break;
		}
	}
	fclose(f2);
	return 0;
}
