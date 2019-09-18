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
#include "src/host/benchmark_helper.h"

/* C++ standard library */
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>

/* External libraries */
#include "CL/cl.hpp"


/**
Includes several helper functions to develop and execute benchmarks on Intel
FPGAs.
*/
namespace bm_helper {

    cl::Program
    fpgaSetup(cl::Context context, std::vector<cl::Device> deviceList,
              std::string usedKernelFile) {
        std::cout << HLINE;
        std::cout << "FPGA Setup:" << usedKernelFile << std::endl;

        // Open file stream if possible
        std::ifstream aocxStream(usedKernelFile, std::ifstream::binary);
        if (!aocxStream.is_open()) {
            std::cerr << "Not possible to open from given file!" << std::endl;
        }

        // Read in file contents and create program from binaries
        std::string prog(std::istreambuf_iterator<char>(aocxStream),
                        (std::istreambuf_iterator<char>()));
        aocxStream.seekg(0, aocxStream.end);
        unsigned file_size = aocxStream.tellg();
        aocxStream.seekg(0, aocxStream.beg);
        char * buf = new char[file_size];
        aocxStream.read(buf, file_size);

        cl::Program::Binaries mybinaries;
        mybinaries.push_back({buf, file_size});

        // Create the Program from the AOCX file.
        cl::Program program(context, deviceList, mybinaries);

        std::cout << "Prepared FPGA successfully for global Execution!" <<
                     std::endl;
        std::cout << HLINE;
        return program;
    }

    void
    setupEnvironmentAndClocks() {
        std::cout << std::setprecision(5) << std::scientific;

        std::cout << HLINE;
        std::cout << "General setup:" << std::endl;

        // Check clock granularity and output result
        std::cout << "C++ high resolution clock is used." << std::endl;
        std::cout << "The clock precision seems to be "
                  << static_cast<double>
                            (std::chrono::high_resolution_clock::period::num) /
                     std::chrono::high_resolution_clock::period::den * 10e9
                  << "ns" << std::endl;

        std::cout << HLINE;
    }


    std::vector<cl::Device>
    selectFPGADevice() {
        // Integer used to store return codes of OpenCL library calls
        int err;

        std::vector<cl::Platform> platformList;
        err = cl::Platform::get(&platformList);
        ASSERT_CL(err);

        // Choose the target platform
        int chosenPlatformId = 0;
        if (platformList.size() > 1) {
            std::cout << platformList.size() <<
                " platforms have been found. Select the platform by typing a"\
                " number:" << std::endl;
            for (int platformId = 0;
                    platformId < platformList.size(); platformId++) {
                std::cout << platformId << ") " <<
                    platformList[platformId].getInfo<CL_PLATFORM_NAME>() <<
                    std::endl;
            }
            std::cout << "Enter platform id [0-" << platformList.size() << "]:";
            std::cin >> chosenPlatformId;
        }
        cl::Platform platform = platformList[chosenPlatformId];
        std::cout << "Selected Platform: "
                  << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        std::vector<cl::Device> deviceList;
        err = platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &deviceList);
        ASSERT_CL(err);

        // Choose taget device
        int chosenDeviceId = 0;
        if (deviceList.size() > 1) {
            std::cout << deviceList.size() <<
                " devices have been found. Select the platform by typing a"\
                " number:" << std::endl;
            for (int deviceId = 0;
                    deviceId < deviceList.size(); deviceId++) {
                std::cout << deviceId << ") " <<
                    deviceList[deviceId].getInfo<CL_DEVICE_NAME>() <<
                    std::endl;
            }
            std::cout << "Enter device id [0-" << deviceList.size() << "]:";
            std::cin >> chosenDeviceId;
        }
        std::vector<cl::Device> chosenDeviceList;
        chosenDeviceList.push_back(deviceList[chosenDeviceId]);

        // Give selection summary
        std::cout << HLINE;
        std::cout << "Selection summary:" << std::endl;
        std::cout << "Platform Name: " <<
            platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
        std::cout << "Device Name:   " <<
            chosenDeviceList[0].getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << HLINE;

