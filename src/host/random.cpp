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

#ifndef NTIMES
#define NTIMES 5
#endif

#ifndef DATA_TYPE
    #define DATA_TYPE cl_long
#endif
#ifndef DATA_TYPE_UNSIGNED
#define DATA_TYPE_UNSIGNED cl_ulong
#endif

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)

#ifndef FPGA_KERNEL_FILE_GLOBAL
#define FPGA_KERNEL_FILE_GLOBAL "random_access_kernels.aocx"
#endif
#ifndef FPGA_KERNEL_FILE_LOCAL
#define FPGA_KERNEL_FILE_LOCAL "random_access_kernels_local.aocx"
#endif
#define RANDOM_ACCESS_KERNEL "accessMemory"

//constants used to check results
#define POLY 7
#define PERIOD 1317624576693539401L

extern double mysecond();
extern int checktick();
extern DATA_TYPE_UNSIGNED starts(DATA_TYPE n);
extern void fpga_setup_and_execution(cl::Context context, std::vector<cl::Device> DeviceList, char *used_kernel, long data_length);
extern void calculate(cl::Context context, cl::Device device, cl::Program program, int ntimes, cl_ulong data_length);
extern void print_results(int ntimes, long data_size);


static double timearray[NTIMES + 1];

int main(int argc, char * argv[])
{
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    std::cout.precision(3);
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
    std::cout << "Size of local data array: " << (DATA_LENGTH_LOCAL * sizeof(DATA_TYPE)) / 1000000
              << "MB" << std::endl;
    std::cout << HLINE;

    DeviceList.resize(1);

	//Read in binaries from file
    char* used_kernel = (char*) FPGA_KERNEL_FILE_GLOBAL;
    if (argc > 1) {
        std::cout << "Using kernel given as argument" << std::endl;
        used_kernel = argv[1];
    }
    fpga_setup_and_execution(context, DeviceList, used_kernel, DATA_LENGTH);
    

    // Print results
    std::cout << "       best       mean      GUOPS      error" << std::endl;
    print_results(0,NTIMES,DATA_LENGTH);

    return 0;
}

void fpga_setup_and_execution(cl::Context context, std::vector<cl::Device> DeviceList, char *used_kernel, long data_length) {
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

    calculate(context, DeviceList[0], program, NTIMES, data_length);
}

void print_results(int ntimes, long data_size) {
    double gups = (4 * data_size) / 1000000000;
    double tmean = 0;
    double tmin = DBL_MAX;
    //calculate mean
    for (int i=0; i<ntimes;i++) {
        tmean += timearray[i];
        if (timearray[i] < tmin) {
            tmin = timearray[i];
        }
    }
    tmean = tmean / ntimes;

    long error_count = timearray[ntimes];

    std::cout << std::setw(11) << tmin << std::setw(11) << tmean << std::setw(11) << gups / tmin
              << std::setw(11) << (100.0 * error_count) / data_size << std::endl;
}

# define	M	20

int
checktick()
    {
    int		i, minDelta, Delta;
    double	t1, t2, timesfound[M];

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

void calculate(cl::Context context, cl::Device device, cl::Program program, int ntimes, cl_ulong data_length){
    DATA_TYPE* data;
    posix_memalign((void **)&data,64, sizeof(DATA_TYPE)*data_length);
    int err;

	//Create Command queue
	cl::CommandQueue compute_queue(context, device);  

    //Create Buffers for input and output
	cl::Buffer Buffer_data(context, CL_MEM_READ_ONLY, sizeof(DATA_TYPE)*data_length);

    // create the kernels
    cl::Kernel accesskernel(program, RANDOM_ACCESS_KERNEL, &err);
    assert(err==CL_SUCCESS);

	//prepare kernels
	err = accesskernel.setArg(0, Buffer_data);
	assert(err==CL_SUCCESS);
	err = accesskernel.setArg(1, data_length);
	assert(err==CL_SUCCESS);

    double t;
    double tmin = DBL_MAX;
    t = mysecond();
    for (int i = 0; i < ntimes; i++){
        t = mysecond();
        compute_queue.enqueueTask(accesskernel);
		compute_queue.finish();	
		t = mysecond() - t;
		timearray[row][i] = t;
		if (t < tmin) {
			tmin = t;
		}
    }

	compute_queue.enqueueReadBuffer(Buffer_data, CL_TRUE, 0, sizeof(DATA_TYPE)*data_length, data);
    err=compute_queue.finish();

	/* --- Check Results --- */
	DATA_TYPE_UNSIGNED temp = 1;
	for (int i =0;i<4*data_length; i++) {
		DATA_TYPE_UNSIGNED v = 0;
		if (((DATA_TYPE) temp) < 0) {
			v = POLY;
		}
		temp = (temp << 1) ^ v;
		data[temp & (data_length - 1)] ^= temp;
	}

	temp = 0;
	for (int i=0; i< data_length; i++) {
		if (data[i] != i) {
			temp++;
		}
	}

    timearray[ntimes];
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
			DATA_TYPE_UNSIGNED v = 0;
			if (((DATA_TYPE) temp) < 0) {
				v = POLY;
			}
			temp = (temp << 1) ^ v;
		}
	}
	DATA_TYPE i = BIT_SIZE - 2;
	while (i >= 0 && !((n >> i) & 1)) {
		i--;
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
			DATA_TYPE_UNSIGNED v = 0;
			if (((DATA_TYPE) ran) < 0) {
				v = POLY;
			}
			ran = (ran << 1) ^v;
		}
	}
	return ran;
}