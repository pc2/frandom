#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)
#define GLOBAL_MEM_UNROLL 8

#pragma OPENCL EXTENSION cl_intel_channels: enable

/**
Number of data access kernels that are generated.

 */
#pragma PY_CODE_GEN block_start
#define DATA_ACCESS_SPLIT $replications$
#pragma PY_CODE_GEN block_end replace()

#pragma PY_CODE_GEN block_start
#define SINGLE_KERNEL
#pragma PY_CODE_GEN block_end if_cond(replications == 1, CODE, None)

/**
Constant used to generate new address for random access
*/
#define POLY 7

/**
Struct used for the input channels of the memory access kernels.
Contain the random value, the old data and a boolean value to stop processing.
 */
typedef struct {
	bool isRunning;
	bool isValid;
	DATA_TYPE_UNSIGNED ran;
	DATA_TYPE_UNSIGNED actual_index;
}  access_data;

/**
Channel between address calculation and memory access kernel.
*/
channel access_data read_channel[DATA_ACCESS_SPLIT];

/**
The following section contains the implementation of two kernels to generate
random accesses to the global memory.
*/
#pragma PY_CODE_GEN block_start

/**
This kernel calulates all random numbers.
It will only forward those numbers to the memory access kernel, which are within
a specific range.
*/
__kernel
void addressCalculationKernel$rep$(ulong m, ulong data_chunk) {
	DATA_TYPE_UNSIGNED ran = 1;

	#ifndef SINGLE_KERNEL
	DATA_TYPE_UNSIGNED address_start = $rep$ * data_chunk;
	#endif
	DATA_TYPE_UNSIGNED iters = (4 * m);
	for (DATA_TYPE_UNSIGNED i=0; i< iters; i++) {
		DATA_TYPE_UNSIGNED v = 0;
		if (((DATA_TYPE) ran) < 0) {
			v = POLY;
		}
		ran = (ran << 1) ^ v;
		DATA_TYPE_UNSIGNED address = ran & (m - 1);
		bool end = (i == iters - 1);
		#ifndef SINGLE_KERNEL
		DATA_TYPE_UNSIGNED actual_index = address - address_start;
		if ((actual_index < data_chunk) | end) {
			access_data adata;
			adata.isRunning = !end;
			adata.ran = ran;
			adata.isValid = (actual_index < data_chunk);
			adata.actual_index = actual_index;
			write_channel_intel(read_channel[$rep$], adata);
		}
		#else
		access_data adata;
		adata.isRunning = !end;
		adata.ran = ran;
		adata.isValid = true;
		adata.actual_index = address;
		write_channel_intel(read_channel[$rep$], adata);
		#endif
	}
}

#define LOOP_DELAY 2
/**
 Make the updates at the calculated addresses

 data is volatile to remove the cache. Since we have random accesses the cache
 will have a high miss rate anyway.
*/
__kernel
void accessMemory$rep$(__global DATA_TYPE_UNSIGNED* restrict volatile data) {

	// Achieve II of 1 by using a buffer to store incoming update requests
	// The size should be as small as possible to prevent update errors due to
	// buffered values
	access_data load_shift_data[LOOP_DELAY];
	DATA_TYPE_UNSIGNED loaded_data[LOOP_DELAY];

	bool isRunning = true;
	// While this kernel gets valid data, modify values in memory
	// First load multiple data items from memory and then store them back
	while (isRunning) {

		// load data from global memory
		for (int ld =0; ld <LOOP_DELAY; ld++) {
			if (isRunning) {
				load_shift_data[ld] = read_channel_intel(read_channel[$rep$]);
				isRunning = load_shift_data[ld].isRunning;
			}
			else {
				load_shift_data[ld].isValid = false;
			}
			if (load_shift_data[ld].isValid) {
				loaded_data[ld] = data[load_shift_data[ld].actual_index];
			}
		}

		// store data back to global memory
		for (int ld=0; ld<LOOP_DELAY; ld++) {
			if (load_shift_data[ld].isValid) {
				data[load_shift_data[ld].actual_index] = loaded_data[ld] ^ load_shift_data[ld].ran;
			}
		}
	}
}

#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
