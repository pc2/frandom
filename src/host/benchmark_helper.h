#ifndef BENCHMARK_HELPER_H
#define BENCHMARK_HELPER_H

/* External libraries */
#include "CL/cl.hpp"

/**
Makro to convert the error integer representation to its string representation
Source: https://gist.github.com/allanmac/9328bb2d6a99b86883195f8f78fd1b93
*/
#define CL_ERR_TO_STR(err) case err: return #err

/**
Output separator
*/
# define HLINE "-------------------------------------------------------------\n"

/**
Makro that enables checks for OpenCL errors with handling of the file and
line number.
*/
#define ASSERT_CL(err) bm_helper::handleClReturnCode(err, __FILE__, __LINE__)

namespace bm_helper {

    /**
    Sets up the given FPGA with the kernel in the provided file.

    @param context The context used for the program
    @param program The devices used for the program
    @param usedKernelFile The path to the kernel file
    @return The program that is used to create the benchmark kernels
    */
    cl::Program
    fpgaSetup(cl::Context context, std::vector<cl::Device> deviceList,
                         std::string usedKernelFile);

    /**
    Sets up the C++ environment by configuring std::cout and checking the clock
    granularity using bm_helper::checktick()
    */
    void
    setupEnvironmentAndClocks();

    /**
    Searches an selects an FPGA device using the CL library functions.
    If multiple platforms or devices are given, the user will be prompted to
    choose a device.

    @return A list containing a single selected device
    */
    std::vector<cl::Device>
    selectFPGADevice();


    /**
    Converts the reveived OpenCL error to a string

    @param err The OpenCL error code

    @return The string representation of the OpenCL error code
    */
    std::string
    getCLErrorString(cl_int const err);

    /**
    Check the OpenCL return code for errors.
    If an error is detected, it will be printed and the programm execution is
    stopped.

    @param err The OpenCL error code
    */
    void
    handleClReturnCode(cl_int const err, std::string const file,
                       int const  line);

}  // namespace bm_helper
#endif // BENCHMARK_HELPER_H_INCLUDED
