#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)
#define UPDATE_SPLIT 128
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

#define POLY 7

/**
Struct used for the input channels of the write kernels.
Contain the random value, the old data and a boolean value to stop processing.
 */
typedef struct {
	bool isRunning;
	DATA_TYPE_UNSIGNED ran;
	DATA_TYPE_UNSIGNED actual_index;
}  access_data;

channel access_data read_channel[DATA_ACCESS_SPLIT];

#pragma PY_CODE_GEN block_start
__kernel 
void addressCalculationKernel$rep$(ulong m, ulong data_chunk, __constant DATA_TYPE_UNSIGNED* ran_const) {
	DATA_TYPE_UNSIGNED ran[UPDATE_SPLIT];
	#pragma unroll GLOBAL_MEM_UNROLL
	for (int i=0; i< UPDATE_SPLIT; i++) {
		ran[i] = ran_const[i];
	}

	#ifndef SINGLE_KERNEL
	DATA_TYPE_UNSIGNED address_start = $rep$ * data_chunk;
	#endif
	uint iters = (4 * m) / UPDATE_SPLIT;
	#pragma loop_coalesce 1
	for (uint i=0; i< iters; i++) {
		for (int j=0; j<UPDATE_SPLIT; j++) {
			DATA_TYPE_UNSIGNED v = 0;
			DATA_TYPE_UNSIGNED ran_tmp = ran[j];
			if (((DATA_TYPE) ran_tmp) < 0) {
				v = POLY;
			}
			ran_tmp = (ran_tmp << 1) ^ v;
			ran[j] = ran_tmp;
			DATA_TYPE_UNSIGNED address = ran_tmp & (m - 1);
			bool end = (i == iters - 1) & (j == UPDATE_SPLIT - 1);
			#ifndef SINGLE_KERNEL
			DATA_TYPE_UNSIGNED actual_index = address - address_start;
			if ((actual_index < data_chunk) | end) {
				access_data adata;
				adata.isRunning = !end;
				adata.ran = ran_tmp;
				adata.actual_index = actual_index;
				write_channel_intel(read_channel[$rep$], adata);
			}
			#else
			access_data adata;
			adata.isRunning = !end;
			adata.ran = ran_tmp;
			adata.actual_index = address;
			write_channel_intel(read_channel[$rep$], adata);
			#endif
		}
	}
}

__kernel
void accessMemory$rep$(__global DATA_TYPE* restrict data) {
	bool isRunning = true;
	while (isRunning) {
		access_data adata = read_channel_intel(read_channel[$rep$]);
		isRunning = adata.isRunning;
		if (adata.isRunning) {
			data[adata.actual_index] ^= adata.ran;
		}
	}
}

#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
