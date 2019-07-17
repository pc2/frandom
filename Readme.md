# Random Access Benchmark for FPGA

This repository contains the Random Access Benchmark for FPGA and its OpenCL kernels.
Currently only the  Intel® FPGA SDK for OpenCL™ utility is supported.

It is a modified implementation of the [https://icl.utk.edu/projectsfiles/hpcc/RandomAccess/](Random Access Benchmark) provided in the [https://icl.utk.edu/hpcc/](HPC Challenge Benchmark) suite.
The implementation follows the Python reference implementation given in  _Introduction to the HPCChallenge Benchmark Suite_ available at [http://icl.cs.utk.edu/news_pub/submissions/hpcc-challenge-intro.pdf](here)

## Build

Depending on the use case you may want to change certain lines in the
Makefile:
     
1. Check the location of the used compilers (A C++ compiler and the aoc/aocl)
2. Update the board name in the Makefile or give the new board name as an argument BOARD
   to make.
3. Set the number of parallel update requests with UPDATE_SPLIT.
4. Set the array sizes for the global nad the local memory benchmark with
   GLOBAL_MEM_SIZE and LOCAL_MEM_SIZE.
5. Build the host program or the kernels by using the available build targets.

For more detailed information about the available build targets call:

    make
without specifying a target.
This will print a list of the targets together with a short description.

To compile all necessary bitstreams and executables for the benchmark run:

    make host kernel local_kernel

To make it easier to generate different versions of the kernels, it
is possible to specify a variable BUILD_SUFFIX when executing make.
This suffix will be added to the kernel name after generation.

Example:
		
	make host BUILD_SUFFIX=18.1.1

Will build the host and name the binary after the given build suffix.
So the host would be named 'random_18.1.1'.
The file will be placed in a folder called 'bin' in the root of this project.

Moreover it is possible to specifiy additional aoc compiler parameters using the AOC_PARAMS variable.
It can be used to disable memory interleaving for the kernel:

    make AOC_PARAMS=-no-interleaving=default


## Execution

The created host uses the kernels with the same build suffix by default.
It tries to load it from the same directory it is executed in.
   
    ./random_18.1.1 

will try to load the kernel file with the name
random_access_kernel_18.1.1.aocx and random_access_kernel_18.1.1_local.aocx by default.
Note, that this benchmark uses two different bitsreams: One to benchmark the global memory 
and one for the local memory.
Additionally, the executable will interpret the first two arguments given as
the path to the kernels that should be used.
For example to use the kernel _other.aocx_ for global memory and *other_local.aocx*:

    ./random_18.1.1 other.aocx other_local.aocx

Also, relative and absolute paths to the kernel can be given.

## Result interpretation


