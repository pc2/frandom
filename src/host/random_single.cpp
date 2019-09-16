#include <math.h>
#include <fstream>
#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <string>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <unistd.h>
#include <stdlib.h>
#include <iomanip>
#include "CL/cl.hpp"
#include <CL/cl_ext_intelfpga.h>

# define HLINE "-------------------------------------------------------------\n"

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

#ifndef DATA_LENGTH
#define DATA_LENGTH 67108864
#endif

#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 128
#endif

#ifndef NTIMES
#define NTIMES 1
#endif

#ifndef DATA_TYPE
    #define DATA_TYPE cl_long
#endif
#ifndef DATA_TYPE_UNSIGNED
#define DATA_TYPE_UNSIGNED cl_ulong
#endif

#define BIT_SIZE sizeof(DATA_TYPE) * 8
//64

#ifndef FPGA_KERNEL_FILE_GLOBAL
#define FPGA_KERNEL_FILE_GLOBAL "random_access_kernels.aocx"
#endif
#define RANDOM_ACCESS_KERNEL "accessMemory"

//constants used to check results
#define POLY 7
#define PERIOD 1317624576693539401L

extern double mysecond();
extern int checktick();
extern DATA_TYPE_UNSIGNED starts(DATA_TYPE n);
extern void fpga_setup_and_execution(cl::Context context, std::vector<cl::Device> DeviceList, char *used_kernel);
extern void calculate(cl::Context context, cl::Device device, cl::Program program);
extern void print_results();


static double timearray[2][NTIMES + 1];

int main(int argc, char * argv[])
{
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(6);
    int quantum;

    int err;
// Setting up OpenCL for FPGA
    //Setup Platform
    //Get Platform ID
    std::vector<cl::Platform> PlatformList;
    err = cl::Platform::get(&PlatformList);
    assert(err==CL_SUCCESS);

    cl::Platform platform = PlatformList[0];
    std::cout << "Platform Name: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

    //Setup Device
    //Get Device ID
    std::vector<cl::Device> DeviceList;
    err = PlatformList[0].getDevices(CL_DEVICE_TYPE_ACCELERATOR, &DeviceList);
    assert(err==CL_SUCCESS);

    //Create Context
    cl::Context context(DeviceList);
    assert(err==CL_SUCCESS);
    std::cout << "Device Name:   " << DeviceList[0].getInfo<CL_DEVICE_NAME>() << std::endl;

    if  ( (quantum = checktick()) >= 1) {
        std::cout << "Your clock granularity/precision appears to be " << quantum
                << " microseconds." << std::endl;
    }
    else {
        std::cout << "Your clock granularity appears to be less than one microsecond."
                << std::endl;
        quantum = 1;
    }

    std::cout << HLINE;
    std::cout << "Size of global data array: " << (DATA_LENGTH * sizeof(DATA_TYPE)) / 1000000
              << "MB" << std::endl;
    std::cout << HLINE;

    DeviceList.resize(1);

    //Read in binaries from file
    char* used_kernel = (char*) FPGA_KERNEL_FILE_GLOBAL;
    if (argc > 1) {
        std::cout << "Using kernel given as argument" << std::endl;
        used_kernel = argv[1];
    }
    fpga_setup_and_execution(context, DeviceList, used_kernel);


    // Print results
    print_results();

    return 0;
}

void fpga_setup_and_execution(cl::Context context, std::vector<cl::Device> DeviceList, char *used_kernel) {
    std::cout << "Kernel:        " << used_kernel << std::endl;
    std::cout << HLINE;
    std::ifstream aocx_stream(used_kernel, std::ifstream::binary);
    if (!aocx_stream.is_open()){
        std::cerr << "Not possible to open from given file!" << std::endl;
    }

    std::string prog(std::istreambuf_iterator<char>(aocx_stream), (std::istreambuf_iterator<char>()));
    aocx_stream.seekg(0, aocx_stream.end);
    unsigned file_size = aocx_stream.tellg();
    aocx_stream.seekg(0,aocx_stream.beg);
    char * buf = new char[file_size];
    aocx_stream.read(buf, file_size);

    cl::Program::Binaries mybinaries;
    mybinaries.push_back({buf, file_size});

    // Create the Program from the AOCX file.
    cl::Program program(context, DeviceList, mybinaries);

    std::cout << "Prepared FPGA successfully for global Execution!" << std::endl;
    std::cout << HLINE;
//End prepare FPGA

    calculate(context, DeviceList[0], program);
}

