#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 128
#endif

#ifndef GLOBAL_MEM_UNROLL
#define GLOBAL_MEM_UNROLL 16
#endif

#define POLY 7


#pragma PY_CODE_GEN block_start
__kernel
void accessMemory$repl$(__global DATA_TYPE* restrict data, ulong m, ulong data_chunk, __constant DATA_TYPE_UNSIGNED* ran_const) {
	uint mupdate = 4 * m;
	DATA_TYPE_UNSIGNED address_start = $repl$ * data_chunk;
	DATA_TYPE_UNSIGNED ran[UPDATE_SPLIT];
	#pragma unroll GLOBAL_MEM_UNROLL
	for (int i=0; i< UPDATE_SPLIT; i++) {
		ran[i] = ran_const[i];
	}

	// initiate data array
	// also included in time measurement
	#pragma unroll GLOBAL_MEM_UNROLL
	for (DATA_TYPE i=0; i<data_chunk; i++) {
		data[i] = address_start + i;
	}

	// do random accesses
	#pragma ivdep
	for (int i=0; i< mupdate / UPDATE_SPLIT; i++) {
		#pragma unroll GLOBAL_MEM_UNROLL
		for (int j=0; j<UPDATE_SPLIT; j++) {
			DATA_TYPE_UNSIGNED v = 0;
			if (((DATA_TYPE) ran[j]) < 0) {
				v = POLY;
			}
			ran[j] = (ran[j] << 1) ^ v;
			DATA_TYPE_UNSIGNED address = ran[j] & (m - 1);
			DATA_TYPE_UNSIGNED local_address = address - address_start;
			if (local_address < data_chunk) {
				data[local_address] ^= ran[j];
			}
		}
	}
}
#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for repl in range(replications)]
