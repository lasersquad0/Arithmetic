// fpaq0b - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq0.cpp
// 10/01/2006 32 bit encoder modified, Fabio Buffoni

#include <cstring>
#include <ctime>
#include "ARIExceptions.h"
#include "fpaq0.h"

using namespace std;

//////////////////////////// Encoder ////////////////////////////

/* An Encoder does arithmetic encoding.  Methods:
   Encoder(COMPRESS, f) creates encoder for compression to archive f, which
     must be open past any header for writing in binary mode
   Encoder(DECOMPRESS, f) creates encoder for decompression from archive f,
     which must be open past any header for reading in binary mode
   encode(bit) in COMPRESS mode compresses bit to file f.
   decode() in DECOMPRESS mode returns the next decompressed bit from file f.
   flush() should be called when there is no more to compress.
*/

//typedef enum { COMPRESS, DECOMPRESS } Mode;


//////////////////////////// main ////////////////////////////
#pragma warning(disable : 4996)
/*
int main(int argc, char** argv) {

    // Chech arguments: fpaq0 c/d input output
    if (argc != 4 || (argv[1][0] != 'c' && argv[1][0] != 'd')) {
        printf("To compress:   fpaq0 c input output\n"
            "To decompress: fpaq0 d input output\n");
        exit(1);
    }

    // Start timer
    clock_t start = clock();

    // Open files
    FILE* in = fopen(argv[2], "rb");
    if (!in) perror(argv[2]), exit(1);
    FILE* out = fopen(argv[3], "wb");
    if (!out) perror(argv[3]), exit(1);
    int c;

    // Compress
    if (argv[1][0] == 'c') 
    {
        Encoder e(COMPRESS, out);
        while ((c = getc(in)) != EOF) 
        {
            e.encode(0);
            for (int i = 7; i >= 0; --i)
                e.encode((c >> i) & 1);
        }
        e.encode(1);  // EOF code
        e.flush();
    }

    // Decompress
    else 
    {
        Encoder e(DECOMPRESS, in);
        while (!e.decode()) 
        {
            int c = 1;
            while (c < 256)
                c += c + e.decode();
            putc(c - 256, out);
        }
    }

    // Print results
    printf("%s (%ld bytes) -> %s (%ld bytes) in %1.2f s.\n",
        argv[2], ftell(in), argv[3], ftell(out),
        ((double)clock() - start) / CLOCKS_PER_SEC);
    return 0;
}
*/
