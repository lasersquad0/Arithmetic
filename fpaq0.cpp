// fpaq0b - Stationary order 0 file compressor.
// (C) 2004, Matt Mahoney under GPL, http://www.gnu.org/licenses/gpl.txt
// To compile: g++ -O fpaq0.cpp
// 10/01/2006 32 bit encoder modified, Fabio Buffoni

#include <cstring>
#include <ctime>
#include <cassert>
#include "Exceptions.h"
#include "fpaq0.h"


//namespace std {}  // for MARS compiler
using namespace std;


#define Top_value uint32or64(0XFFFFFFFF)	  /* Largest code value */
/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */
#define First_qtr uint32or64(Top_value/4+1)  /* Point after first quarter    */
#define Half	  uint32or64(2*First_qtr)    /* Point after first half  */
#define Third_qtr uint32or64(3*First_qtr)    /* Point after third quarter */



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


fpaqBitCoder::fpaqBitCoder(IBlockCoder& cr) : BasicModel(cr), predictor(), x1(0), x2(Top_value),
x(0), bits_to_follow(0), bptr(128), bout(0), bptrin(1), bytesPassed(0)
{

}

inline int fpaqBitCoder::input_bit(void)
{
    if (!(bptrin >>= 1)) 
    {
        //bin = getc(archive);
        bin = input_byte();
        if (bin == EOF) bin = 0;
        bptrin = 128;
    }
    return ((bin & bptrin) != 0);
}


uchar fpaqBitCoder::input_byte()
{
    int ch = fin->get();
    
    if (ch == std::char_traits<char>::eof())
        throw bad_file_format("EOF got unexpectedly.");

    bytesPassed++;
    return (uchar)ch; /* fgetc(f);*/
}

void fpaqBitCoder::output_byte(uchar c)
{
    bytesPassed++;
    fout->put(c);
}

void fpaqBitCoder::Init()
{
    bytesPassed = 0;
    x1 = 0;
    x2 = Top_value;
    x = 0;
    bits_to_follow = 0;
    bptr = 128;
    bout = 0;
    bptrin = 1;
}


void fpaqBitCoder::BeginEncode(std::ostream* f)
{
    fout = f;
    Init();
}


void fpaqBitCoder::StopEncode()
{
    this->flush();
}

void fpaqBitCoder::BeginDecode(std::istream* f)
{
    fin = f;
    Init();

    // In DECOMPRESS mode, initialize x to the first 4 bytes of the archive
    //if (mode == DECOMPRESS)
    //{
    x = 1;
    for (; x < Half;) x += x + input_bit();
    x += x + input_bit();
    //}
}


void fpaqBitCoder::StopDecode()
{
    //nothing to do
}


/* encode(y) -- Encode bit y by splitting the range [x1, x2] in proportion
to P(1) and P(0) as given by the predictor and narrowing to the appropriate
subrange. Output leading bytes of the range as they become know n. */

inline void fpaqBitCoder::EncodeSymbol(uchar*sym) //(int y) 
{
    int c = *sym;
    //EncodeSymbol((uchar)0);
    for (int i = 7; i >= 0; --i)
        EncodeSymbol((uchar)((c >> i) & 1));

}

inline void fpaqBitCoder::EncodeSymbol(uchar y) //(int y) 
{
    // Update the range
    const uint32or64 xmid = x1 + ((x2 - x1) >> 12) * predictor.p();
    assert(x1 <= Top_value && x2 <= Top_value);
    assert(xmid >= x1 && xmid < x2);
    if (y)
        x2 = xmid;
    else
        x1 = xmid + 1;
    predictor.update(y);

    // Shift equal MSB's out
    for (;;) 
    {
        if (x2 < Half) 
        {
            bit_plus_follow(0);
        }
        else if (x1 >= Half) 
        {
            bit_plus_follow(1);
            x1 -= Half;
            x2 -= Half;
        }
        else if (x1 >= First_qtr && x2 < Third_qtr) 
        {
            bits_to_follow++;
            x1 -= First_qtr;
            x2 -= First_qtr;
            //x1 ^= First_qtr;
            //x2 ^= First_qtr;
        }
        else 
        {
            break;
        }
        x1 += x1;
        //x1 &= Top_value;
        x2 += x2 + 1;
        //x2 &= Top_value;
    }
}

/* Decode one bit from the archive, splitting [x1, x2] as in the encoder
and returning 1 or 0 depending on which subrange the archive point x is in.
*/

inline uchar fpaqBitCoder::DecodeSymbol(uchar*)
{
    int c = 0;

    for (int i = 7; i >= 0; --i)
        c += c + DecodeSymbol();
    return (uchar)c;

    //while (c < 256)
    //    c += c + DecodeSymbol();
    ////putc(c - 256, out);
    //return (uchar)(c-256);
}

inline uchar fpaqBitCoder::DecodeSymbol(void)
{
    // Update the range
    const uint32or64 xmid = x1 + ((x2 - x1) >> 12) * predictor.p();
    assert(x1 <= Top_value && x2 <= Top_value);
    assert(xmid >= x1 && xmid < x2);
    uchar y = 0;
    if (x <= xmid) 
    {
        y = 1;
        x2 = xmid;
    }
    else
        x1 = xmid + 1;
    
    predictor.update(y);

    // Shift equal MSB's out
    for (;;) 
    {
        if (x2 < Half) 
        {   
        }
        else if (x1 >= Half) 	   /* Output 1 if in high half. */
        {
            x1 -= Half;
            x -= Half;
            x2 -= Half;		        /* Subtract offset to top.  */
        }
        else if (x1 >= First_qtr && x2 < Third_qtr)	/* Output an opposite bit  later if in middle half. */
        {
            x1 -= First_qtr;	    /* Subtract offset to middle */
            x -= First_qtr;
            x2 -= First_qtr;
        }
        else 
        {
            break;			/* Otherwise exit loop.     */
        }
        x1 += x1;
        x += x + input_bit();
        x2 += x2 + 1;	/* Scale up code range.     */
    }
    return y;
}

// Should be called when there is no more to compress
void fpaqBitCoder::flush() 
{
    // In COMPRESS mode, write out the remaining bytes of x, x1 < x < x2
   // if (mode == COMPRESS) 
   // {
        bits_to_follow = 0;       //FB 10/01/2006
        if (x1 == 0)
            bit_plus_follow(0);
        else
            bit_plus_follow(1);
        if (bout) output_byte(bout); //putc(bout, archive);
   // }
}


inline void fpaqBitCoder::bit_plus_follow(int bit)
{
    bits_to_follow++;
    for (int notb = bit ^ 1; bits_to_follow > 0; bits_to_follow--, bit = notb)
    {
        if (bit) bout |= bptr;
        if (!(bptr >>= 1))
        {
            output_byte(bout);
            //putc(bout, archive);
            bptr = 128;
            bout = 0;
        }
    }
}


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
