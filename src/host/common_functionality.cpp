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

/* Related header files */
#include "src/host/common_functionality.h"

/* C++ standard library headers */
#include <iostream>
#include <string>
#include <limits>
#include <iomanip>
#include <memory>
#include <vector>

/* External library headers */
#include "CL/cl.hpp"
#if QUARTUS_MAJOR_VERSION > 18
#include "CL/cl_ext_intelfpga.h"
#endif

/* Project's headers */
#include "src/host/benchmark_helper.h"
#include "src/host/cxxopts.hpp"
#include "src/host/execution.h"


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

/**
Print the benchmark Results

@param results The result struct provided by the calculation call
@param dataSize The size of the used data array

*/
void printResults(std::shared_ptr<bm_execution::ExecutionResults> results,
                  size_t dataSize) {
    std::cout << std::setw(ENTRY_SPACE)
              << "best" << std::setw(ENTRY_SPACE) << "mean"
              << std::setw(ENTRY_SPACE) << "GUOPS"
              << std::setw(ENTRY_SPACE) << "error" << std::endl;

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

    std::cout << std::setw(ENTRY_SPACE)
              << tmin << std::setw(ENTRY_SPACE) << tmean
              << std::setw(ENTRY_SPACE) << gups / tmin
              << std::setw(ENTRY_SPACE) << (100.0 * results->errorRate)
              << std::endl;
}

/**
The program entry point.
Prepares the FPGA and executes the kernels on the device.
*/
int main(int argc, char * argv[]) {
    // Setup benchmark
    std::shared_ptr<ProgramSettings> programSettings =
                                            parseProgramParameters(argc, argv);
    bm_helper::setupEnvironmentAndClocks();
    std::vector<cl::Device> usedDevice = bm_helper::selectFPGADevice();
    cl::Context context = cl::Context(usedDevice);
    const char* usedKernel = programSettings->kernelFileName.c_str();
    cl::Program program = bm_helper::fpgaSetup(context, usedDevice, usedKernel);

    // Give setup summary
    std::cout << "Summary:" << std::endl
              << "Kernel Replications: " << programSettings->numReplications
              << std::endl
              << "Repetitions:         " << programSettings->numRepetitions
              << std::endl
              << "Total data size:     " << (programSettings->dataSize
                                            * sizeof(DATA_TYPE))
                                         << " Byte" << std::endl
              << "Memory Interleaving: " << programSettings->useMemInterleaving
              << std::endl
              << "Kernel file:         " << programSettings->kernelFileName
              << std::endl
              << "Device:              "
              << usedDevice[0].getInfo<CL_DEVICE_NAME>() << std::endl
              << HLINE
              << "Start benchmark using the given configuration." << std::endl
              << HLINE;

    // Start actual benchmark
    auto results = bm_execution::calculate(context, usedDevice[0], program,
              programSettings->numRepetitions,
              programSettings->numReplications, programSettings->dataSize,
              programSettings->useMemInterleaving);

    printResults(results, programSettings->dataSize);

    return 0;
}
