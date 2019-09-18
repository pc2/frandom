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

/* C++ standard library headers */
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <iomanip>
#include <limits>

/* External library headers */
#include "CL/cl.hpp"
#include "CL/cl_ext_intelfpga.h"

/* Project's headers */
#include "src/host/benchmark_helper.h"
#include "src/host/cxxopts.hpp"

/*
Short description of the program
*/
#define PROGRAM_DESCRIPTION "Implementation of the random access benchmark"\
                            " proposed in the HPCC benchmark suite for FPGA"

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

struct ProgramSettings {
    uint numRepetitions;
    uint numReplications;
    size_t dataSize;
    bool useMemInterleaving;
    std::string kernelFileName;
};

struct ExecutionResults {
    std::vector<double> times;
    double errorRate;
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
parseProgramParameters(int argc, char * argv[]) {
    // Defining and parsing program options
    cxxopts::Options options(argv[0], PROGRAM_DESCRIPTION);
    options.add_options()
        ("f,file", "Kernel file name", cxxopts::value<std::string>())
        ("n", "Number of repetitions",
                cxxopts::value<uint>()->default_value(std::to_string(NTIMES)))
        ("r", "Number of used kernel replications",
            cxxopts::value<uint>()->default_value(std::to_string(REPLICATIONS)))
        ("d,data", "Size of the used data array (Should be half of the "\
                       "available global memory)",
                cxxopts::value<size_t>()
                                ->default_value(std::to_string(DATA_LENGTH)))
        ("i,nointerleaving", "Disable memory interleaving")
        ("h,help", "Print this help");
    cxxopts::ParseResult result = options.parse(argc, argv);

    // Check parsed options and handle special cases
    if (result.count("f") <= 0) {
        // Path to the kernel file is mandatory - exit if not given!
        std::cerr << "Kernel file must be given! Aborting" << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }
    if (result.count("h")) {
        // Just print help when argument is given
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // Create program settings from program arguments
    std::shared_ptr<ProgramSettings> sharedSettings(
            new ProgramSettings {result["n"].as<uint>(), result["r"].as<uint>(),
                                result["d"].as<size_t>(),
                                static_cast<bool>(result.count("i") <= 0),
                                result["f"].as<std::string>()});
    return sharedSettings;
}

void printResults(std::shared_ptr<ExecutionResults> results, size_t dataSize) {
    std::cout << std::setw(11) << "type" << std::setw(11) << "best"
              << std::setw(11) << "mean" << std::setw(11) << "GUOPS"
              << std::setw(11) << "error" << std::endl;

    // Calculate performance for kernel execution plus data transfer
    double tmean = 0;
    double tmin = std::numeric_limits<double>::max();
    double gups = static_cast<double>(4 * dataSize) / 1000000000;
    for (double currentTime : results->times) {
        tmean +=  currentTime;
        if (currentTime < tmin) {
            tmin = currentTime;
        }
    }
    tmean = tmean / results->times.size();

    std::cout << std::setw(11) << "calc" << std::setw(11) << tmin
              << std::setw(11) << tmean << std::setw(11) << gups / tmin
              << std::setw(11) << (100.0 * results->errorRate)
              << std::endl;
}

/*
 Prepare kernels and execute benchmark
*/
std::shared_ptr<ExecutionResults>
calculate(cl::Context context, cl::Device device, cl::Program program,
               uint repetitions, uint replications, size_t dataSize,
               bool useMemInterleaving) {
    // int used to check for OpenCL errors
    int err;

    std::vector<cl::CommandQueue> compute_queue;
    std::vector<cl::Buffer> Buffer_data;
    std::vector<cl::Buffer> Buffer_random;
    std::vector<cl::Kernel> accesskernel;
    std::vector<DATA_TYPE_UNSIGNED*> data_sets;

    /* --- Prepare kernels --- */

    for (int r=0; r < replications; r++) {
        DATA_TYPE_UNSIGNED* data;
        posix_memalign(reinterpret_cast<void **>(&data), 64,
                       sizeof(DATA_TYPE)*(dataSize / replications));
        data_sets.push_back(data);

        compute_queue.push_back(cl::CommandQueue(context, device));

        // Select memory bank to place data replication
        int channel = 0;
        if (!useMemInterleaving) {
            switch ((r % replications) + 1) {
                case 1: channel = CL_CHANNEL_1_INTELFPGA; break;
                case 2: channel = CL_CHANNEL_2_INTELFPGA; break;
                case 3: channel = CL_CHANNEL_3_INTELFPGA; break;
                case 4: channel = CL_CHANNEL_4_INTELFPGA; break;
                case 5: channel = CL_CHANNEL_5_INTELFPGA; break;
                case 6: channel = CL_CHANNEL_6_INTELFPGA; break;
                case 7: channel = CL_CHANNEL_7_INTELFPGA; break;
            }
        }

        Buffer_data.push_back(cl::Buffer(context, channel | CL_MEM_READ_WRITE,
                    sizeof(DATA_TYPE_UNSIGNED)*(dataSize / replications)));
        accesskernel.push_back(cl::Kernel(program,
                    (RANDOM_ACCESS_KERNEL + std::to_string(r)).c_str() , &err));
        ASSERT_CL(err);

        // prepare kernels
        err = accesskernel[r].setArg(0, Buffer_data[r]);
        ASSERT_CL(err);
        err = accesskernel[r].setArg(1, DATA_TYPE_UNSIGNED(dataSize));
        ASSERT_CL(err);
        err = accesskernel[r].setArg(2,
                            DATA_TYPE_UNSIGNED(dataSize / replications));
        ASSERT_CL(err);
    }

    /* --- Execute actual benchmark kernels --- */

    double t;
    std::vector<double> executionTimes;
    for (int i = 0; i < repetitions; i++) {
        // prepare data and send them to device
        for (DATA_TYPE_UNSIGNED r =0; r < replications; r++) {
            for (DATA_TYPE_UNSIGNED j=0;
                 j < (dataSize / replications); j++) {
                data_sets[r][j] = r*(dataSize / replications) + j;
            }
        }
        for (int r=0; r < replications; r++) {
            compute_queue[r].enqueueWriteBuffer(Buffer_data[r], CL_TRUE, 0,
                 sizeof(DATA_TYPE)*(dataSize / replications), data_sets[r]);
        }

        // Execute benchmark kernels
        auto t1 = std::chrono::high_resolution_clock::now();
        for (int r=0; r < replications; r++) {
            compute_queue[r].enqueueTask(accesskernel[r]);
        }
        for (int r=0; r < replications; r++) {
            compute_queue[r].finish();
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> timespan =
            std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
        executionTimes.push_back(timespan.count());
    }

    /* --- Read back results from Device --- */

    for (int r=0; r < replications; r++) {
        compute_queue[r].enqueueReadBuffer(Buffer_data[r], CL_TRUE, 0,
                 sizeof(DATA_TYPE)*(dataSize / replications), data_sets[r]);
    }
    DATA_TYPE_UNSIGNED* data;
    posix_memalign(reinterpret_cast<void **>(&data), 64,
                                    (sizeof(DATA_TYPE)*dataSize));
    for (size_t j=0; j < (dataSize / replications); j++) {
        for (size_t r =0; r < replications; r++) {
            data[r*(dataSize / replications) + j] = data_sets[r][j];
        }
    }

    /* --- Check Results --- */

    DATA_TYPE_UNSIGNED temp = 1;
    for (DATA_TYPE_UNSIGNED i=0; i < 4L*dataSize; i++) {
        DATA_TYPE v = 0;
        if (((DATA_TYPE)temp) < 0) {
            v = POLY;
        }
        temp = (temp << 1) ^ v;
        data[temp & (dataSize - 1)] ^= temp;
    }

    double errors = 0;
    for (DATA_TYPE_UNSIGNED i=0; i< dataSize; i++) {
        if (data[i] != i) {
            errors++;
        }
    }
    free(reinterpret_cast<void *>(data));

    std::shared_ptr<ExecutionResults> results(
                    new ExecutionResults{executionTimes, errors / dataSize});
    return results;
}


int main(int argc, char * argv[]) {
    std::shared_ptr<ProgramSettings> programSettings =
                                            parseProgramParameters(argc, argv);
    bm_helper::setupEnvironmentAndClocks();
    std::vector<cl::Device> usedDevice = bm_helper::selectFPGADevice();
    cl::Context context = cl::Context(usedDevice);
    const char* usedKernel = programSettings->kernelFileName.c_str();
    cl::Program program = bm_helper::fpgaSetup(context, usedDevice, usedKernel);

    auto results = calculate(context, usedDevice[0], program,
              programSettings->numRepetitions,
              programSettings->numReplications, programSettings->dataSize,
              programSettings->useMemInterleaving);

    printResults(results, programSettings->dataSize);

    return 0;
}