void print_results() {
    std::cout << std::setw(11) << "type" << std::setw(11) << "best" << std::setw(11)
              << "mean" << std::setw(11) << "GUOPS"
              << std::setw(11) << "error" << std::endl;

    // Calculate performance for kernel runtime only
    double gups = double(4 * DATA_LENGTH) / 1000000000;
    double tmean = 0;
    double tmin = DBL_MAX;
    //calculate mean
    for (int i=0; i<NTIMES;i++) {
        tmean += timearray[1][i];
        if (timearray[1][i] < tmin) {
            tmin = timearray[1][i];
        }
    }
    tmean = tmean / NTIMES;

    double error_count = timearray[0][NTIMES];
    std::cout << std::setw(11) << "calc" << std::setw(11) << tmin << std::setw(11)
              << tmean << std::setw(11) << gups / tmin
              << std::setw(11) << (100.0 * error_count) / DATA_LENGTH << std::endl;

    // Calculate performance for kernel execution plus data transfer
    tmean = 0;
    tmin = DBL_MAX;
    for (int i=0; i<NTIMES;i++) {
        double tmp_total = timearray[0][i] + timearray[1][i];
        tmean +=  tmp_total;
        if (tmp_total < tmin) {
            tmin = tmp_total;
        }
    }
    tmean = tmean / NTIMES;

    std::cout << std::setw(11) << "calc+comm" << std::setw(11) << tmin << std::setw(11)
              << tmean << std::setw(11) << gups / tmin
              << std::setw(11) << (100.0 * error_count) / DATA_LENGTH << std::endl;
}

# define    M    20

int
checktick()
    {
    int        i, minDelta, Delta;
    double    t1, t2, timesfound[M];

/*  Collect a sequence of M unique time values from the system. */

    for (i = 0; i < M; i++) {
    t1 = mysecond();
    while( ((t2=mysecond()) - t1) < 1.0E-6 )
        ;
    timesfound[i] = t1 = t2;
    }

/*
 * Determine the minimum difference between these M values.
 * This result will be our estimate (in microseconds) for the
 * clock granularity.
 */

    minDelta = 1000000;
    for (i = 1; i < M; i++) {
    Delta = (int)( 1.0E6 * (timesfound[i]-timesfound[i-1]));
    minDelta = MIN(minDelta, MAX(Delta,0));
    }

   return(minDelta);
    }



/* A gettimeofday routine to give access to the wall
   clock timer on most UNIX-like systems.  */

#include <sys/time.h>

double mysecond()
{
        struct timeval tp;
        struct timezone tzp;
        int i;

        i = gettimeofday(&tp,&tzp);
        return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}

