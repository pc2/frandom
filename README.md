# Random Access Benchmark for FPGA

This repository contains the Random Access Benchmark for FPGA and its OpenCL kernels.
Currently only the  Intel® FPGA SDK for OpenCL™ utility is supported.

It is a modified implementation of the
[Random Access Benchmark](https://icl.utk.edu/projectsfiles/hpcc/RandomAccess/)
provided in the [HPC Challenge Benchmark](https://icl.utk.edu/hpcc/) suite.
The implementation follows the Python reference implementation given in  
_Introduction to the HPCChallenge Benchmark Suite_ available
[here](http://icl.cs.utk.edu/news_pub/submissions/hpcc-challenge-intro.pdf).

## Build

The Makefile will generate the device code using the code generator given in a submodule.
So to make use of the code generation make sure to check out the repository recursively.

The code has the following dependencies:

- C++ compiler
- Intel® FPGA SDK for OpenCL™
- Python 3 (only for code generation)

Depending on the use case you may want to change certain lines in the
Makefile:

1. Check the location of the used compilers (A C++ compiler and the aoc/aocl)
2. Update the board name in the Makefile or give the new board name as an
   argument `BOARD` to make.
3. Set the number of kernels that should be generated with `REPLICATIONS`.
4. Set the array sizes for the global memory with `GLOBAL_MEM_SIZE`. It should
   be half of the available global memory.
5. Build the host program or the kernels by using the available build targets.

For more detailed information about the available build targets call:

    make

without specifying a target.
This will print a list of the targets together with a short description.

To compile all necessary bitstreams and executables for the benchmark run:

    make host kernel

To make it easier to generate different versions of the kernels, it
is possible to specify a variable `BUILD_SUFFIX` when executing make.
This suffix will be added to the kernel name after generation.

Example:

	make host BUILD_SUFFIX=18.1.1

Will build the host and name the binary after the given build suffix.
So the host would be named `random_18.1.1`.
The file will be placed in a folder called 'bin' in the root of this project.

Moreover it is possible to specifiy additional aoc compiler parameters using the
`AOC_FLAGS` variable.
It can be used to disable memory interleaving for the kernel:

    make kernel AOC_FLAGS=-no-interleaving=default

## Execution

The created host needs the kernel file as a program argument.
The kernel file has to be specified using the `-f` flag like the following:

    ./random_single_18.1.1 -f path/to/file.aocx

It is also possible to give additional settings. To get a more detailed overview
of the available settings execute:

    ./random_single -h

## Implementation Details

The benchmark will measure the elapsed time for performing `4 * GLOBAL_MEM_SIZE`
updates on an data array which should allocate up to half of the available
global memory of the benchmarked device.
The updates are done unaligned and randomly directly on the global memory.
The repository contains two different implementations:
- `single`: Creates one or multiple kernels that are in charge of a subset of the
    data array. Every kernel is calculating all addresses but only updates the
    values in the subset.
- `single_rnd`: Creates one or multiple kernels that are in charge of a subset
    of the data array. The kernels behave the same as `single`, but a set of
    pre-calculated random numbers is given to the kernels so they can do
    multiple updates simultaneously.
- `ndrange`: Implements the random accesses with an NDRange kernel.
    Also takes pre-calculated random numbers similar to the `single_rnd` kernel.

#### Adjustable Parameters

The following table shows the modifiable parameters and in which kernel they
can be used to adjust the code.
Parameters like `REPLICATIONS` and `GLOBAL_MEM_SIZE` can also be adjusted at
runtime in the host code using command line parameters.
Use the `-h` option to show available parameters when running the host code.
You can specify the kernel type using the `TYPE` parameter when running make.
See the example below the table.

| Parameter         | `single`/<br>`single_rnd`/<br>`ndrange`/<br>Host      | Details                                  |
|------------------ | ------------------------------------------------------ | ---------------------------------------- |
| `BOARD`           |:white_check_mark:/:white_check_mark:/:white_check_mark:/:x:     |   Name of the target board               |
| `BUILD_SUFFIX`    |:white_check_mark:/:white_check_mark:/:white_check_mark:/:white_check_mark:| Addition to the kernel name              |
| `AOC_FLAGS`       |:white_check_mark:/:white_check_mark:/:white_check_mark:/:x:               | Additional compile flags for `aoc`       |
| `REPLICATIONS`    |:white_check_mark:/:white_check_mark:/:x:/:white_check_mark:| Number of kernels that are created       |
| `GLOBAL_MEM_SIZE` |:x:/:x:/:x:/:white_check_mark:                              | Number of data items in the data array   |
| `UPDATE_SPLIT`    |:x:/:white_check_mark:/:white_check_mark:/(:white_check_mark:)             | Number of pre-calculated random numbers  |
| `GLOBAL_MEM_UNROLL`|:white_check_mark:/:white_check_mark:/:x:/:x:              | Unrolling of loops that access the global memory |
| `CXX_FLAGS`       |:x:/:x:/:x:/:white_check_mark:                              | Additional C++ compiler flags            |

Example for synthesizing a kernel to create a profiling report:

```bash
make kernel_profile BOARD=p520_hpc_sg280l CXX_FLAGS=-03 \
AOC_FLAGS="-no-interleave=default" REPLICATIONS=4 TYPE=single
```

## Result Interpretation

The host code will print the results of the execution to the standard output.
The result  summary looks similar to this:

    best         mean         GUPS        error
    1.73506e+01  1.73507e+01  2.47540e-01  9.87137e-03

- `best` and `mean` are the fastest and the mean kernel execution time.
    The pure kernel execution time is measured without transferring the buffer
    back and forth to the host.
- `GUPS` contains the calculated metric _Giga Updates per Second_. It takes the
    fastest kernel execution time. The formula is
    $`GUPS = \frac{4 * GLOBAL\_MEM\_SIZE}{ best\_time * 10^9}`$.
- `error` contains the percentage of memory positions with wrong values
    after the updates where made. The maximal allowed error rate of the
    random access benchmark is 1% according to the rules given in the HPCChallenge
    specification.
