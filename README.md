# ARPack - fast and convenient arithmetic archiver

## Contacts
Author: Andrey Romanchenko (lasersquad@gmail.com)
Web site: https://github.com/lasersquad0/Arithmetic

Questions and comments: https://github.com/lasersquad0/Arithmetic/discussions or by email

## Functions

This is a project the implements different compression techniques such as:
- Arithmetic encoding. Four arithmetic coder implementations - 32bit, 64 bit, byte-wise, bit-wise. Each of them can be either ordinary or adaptive.
- Huffman encoding. Two different implementations - ordinary and adaptive.
- BWT and MTF transformations which are done before actual compression significantly increasing level of compression.
- Several simple models (order0, order1, order2, and order3) that can be used with any compression coders. 
- Compression/decompression can be done in several threads. 
- Support for up to 65535 files in archive.
- Support for large files in archive (up to 100G).
- Unicode support in filenames.
- Can be buit as: console app, static library, BPL library for C++ Builder.
- Total Commander Plugin to easy work with archives from Total Commander
- Command line tool for testing Total Commander plugin
 

## How to use

### Command line application
Command line application has name `ARPack.exe`.
You do not need any additional software installed to run ARPack archiver

If you run `ARPack.exe` without parameters it will show you list of command line parameters it accepts. 

```
ARPack arithmetic compressor 1.3.0 x64 Copyright(c) 2024 - 2026 Andrey Romanchenko 26 Oct 2026

Usage:
   ARPack.exe -command <arg1>...<argN> -option <arg>

Commands and options:
-a, --add         <req args>(2)..<tot args>(55) Add files to archive
-x, --extract     <req args>(1)..<tot args>(55) Extract files from archive
-d, --delete      <req args>(2)..<tot args>(55) Delete files from archive
-b, --blocksize   <arg>                         Set the block size for BWT transformation
-l, --list        <arg>                         List content of archive
-t, --threads     <arg>                         Use specified number of threads during operation
-h, --help                                      Show help
-m, --model-type  <arg>                         Use model of specified order. Valid model types: O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ.
-c, --coder-type  <arg>                         Use specified coder. Valid coders: huf, ahuf, ari, ari32, ari64, bitari, fpaqari.
-v, --verbose                                   Print more detailed (verbose) information to screen.
-s, --stream-mode                               Use stream mode (oposite to block mode). No BWT, no MTB in this mode.
-o, --output-dir  <arg>                         Specifies directory where uncompressed files will be placed. Valid with -x option only.
```

There are four commands: ` -a, -x, -l, -d`, all the other switches are options.
All options should be specified **before** any command.

### Commands
`-a <archive> <inputfiles...>` - add files <inputfiles> to archive <archive>. Name of the archive should go first after `-a` command. After archive name can be 1 or more files to compress.
Input files can contain paths (either full or relative). If input file does not contain path it is loaded from current directory.
Paths are not stored into archive at the moment, if two files have the same names they will be added in archive as two separate entities, but will overwrite each other during uncompression. Be carefull.
<inputfiles> should contain at least one file.

`-x <archive> <files to extract>` - uncompresses and extracts files from archive into current directory.
if <files to extract> is not empty - only specified files will be extracted from archive.
if <files to extract> is empty - all files will be extracted from archive.
Archive stays unmodified.
Command line parameter `-o <output dir>` can be used to specify other directory to put extracted files.  

`-l <archive>` - lists contents of archive. 
If option `-v` is used together with command `-l` then list of blocks is shown on the screen in addition to list of files.  

`-d <archive> <files to delete>` - deletes files from archive. 
<files to delete> should contain at least one filename.
if <files to delete> contains file names which are NOT present in archive, they are ignored.

### Switches

`-huf` and `-ahuf` - Huffman compression method (either original or adaptive) will be used for compression of specified files. 
With `-huf` each file will be passed two times. First time is for collecting frequency statistics and second time for actual compression using this statistics. 
Statistics table will be stored together with compressed data.
With `-ahuf` option Adaptive Huffman compression method will be used that does not require storing separate table with frequencies.
Both methods have similar compresssion ratio. Compression method is applied to all files being compressed.

`-ari`, `-ari32`, `-ari64`, `-bitari`, `fpaqari` - use Arithmetic encoding for compression. Algorithms are the same, they differ by bitness of calculations.
Due to this they produce different compressed output, but have similar compression ratio. `-ari` is an alias of `-ari32`, they produce the same result.
Compression method is applied to all files being compressed.

`-m` - specifies model to use for compression. Model is stored into archive so you do not need to specify model during decompression.
Valid models: O0, O1, O2, O3, O0FIX, O0SORT, O0PAIR, O3MIX, O1FPAQ. Models O3 and O3MIX may use much memory, be careful with them.

`-b` - specifies block size to use. Blocks are required for BWT and MTF transformations which provide better compression. 
Each input file is divided into blocks of specified size and each block will be compressed separately.
Block size can be specified in bytes, kilobytes (K) or megabytes (M).
Examples: `-b 2048`, `-b 100000`, `-b 10K`, `-b 128K`, `-b 2M`
Block size cannot be less than 1000 bytes and greater than 300 000 000 bytes.
This option is valid for compression only. Ignored during uncompresion.

`-t` - specifies number of threads to be used for doing compression or uncompression. 
Significantly speeds up the process on computers with several CPU cores.
Multiple threads will be used only if value specified by `-t` option is greater than 1.
Value cannot be greater than 24.
Examples: `-t 0`, `-t 1`, `-t 4`,`-t 10`.

`-s` - specifies to DO NOT use blocks during compression. Files will be compressed as streams of bytes without using BWT and MTF transformations. 
Compression will be faster, but compression rate much worse. 
This option is intended for experimental purposes for example for adding new compresson algorithms which do not support block mode by default. 

`-o` - specifies path to directory where to extract files during uncompressing. 

`-v` - turns on verbose mode. Additional information will be printed to console during compression or uncompression.

`-h` - shows help.

# Performance test results
For tests we are going to use [Calgary group dataset](http://www.data-compression.info/Corpora/CalgaryCorpus/)
