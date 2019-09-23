#ifndef EXECUTION_H
#define EXECUTION_H

/* C++ standard library headers */
#include <memory>

/* External library headers */
#include "CL/cl.hpp"


namespace bm_execution {

    struct ExecutionResults {
        std::vector<double> times;
        double errorRate;
    };

    /**
    The actual execution of the benchmark.
    This method can be implemented in multiple *.cpp files. This header enables
    simple exchange of the different calculation methods.

    @param context OpenCL context used to create needed Buffers and queues
    @param device The OpenCL device that is used to execute the benchmarks
    @param program The OpenCL program containing the kernels
    @param repetitions Number of times the kernels are executed
    @param replications Number of times a kernel is replicated - may be used in
                        different ways depending on the implementation of this
                        method
    @param dataSize The size of the data array that may be used for benchmark
                    execution in number of items
    @param useMemInterleaving Prepare buffers using memory interleaving

    @return The time measurements and the error rate counted from the executions
    */
    std::shared_ptr<ExecutionResults>
    calculate(cl::Context context, cl::Device device, cl::Program program,
                   uint repetitions, uint replications, size_t dataSize,
                   bool useMemInterleaving);
}

#endif // SRC_HOST_EXECUTION_H_