void calculate(cl::Context context, cl::Device device, cl::Program program){
    DATA_TYPE_UNSIGNED* random;
    posix_memalign((void **)&random,64, sizeof(DATA_TYPE_UNSIGNED)*UPDATE_SPLIT);
    int err;
    std::cout << "Prepare FPGA instances" << std::endl;

    std::vector<cl::CommandQueue> compute_queue;
    std::vector<cl::Buffer> Buffer_data;
    std::vector<cl::Buffer> Buffer_random;
    std::vector<cl::Kernel> accesskernel;
    std::vector<DATA_TYPE_UNSIGNED*> data_sets;

    for (int r=0; r < REPLICATIONS; r++) {
        DATA_TYPE_UNSIGNED* data;
        posix_memalign((void **)&data,64, sizeof(DATA_TYPE)*(DATA_LENGTH / REPLICATIONS));
        data_sets.push_back(data);
        // Prepare kernel with ID $rep$
        compute_queue.push_back(cl::CommandQueue(context, device));

        // manually calculate channel flag specified in cl_ext_intelfpga.h as CL_CHANNEL_N_INTELFPGA
        // TODO: change this in case the preprocessor define changes
        int channel = 0;
        switch((r % 4) + 1) {
            case 1: channel = CL_CHANNEL_1_INTELFPGA; break;
            case 2: channel = CL_CHANNEL_2_INTELFPGA; break;
            case 3: channel = CL_CHANNEL_3_INTELFPGA; break;
            case 4: channel = CL_CHANNEL_4_INTELFPGA; break;
            case 5: channel = CL_CHANNEL_5_INTELFPGA; break;
            case 6: channel = CL_CHANNEL_6_INTELFPGA; break;
            case 7: channel = CL_CHANNEL_7_INTELFPGA; break;
        }
        channel = 0;
        //channel = CL_CHANNEL_1_INTELFPGA;
        //int channel = ((r % 7) + 1) << 16;
        Buffer_data.push_back(cl::Buffer(context, channel | CL_MEM_READ_WRITE, sizeof(DATA_TYPE_UNSIGNED)*(DATA_LENGTH / REPLICATIONS)));

        accesskernel.push_back(cl::Kernel(program, ("accessMemory" + std::to_string(r)).c_str() , &err));
        assert(err==CL_SUCCESS);
        //prepare kernels
        err = accesskernel[r].setArg(0, Buffer_data[r]);
        assert(err==CL_SUCCESS);
        err = accesskernel[r].setArg(1, DATA_TYPE_UNSIGNED(DATA_LENGTH));
        assert(err==CL_SUCCESS);
        err = accesskernel[r].setArg(2, DATA_TYPE_UNSIGNED(DATA_LENGTH / REPLICATIONS));
        assert(err==CL_SUCCESS);
    }

    double t;
    double tmin = DBL_MAX;
    for (int i = 0; i < NTIMES; i++){
        std::cout << "Start run " << i << std::endl;
        t = mysecond();
        for (DATA_TYPE_UNSIGNED r =0; r < REPLICATIONS; r++) {
            for (DATA_TYPE_UNSIGNED j=0; j<(DATA_LENGTH / REPLICATIONS); j++) {
                #ifndef EMBARRASSINGLY_PARALLEL
                data_sets[r][j] = r*(DATA_LENGTH / REPLICATIONS) + j;
                #else
                data_sets[r][j] = j;
                #endif
            }
        }
        for (int r=0; r < REPLICATIONS; r++) {
            compute_queue[r].enqueueWriteBuffer(Buffer_data[r], CL_TRUE, 0, sizeof(DATA_TYPE)*(DATA_LENGTH / REPLICATIONS), data_sets[r]);
        }
        timearray[0][i] = mysecond() - t;
        t = mysecond();
        for (int r=0; r < REPLICATIONS; r++) {
            compute_queue[r].enqueueTask(accesskernel[r]);
        }
        for (int r=0; r < REPLICATIONS; r++) {
            compute_queue[r].finish();
        }
        t = mysecond() - t;
        timearray[1][i] = t;
        if (t < tmin) {
            tmin = t;
        }
    }
    std::cout << "Read back results" << std::endl;

    for (int r=0; r < REPLICATIONS; r++) {
        compute_queue[r].enqueueReadBuffer(Buffer_data[r], CL_TRUE, 0, sizeof(DATA_TYPE)*(DATA_LENGTH / REPLICATIONS), data_sets[r]);
    }
    DATA_TYPE_UNSIGNED* data;
    posix_memalign((void **)&data,64, (sizeof(DATA_TYPE)*DATA_LENGTH));
    for (ulong j=0; j<(DATA_LENGTH / REPLICATIONS); j++) {
        for (ulong r =0; r < REPLICATIONS; r++) {
            data[r*(DATA_LENGTH / REPLICATIONS) + j] = data_sets[r][j];
        }
    }

    /* --- Check Results --- */
    DATA_TYPE_UNSIGNED temp = 1;
    for (DATA_TYPE_UNSIGNED i=0; i<4L*DATA_LENGTH; i++) {
        DATA_TYPE v = 0;
        if (((DATA_TYPE) temp) < 0) {
            v = POLY;
        }
        temp = (temp << 1) ^ v;
        data[temp & (DATA_LENGTH - 1)] ^= temp;
    }

    double correct = 0;
    for (DATA_TYPE_UNSIGNED i=0; i< DATA_LENGTH; i++) {
        if (data[i] != i) {
            std::cout << int(data[i]) << " " << int(i) << std::endl;
            correct++;
        }
    }

    timearray[0][NTIMES] = correct;
    free((void*)data);
}

/**
 Pseudo random number generator for memory accesses

 */
DATA_TYPE_UNSIGNED starts(DATA_TYPE n) {
    DATA_TYPE_UNSIGNED m2[BIT_SIZE];

    while (n < 0) {
        n += PERIOD;
    }
    while (n > 0) {
        n -= PERIOD;
    }

    if (n == 0) {
        return 1;
    }

    DATA_TYPE_UNSIGNED temp = 1;
    for (int i=0;i<BIT_SIZE;i++) {
        m2[i] = temp;
        for (int j=0; j<2;j++) {
            DATA_TYPE v = 0;
            if (((DATA_TYPE) temp) < 0) {
                v = POLY;
            }
            temp = (temp << 1) ^ v;
        }
    }
    // DATA_TYPE i = BIT_SIZE - 2;
    // while (i >= 0 && !((n >> i) & 1)) {
    //     i--;
    // }
    int i = 0;
    for (i=BIT_SIZE - 2; i>=0; i--) {
        if ((n >> i) & 1){
            break;
        }
    }

    DATA_TYPE_UNSIGNED ran = 2;
    while (i > 0) {
        temp = 0;
        for (int j=0; j < BIT_SIZE; j++) {
            if ((ran >> j) & 1) {
                temp ^= m2[j];
            }
        }
        ran = temp;
        i--;
        if ((n>>i) & 1) {
            DATA_TYPE v = 0;
            if (((DATA_TYPE) ran) < 0) {
                v = POLY;
            }
            ran = (ran << 1) ^v;
        }
    }
    return ran;
}
