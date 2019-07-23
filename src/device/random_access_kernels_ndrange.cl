#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 128
#endif

#define POLY 7

#pragma PY_CODE_GEN block_start
__kernel
void accessMemory$repl$(__global DATA_TYPE* volatile data, ulong m, DATA_TYPE_UNSIGNED data_chunk, __constant DATA_TYPE_UNSIGNED* ran_const, DATA_TYPE_UNSIGNED address_start) {
	DATA_TYPE_UNSIGNED ran;
	int ran_id = get_global_id(0);
	ran = ran_const[ran_id];

	uint mupdate = 4 * m;
	// do random accesses
	for (int i=0; i< mupdate / UPDATE_SPLIT; i++) {
		DATA_TYPE_UNSIGNED v = 0;
		if (((DATA_TYPE) ran) < 0) {
			v = POLY;
		}
		ran = (ran << 1) ^ v;
		DATA_TYPE_UNSIGNED address = ran & (m - 1);
		DATA_TYPE_UNSIGNED local_address = address - address_start;
		if (local_address < data_chunk) {
			data[local_address] ^= ran;
		}
	}
}
#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for repl in range(replications)]

//  0 1 1 0 0 
//  1 0 1 0 1
//  0 0 1 1 1
// -----------
//  1 1 1 1 0
//  1 1 1 1 

