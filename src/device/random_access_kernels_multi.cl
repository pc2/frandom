#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)
#define UPDATE_SPLIT 128
#define GLOBAL_MEM_UNROLL 16

#pragma OPENCL EXTENSION cl_intel_channels: enable

/**
Number of data access kernels that are generated.

 */
#pragma PY_CODE_GEN block_start
#define DATA_ACCESS_SPLIT $replications$
#pragma PY_CODE_GEN block_end replace()

#define POLY 7

/**
Struct used for the input channels of the write kernels.
Contain the random value, the old data and a boolean value to stop processing.
 */
typedef struct {
	DATA_TYPE_UNSIGNED randoms[GLOBAL_MEM_UNROLL];
}  read_data;

/**
Struct used for the input channels of the write kernels.
Contain the random value, the old data and a boolean value to stop processing.
 */
typedef struct {
	bool isValid[GLOBAL_MEM_UNROLL];
	DATA_TYPE_UNSIGNED ran[GLOBAL_MEM_UNROLL];
	DATA_TYPE old_value[GLOBAL_MEM_UNROLL];
}  write_data;

channel read_data read_channel[DATA_ACCESS_SPLIT];
channel write_data write_channel[DATA_ACCESS_SPLIT];

#pragma PY_CODE_GEN block_start
__kernel 
void addressCalculationKernel$rep$(ulong m, __constant DATA_TYPE_UNSIGNED* ran_const) {
	DATA_TYPE_UNSIGNED ran[UPDATE_SPLIT];
	#pragma unroll GLOBAL_MEM_UNROLL
	for (int i=0; i< UPDATE_SPLIT; i++) {
		ran[i] = ran_const[i];
	}

	DATA_TYPE_UNSIGNED mm1 = m - 1;
	#pragma loop_coalesce 1
	for (ulong i=0; i< (4 * m); i += UPDATE_SPLIT) {
		for (int j=0; j<UPDATE_SPLIT; j +=GLOBAL_MEM_UNROLL) {
			read_data rdata;
			for (int k=0; k< GLOBAL_MEM_UNROLL; k++) {
				DATA_TYPE_UNSIGNED v = 0;
				DATA_TYPE_UNSIGNED ran_tmp = ran[j + k];
				if (((DATA_TYPE) ran_tmp) < 0) {
					v = POLY;
				}
				ran_tmp = (ran_tmp << 1) ^ v;
				ran[j + k] = ran_tmp;
				DATA_TYPE_UNSIGNED address = ran_tmp & mm1;
				rdata.randoms[k] = ran_tmp;
			}
			write_channel_intel(read_channel[$rep$], rdata);
		}
	}
}

__kernel
void readMemory$rep$(__global DATA_TYPE* restrict data, const ulong m,
					const ulong data_chunk_size, 
					const DATA_TYPE_UNSIGNED address_start) {

	DATA_TYPE_UNSIGNED mm1 = m - 1;
	for (ulong i=0; i < 4 * m; i += GLOBAL_MEM_UNROLL) {
		read_data rdata = read_channel_intel(read_channel[$rep$]);
		write_data wdata;
		#pragma unroll
		for (int k=0; k < GLOBAL_MEM_UNROLL; k++) {
			DATA_TYPE_UNSIGNED value = rdata.randoms[k];
			DATA_TYPE_UNSIGNED address = value & mm1;
			address -= address_start;
			if (address < data_chunk_size) {
				DATA_TYPE old_data = data[address];
				wdata.ran[k] = value;
				wdata.isValid[k] = true;
				wdata.old_value[k] = old_data;		
			}
			else {
				wdata.ran[k] = 0;
				wdata.isValid[k] = false;
				wdata.old_value[k] = 0;	
			}
		}
		write_channel_intel(write_channel[$rep$], wdata);
	}
}

__kernel
void writeMemory$rep$(__global DATA_TYPE* restrict data, ulong m,
					const ulong data_chunk_size, 
					const DATA_TYPE_UNSIGNED address_start) {
	for (ulong i=0; i < 4 * m; i += GLOBAL_MEM_UNROLL) {
		write_data wdata = read_channel_intel(write_channel[$rep$]);
		#pragma unroll
		for (int k=0; k < GLOBAL_MEM_UNROLL; k++) {
			if (wdata.isValid[k]) {
				DATA_TYPE_UNSIGNED value = wdata.ran[k];
				DATA_TYPE old_data = wdata.old_value[k];
				DATA_TYPE_UNSIGNED address = value & (m - 1);
				address -= address_start;
				data[address] = old_data ^ value;
			}
		}
    }
}

#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for rep in range(replications)]
