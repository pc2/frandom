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
#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 8
#endif

/* Related header files */
#include "src/host/execution.h"

/* C++ standard library headers */
#include <chrono>
#include <fstream>
#include <memory>
#include <vector>
#include <iostream>

/* External library headers */
#include "CL/cl.hpp"
#include "CL/opencl.h"

/* Project's headers */
#include "src/host/fpga_setup.h"
#include "src/host/random_access_functionality.h"

namespace bm_execution {

    /*
    Implementation for the single_rnd kernel.
     @copydoc bm_execution::calculate()
    */
    std::shared_ptr<ExecutionResults>
    calculate(cl::Context context, cl::Device device, cl::Program program,
                   uint repetitions, uint replications, size_t dataSize,
                   bool useMemInterleaving) {
        // int used to check for OpenCL errors
        int err;

        std::vector<cl::CommandQueue> compute_queue;
        std::vector<DATA_TYPE_UNSIGNED*> random_sets;
        std::vector<cl::Kernel> accesskernel;
        std::vector<DATA_TYPE_UNSIGNED*> data_sets;

        /* --- Prepare kernels --- */

        for (int r=0; r < replications; r++) {
            DATA_TYPE_UNSIGNED* data = reinterpret_cast<DATA_TYPE_UNSIGNED*>(
                            clSVMAlloc(context(), 0 ,
                            sizeof(DATA_TYPE)*(dataSize / replications), 1024));
            data_sets.push_back(data);

            DATA_TYPE_UNSIGNED* random = reinterpret_cast<DATA_TYPE_UNSIGNED*>(
                                    clSVMAlloc(context(), 0 ,
                                    sizeof(DATA_TYPE_UNSIGNED)*UPDATE_SPLIT, 1024));

            for (DATA_TYPE i=0; i < UPDATE_SPLIT; i++) {
                random[i] = starts((4 * DATA_LENGTH) / UPDATE_SPLIT * i);
            }
            random_sets.push_back(random);
            compute_queue.push_back(cl::CommandQueue(context, device));

            // Select memory bank to place data replication
            int channel = 0;
            if (!useMemInterleaving) {
                // Memory interleaving not used in shared memory system
                std::cerr << "Memory interleaving not supported!";
                exit(1);
            }
            accesskernel.push_back(cl::Kernel(program,
                        (RANDOM_ACCESS_KERNEL + std::to_string(r)).c_str() ,
                        &err));
            ASSERT_CL(err);

            // prepare kernels
            err = clSetKernelArgSVMPointerAltera(accesskernel[r](), 0,
                                        reinterpret_cast<void*>(data_sets[r]));
            ASSERT_CL(err);
            err = clSetKernelArgSVMPointerAltera(accesskernel[r](), 1,
                                    reinterpret_cast<void*>(random_sets[r]));
            ASSERT_CL(err);
            err = accesskernel[r].setArg(2, DATA_TYPE_UNSIGNED(dataSize));
            ASSERT_CL(err);
            err = accesskernel[r].setArg(3,
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
                std::chrono::duration_cast<std::chrono::duration<double>>
                                                                    (t2 - t1);
            executionTimes.push_back(timespan.count());
        }

        DATA_TYPE_UNSIGNED* data =
                            new DATA_TYPE_UNSIGNED[sizeof(DATA_TYPE)*dataSize];
        for (size_t r =0; r < replications; r++) {
            for (size_t j=0; j < (dataSize / replications); j++) {
                data[r*(dataSize / replications) + j] = data_sets[r][j];
            }
            clSVMFreeAltera(context(), reinterpret_cast<void *>(data_sets[r]));
            clSVMFreeAltera(context(),
                                    reinterpret_cast<void *>(random_sets[r]));
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
        delete data;

        std::shared_ptr<ExecutionResults> results(
                        new ExecutionResults{executionTimes,
                                             errors / dataSize});
        return results;
    }

}  // namespace bm_execution
