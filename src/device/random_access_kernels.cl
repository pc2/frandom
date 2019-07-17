#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)
#define UPDATE_SPLIT 16

#define POLY 7


__kernel
void accessMemory(__global DATA_TYPE* restrict data, ulong m, __constant DATA_TYPE_UNSIGNED* ran_const) {
	uint mupdate = 4 * m;
	DATA_TYPE_UNSIGNED ran[UPDATE_SPLIT];
	#pragma unroll
	for (int i=0; i< UPDATE_SPLIT; i++) {
		ran[i] = ran_const[i];
	}

	// initiate data array
	// also included in time measurement
	for (DATA_TYPE i=0; i<m; i++) {
		data[i] = i;
	}

	// do random accesses
	#pragma ivdep
	for (int i=0; i< mupdate / UPDATE_SPLIT; i++) {
		#pragma unroll
		for (int j=0; j<UPDATE_SPLIT; j++) {
			DATA_TYPE_UNSIGNED v = 0;
			if (((DATA_TYPE) ran[j]) < 0) {
				v = POLY;
			}
			ran[j] = (ran[j] << 1) ^ v;
			data[ran[j] & (m - 1)] ^= ran[j];
		}
	}
}
