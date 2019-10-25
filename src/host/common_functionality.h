/*
Copyright (c) 2019 Marius Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef COMMON_FUNCTIONALITY_H
#define COMMON_FUNCTIONALITY_H

/* C++ standard library headers */
#include <memory>

/* Project's headers */
#include "src/host/execution.h"

/*
Short description of the program
*/

#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#define PROGRAM_DESCRIPTION "Implementation of the random access benchmark"\
                            " proposed in the HPCC benchmark suite for FPGA.\n"\
                            "Version: " STR(VERSION) "\nBuild date: "\
                            STR(BUILD_DATE)

/*
Total length of the data array used for random accesses.
The array should allocate half of the available global memory space.
Keep in mind that this also depends on DATA_TYPE.
*/
#ifndef DATA_LENGTH
#define DATA_LENGTH 67108864
#endif

/*
Number of times the execution of the benchmark will be repeated.
*/
#ifndef NTIMES
#define NTIMES 1
#endif

/*
The data type used for the random accesses.
Note that it should be big enough to address the whole data array. Moreover it
has to be the same type as in the used kernels.
The signed and unsigned form of the data type have to be given separately.
*/
#ifndef DATA_TYPE
    #define DATA_TYPE cl_long
#endif
#ifndef DATA_TYPE_UNSIGNED
#define DATA_TYPE_UNSIGNED cl_ulong
#endif

/*
Prefix of the function name of the used kernel.
It will be used to construct the full function name for the case of replications.
The full name will be
*/
#define RANDOM_ACCESS_KERNEL "accessMemory"

/*
Constants used to verify benchmark results
*/
#define POLY 7
#define PERIOD 1317624576693539401L

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)

#define ENTRY_SPACE 13

struct ProgramSettings {
    uint numRepetitions;
    uint numReplications;
    int defaultPlatform;
    int defaultDevice;
    size_t dataSize;
    bool useMemInterleaving;
    std::string kernelFileName;
};


/**
Parses and returns program options using the cxxopts library.
Supports the following parameters:
    - file name of the FPGA kernel file (-f,--file)
    - number of repetitions (-n)
    - number of kernel replications (-r)
    - data size (-d)
    - use memory interleaving
@see https://github.com/jarro2783/cxxopts

@return program settings that are created from the given program arguments
*/
std::shared_ptr<ProgramSettings>
parseProgramParameters(int argc, char * argv[]);


/**
 Generates the value of the random number after a desired number of updates

 @param n number of random number updates

 @return The random number after n number of updates
 */
DATA_TYPE_UNSIGNED
starts(DATA_TYPE n);

/**
Prints the execution results to stdout

@param results The execution results
@param dataSize Size of the used data array. Needed to calculate GUOP/s from
                timings
*/
void printResults(std::shared_ptr<bm_execution::ExecutionResults> results,
                  size_t dataSize);


/**
The program entry point
*/
int main(int argc, char * argv[]);

#endif // SRC_HOST_COMMON_FUNCTIONALITY_H_
