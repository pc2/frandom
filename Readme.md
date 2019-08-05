# Random Access Benchmark for FPGA

This repository contains the Random Access Benchmark for FPGA and its OpenCL kernels.
Currently only the  Intel® FPGA SDK for OpenCL™ utility is supported.

It is a modified implementation of the [https://icl.utk.edu/projectsfiles/hpcc/RandomAccess/](Random Access Benchmark) provided in the [https://icl.utk.edu/hpcc/](HPC Challenge Benchmark) suite.
The implementation follows the Python reference implementation given in  _Introduction to the HPCChallenge Benchmark Suite_ available at [http://icl.cs.utk.edu/news_pub/submissions/hpcc-challenge-intro.pdf](here)

## Build

The Makefile will generate the host code using the code generator given in a submodule.
So to make use of the code generation make sure to check out the repository recursively.

The code has the following dependencies:

- C++ compiler
- Intel® FPGA SDK for OpenCL™
- Python 3 (only for code generation)

Depending on the use case you may want to change certain lines in the
Makefile:

1. Check the location of the used compilers (A C++ compiler and the aoc/aocl)
2. Update the board name in the Makefile or give the new board name as an argument BOARD
   to make.
3. Set the size of the random number array with UPDATE_SPLIT.
4. Set the number of kernels that should be generated with REPLICATIONS.
5. Set the array sizes for the global memory with GLOBAL_MEM_SIZE. It should be
half of the available global memory.
6. Build the host program or the kernels by using the available build targets.

For more detailed information about the available build targets call:

    make
without specifying a target.
This will print a list of the targets together with a short description.

To compile all necessary bitstreams and executables for the benchmark run:

    make host kernel

To make it easier to generate different versions of the kernels, it
is possible to specify a variable BUILD_SUFFIX when executing make.
This suffix will be added to the kernel name after generation.

Example:

	make host BUILD_SUFFIX=18.1.1

Will build the host and name the binary after the given build suffix.
So the host would be named 'random_18.1.1'.
The file will be placed in a folder called 'bin' in the root of this project.

Moreover it is possible to specifiy additional aoc compiler parameters using the AOC_FLAGS variable.
It can be used to disable memory interleaving for the kernel:

    make kernel AOC_FLAGS=-no-interleaving=default

## Execution

The created host uses the kernels with the same build suffix by default.
It tries to load it from the same directory it is executed in.

    ./random_18.1.1

will try to load the kernel file with the name
random_access_kernel_18.1.1.aocx by default.
Additionally, the executable will interpret the first argument given as
the path to the kernel that should be used.
For example to use the kernel _other.aocx_:

    ./random_18.1.1 other.aocx

Also, relative and absolute paths to the kernel can be given.

## Result interpretation
