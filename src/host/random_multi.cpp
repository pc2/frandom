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

#ifndef RANDOM_LENGTH
#define RANDOM_LENGTH 128
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
#define ADDRESS_CALC_KERNEL "addressCalculationKernel"
#define ADDRESS_FORWARD_KERNEL "addressForwardingKernel"

//constants used to check results
#define POLY 7
#define PERIOD 1317624576693539401L

extern double mysecond();
extern int checktick();
extern DATA_TYPE_UNSIGNED starts(DATA_TYPE n);
extern void fpga_setup_and_execution(cl::Context context, std::vector<cl::Device> DeviceList, char *used_kernel);
extern void calculate(cl::Context context, cl::Device device, cl::Program program);
extern void print_results();


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
    std::cout << "       best       mean      GUOPS      error" << std::endl;
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
    double gups = (4 * long(DATA_LENGTH)) / 1000000000;
    double tmean = 0;
    double tmin = DBL_MAX;
    //calculate mean
    for (int i=0; i<NTIMES;i++) {
        tmean += timearray[i];
        if (timearray[i] < tmin) {
            tmin = timearray[i];
        }
    }
    tmean = tmean / NTIMES;

    double error_count = timearray[NTIMES];

    std::cout << std::setw(11) << tmin << std::setw(11) << tmean << std::setw(11) << gups / tmin
              << std::setw(11) << (100.0 * error_count) / double(DATA_LENGTH) << std::endl;
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

void calculate(cl::Context context, cl::Device device, cl::Program program){
    DATA_TYPE* data;
	DATA_TYPE_UNSIGNED* random;
    posix_memalign((void **)&data,64, sizeof(DATA_TYPE)*DATA_LENGTH);
	posix_memalign((void **)&random,64, sizeof(DATA_TYPE_UNSIGNED)*RANDOM_LENGTH);
    int err;

	//Create Command queue
	#pragma PY_CODE_GEN block_start
	cl::CommandQueue address_queue$rep$(context, device);  
	#pragma PY_CODE_GEN block_start
    cl::CommandQueue $type$_queue$rep$(context, device);
	#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for type in ["read", "write"]]
	#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
    //Create Buffers for input and output
	#pragma PY_CODE_GEN block_start
	cl::Buffer Buffer_data_$rep$(context, CL_CHANNEL_$channel$_INTELFPGA, sizeof(DATA_TYPE)*(DATA_LENGTH / $replications$));
	cl::Buffer Buffer_random$rep$(context, CL_MEM_WRITE_ONLY, sizeof(DATA_TYPE_UNSIGNED) * RANDOM_LENGTH);

    // create the kernels
    cl::Kernel addresskernel$rep$(program, "addressCalculationKernel$rep$", &err);
    assert(err==CL_SUCCESS);

	//prepare kernels
	err = addresskernel$rep$.setArg(0, DATA_TYPE_UNSIGNED(DATA_LENGTH));
	assert(err==CL_SUCCESS);
	err = addresskernel$rep$.setArg(1, Buffer_random$rep$);
	assert(err==CL_SUCCESS);

	#pragma PY_CODE_GEN block_start
    cl::Kernel $type$kernel$rep$(program, "$type$Memory$rep$", &err);
    assert(err==CL_SUCCESS);

	//prepare kernels
    err = $type$kernel$rep$.setArg(0, Buffer_data_$rep$);
    assert(err==CL_SUCCESS);
    err = $type$kernel$rep$.setArg(1, DATA_TYPE_UNSIGNED(DATA_LENGTH));
    assert(err==CL_SUCCESS);
    err = $type$kernel$rep$.setArg(2, DATA_TYPE_UNSIGNED(DATA_LENGTH / $replications$));
    assert(err==CL_SUCCESS);
	err = $type$kernel$rep$.setArg(3, DATA_TYPE_UNSIGNED(($rep$ * DATA_LENGTH) / $replications$));
    assert(err==CL_SUCCESS);

	#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for type in ["read", "write"]]
	#pragma PY_CODE_GEN block_end [replace(replace_dict={**globals(), **locals(), "channel": (rep % 4) + 1}) for rep in range(replications)]

	for (DATA_TYPE i=0; i<RANDOM_LENGTH; i++) {
		random[i] = starts((4 * DATA_LENGTH) / RANDOM_LENGTH * i);
	}
	for (DATA_TYPE i=0; i<DATA_LENGTH; i++) {
		data[i] = i;
	}

	#pragma PY_CODE_GEN block_start
	address_queue$rep$.enqueueWriteBuffer(Buffer_data_$rep$, CL_TRUE, 0, sizeof(DATA_TYPE)*(DATA_LENGTH / $replications$), &data[$rep$ * (DATA_LENGTH / $replications$)]);
	address_queue$rep$.enqueueWriteBuffer(Buffer_random$rep$, CL_TRUE, 0, sizeof(DATA_TYPE_UNSIGNED) * RANDOM_LENGTH, random);
	address_queue$rep$.finish();
	#pragma PY_CODE_GEN block_end [replace(replace_dict={**globals(), **locals()}) for rep in range(replications)]

    double t;
    t = mysecond();
    for (int i = 0; i < NTIMES; i++){
        t = mysecond();
		#pragma PY_CODE_GEN block_start
        address_queue$rep$.enqueueTask(addresskernel$rep$);
		#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
		#pragma PY_CODE_GEN block_start
        $type$_queue$rep$.enqueueTask($type$kernel$rep$);
		#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications) for type in ["read", "write"]]
		#pragma PY_CODE_GEN block_start
        $type$_queue$rep$.finish();
		#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications) for type in ["read", "write"]]
		#pragma PY_CODE_GEN block_start
        address_queue$rep$.finish();
		#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
        t = mysecond() - t;
        timearray[i] = t;
    }

	#pragma PY_CODE_GEN block_start
    write_queue$rep$.enqueueReadBuffer(Buffer_data_$rep$, CL_TRUE, 0, sizeof(DATA_TYPE)*(DATA_LENGTH / $replications$), &data[$rep$ * (DATA_LENGTH / $replications$)]);
	#pragma PY_CODE_GEN block_end [replace(replace_dict={**globals(), **locals()}) for rep in range(replications)]
	#pragma PY_CODE_GEN block_start
	write_queue$rep$.finish();
	#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]

	/* --- Check Results --- */
    DATA_TYPE_UNSIGNED temp = 1;
    for (int i =0;i<4*DATA_LENGTH; i++) {
		
		DATA_TYPE_UNSIGNED v = 0;
		if (((DATA_TYPE) temp) < 0) {
			v = POLY;
		}
		temp = (temp << 1) ^ v;
		data[temp & (DATA_LENGTH - 1)] ^= temp;
	}

	double correct = 0;
	for (DATA_TYPE i=0; i< DATA_LENGTH; i++) {
		if (data[i] != i) {
			correct++;
		}
	}

    timearray[NTIMES] = correct;
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