        return chosenDeviceList;
    }


    void
    handleClReturnCode(cl_int const err, std::string const file,
                       int const line) {
        if (err != CL_SUCCESS) {
            std::string err_string = getCLErrorString(err);
            std::cerr << "ERROR in OpenCL library detected! Aborting."
                      << std::endl << file << ":" << line << ": " << err_string
                      << std::endl;
            exit(err);
        }
    }


    std::string
    getCLErrorString(cl_int const err) {
      switch (err) {
          CL_ERR_TO_STR(CL_DEVICE_NOT_FOUND);
          CL_ERR_TO_STR(CL_DEVICE_NOT_AVAILABLE);
          CL_ERR_TO_STR(CL_COMPILER_NOT_AVAILABLE);
          CL_ERR_TO_STR(CL_MEM_OBJECT_ALLOCATION_FAILURE);
          CL_ERR_TO_STR(CL_OUT_OF_RESOURCES);
          CL_ERR_TO_STR(CL_OUT_OF_HOST_MEMORY);
          CL_ERR_TO_STR(CL_PROFILING_INFO_NOT_AVAILABLE);
          CL_ERR_TO_STR(CL_MEM_COPY_OVERLAP);
          CL_ERR_TO_STR(CL_IMAGE_FORMAT_MISMATCH);
          CL_ERR_TO_STR(CL_IMAGE_FORMAT_NOT_SUPPORTED);
          CL_ERR_TO_STR(CL_BUILD_PROGRAM_FAILURE);
          CL_ERR_TO_STR(CL_MAP_FAILURE);
          CL_ERR_TO_STR(CL_MISALIGNED_SUB_BUFFER_OFFSET);
          CL_ERR_TO_STR(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
          CL_ERR_TO_STR(CL_COMPILE_PROGRAM_FAILURE);
          CL_ERR_TO_STR(CL_LINKER_NOT_AVAILABLE);
          CL_ERR_TO_STR(CL_LINK_PROGRAM_FAILURE);
          CL_ERR_TO_STR(CL_DEVICE_PARTITION_FAILED);
          CL_ERR_TO_STR(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
          CL_ERR_TO_STR(CL_INVALID_VALUE);
          CL_ERR_TO_STR(CL_INVALID_DEVICE_TYPE);
          CL_ERR_TO_STR(CL_INVALID_PLATFORM);
          CL_ERR_TO_STR(CL_INVALID_DEVICE);
          CL_ERR_TO_STR(CL_INVALID_CONTEXT);
          CL_ERR_TO_STR(CL_INVALID_QUEUE_PROPERTIES);
          CL_ERR_TO_STR(CL_INVALID_COMMAND_QUEUE);
          CL_ERR_TO_STR(CL_INVALID_HOST_PTR);
          CL_ERR_TO_STR(CL_INVALID_MEM_OBJECT);
          CL_ERR_TO_STR(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
          CL_ERR_TO_STR(CL_INVALID_IMAGE_SIZE);
          CL_ERR_TO_STR(CL_INVALID_SAMPLER);
          CL_ERR_TO_STR(CL_INVALID_BINARY);
          CL_ERR_TO_STR(CL_INVALID_BUILD_OPTIONS);
          CL_ERR_TO_STR(CL_INVALID_PROGRAM);
          CL_ERR_TO_STR(CL_INVALID_PROGRAM_EXECUTABLE);
          CL_ERR_TO_STR(CL_INVALID_KERNEL_NAME);
          CL_ERR_TO_STR(CL_INVALID_KERNEL_DEFINITION);
          CL_ERR_TO_STR(CL_INVALID_KERNEL);
          CL_ERR_TO_STR(CL_INVALID_ARG_INDEX);
          CL_ERR_TO_STR(CL_INVALID_ARG_VALUE);
          CL_ERR_TO_STR(CL_INVALID_ARG_SIZE);
          CL_ERR_TO_STR(CL_INVALID_KERNEL_ARGS);
          CL_ERR_TO_STR(CL_INVALID_WORK_DIMENSION);
          CL_ERR_TO_STR(CL_INVALID_WORK_GROUP_SIZE);
          CL_ERR_TO_STR(CL_INVALID_WORK_ITEM_SIZE);
          CL_ERR_TO_STR(CL_INVALID_GLOBAL_OFFSET);
          CL_ERR_TO_STR(CL_INVALID_EVENT_WAIT_LIST);
          CL_ERR_TO_STR(CL_INVALID_EVENT);
          CL_ERR_TO_STR(CL_INVALID_OPERATION);
          CL_ERR_TO_STR(CL_INVALID_GL_OBJECT);
          CL_ERR_TO_STR(CL_INVALID_BUFFER_SIZE);
          CL_ERR_TO_STR(CL_INVALID_MIP_LEVEL);
          CL_ERR_TO_STR(CL_INVALID_GLOBAL_WORK_SIZE);
          CL_ERR_TO_STR(CL_INVALID_PROPERTY);
          CL_ERR_TO_STR(CL_INVALID_IMAGE_DESCRIPTOR);
          CL_ERR_TO_STR(CL_INVALID_COMPILER_OPTIONS);
          CL_ERR_TO_STR(CL_INVALID_LINKER_OPTIONS);
          CL_ERR_TO_STR(CL_INVALID_DEVICE_PARTITION_COUNT);
         // TODO CL_ERR_TO_STR(CL_INVALID_PIPE_SIZE);
         // TODO CL_ERR_TO_STR(CL_INVALID_DEVICE_QUEUE);

        default:
          return "UNKNOWN ERROR CODE";
        }
    }
}  // namespace bm_helper
