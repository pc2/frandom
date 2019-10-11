#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 1024
#endif

#define POLY 7


// SIMD not used, and instead CU replication since we have random accesses
__attribute__((num_simd_work_items(1)))
__attribute__((num_compute_units(REPLICATIONS)))
__kernel
void accessMemory(__global volatile DATA_TYPE_UNSIGNED* restrict data,
                  __global const DATA_TYPE_UNSIGNED* restrict ran_const,
                  ulong m) {
	DATA_TYPE_UNSIGNED ran = ran_const[get_global_id(0)];

	uint mupdate = 4 * m;
	// do random accesses
	for (int i=0; i< mupdate / UPDATE_SPLIT; i++) {
		DATA_TYPE_UNSIGNED v = 0;
		if (((DATA_TYPE) ran) < 0) {
			v = POLY;
		}
		ran = (ran << 1) ^ v;
		DATA_TYPE_UNSIGNED address = ran & (m - 1);
		data[address] ^= ran;

	}
}
