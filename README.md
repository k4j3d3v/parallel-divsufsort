Description
==========

Parallel-DivSufSort is a parallel lightweight suffix array construction algorithm for byte alphabets written in C++.
It is a implementation based on:
+ [divsufsort](https://github.com/y-256/libdivsufsort) implementation of induced sorting by Yuta Mori.
+ [parallel-range-lite](https://github.com/jlabeit/parallel-range-lite) implementation of prefix doubling for integer alphabets. 

A detailed description and benchmarks of the algorithm can be found in the following work.
> Julian Labeit, Julian Shun, and Guy E. Blelloch. Parallel Lightweight Wavelet Tree, Suffix Array and FM-Index Construction. DCC 2015.

Installation
==========
The following steps have been tested on Ubuntu 14.04 with gcc 5.3.0 and cmake 2.8.12.
```shell
git clone https://github.com/jlabeit/parallel-divsufsort.git
cd parallel-divsufsort
mkdir build
cd build
cmake ..
make
make install
```
Note that in the default version the cilkplus implementation by gcc is used for parallelization.
To change this setting edit parallelization settings in the CMakeLists.txt file.

Getting Started
==========
The project builds a demo driver in `demo/main.cpp`, which is compiled as the `pardss` executable.

After configuring and building the project:

```shell
mkdir build
cd build
cmake ..
make
```

The demo expects a single required positional argument: the input file path. It optionally accepts a binary suffix-array output path via `-w/--output` and a thread count via `-t/--threads`.

```shell
./demo/pardss input.txt
./demo/pardss input.txt -t 8
./demo/pardss input.txt -w sa.bin -t 16
```

If the input length fits in 32 bits, the program uses a 32-bit index type; otherwise it falls back to 64-bit indices.

The library itself exposes the following core functions:

```c++
// 32 bit version.
uint8_t divsufsort(const uint8_t *T, int32_t *SA, int32_t n);
// 64 bit version.
uint8_t divsufsort(const uint8_t *T, int64_t *SA, int64_t n);
```

To use the library include the header `divsufsort.h`, link against the library `divsufsort` and `libprange`.

Benchmarks
==========
See [Benchmarks](https://github.com/jlabeit/parallel-divsufsort/blob/master/benchmarks/OVERVIEW.md) page for details.
