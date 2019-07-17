#pragma PY_CODE_GEN use_file("../settings.py")
#define DATA_TYPE long
#define DATA_TYPE_UNSIGNED unsigned DATA_TYPE

#define BIT_SIZE (sizeof(DATA_TYPE) * 8)
#define UPDATE_SPLIT 16

#pragma OPENCL EXTENSION cl_intel_channels: enable

/**
Number of data access kernels that are generated.

 */
#pragma PY_CODE_GEN block_start
#define DATA_ACCESS_SPLIT replications
#pragma PY_CODE_GEN block_end CODE.replace("replications", str(replications))

#define POLY 7

#pragma PY_CODE_GEN block_start
channel DATA_TYPE_UNSIGNED address_channel_$N;
channel DATA_TYPE_UNSIGNED data_channel_$N;
channel DATA_TYPE_UNSIGNED data_address_channel_$N;
#pragma PY_CODE_GEN block_end [CODE.replace("$N", str(i)) for i in range(replications)]

__kernel 
void addressCaclulationKernel(ulong m, __constant DATA_TYPE_UNSIGNED* ran_const) {
	uint mupdate = 4 * m;
	DATA_TYPE_UNSIGNED ran[UPDATE_SPLIT];
	#pragma unroll
	for (int i=0; i< UPDATE_SPLIT; i++) {
		ran[i] = ran_const[i];
	}

	for (int i=0; i< mupdate / UPDATE_SPLIT; i++) {
		#pragma unroll
		for (int j=0; j<UPDATE_SPLIT; j++) {
			DATA_TYPE_UNSIGNED v = 0;
			if (((DATA_TYPE) ran[j]) < 0) {
				v = POLY;
			}
			ran[j] = (ran[j] << 1) ^ v;

			DATA_TYPE_UNSIGNED address = ran[j] & (m - 1);

			switch (address % DATA_ACCESS_SPLIT) {
				#pragma PY_CODE_GEN block_start
				case $N: write_channel_intel(address_channel_$N, ran[j]); break;
				#pragma PY_CODE_GEN block_end [CODE.replace("$N", str(i)) for i in range(replications)]
			}

		}
	}
}

#pragma PY_CODE_GEN block_start
__kernel
void readMemory$N(__global DATA_TYPE* restrict data, ulong m) {
    uint mupdate = 4 * m;
    for (uint i=0; i < mupdate; i++) {
        DATA_TYPE_UNSIGNED value = read_channel_intel(address_channel_$N);
        DATA_TYPE_UNSIGNED address = value & (m - 1);
        write_channel_intel(data_address_channel_$N, address);
        DATA_TYPE old_data = data[address];
        write_channel_intel(data_channel_$N, old_data);
    }
}

__kernel
void writeMemory$N(__global DATA_TYPE* restrict data, ulong m) {
    uint mupdate = 4 * m;
    for (uint i=0; i < mupdate; i++) {
        DATA_TYPE_UNSIGNED address = read_channel_intel(data_address_channel_$N);
        DATA_TYPE old_data = read_channel_intel(data_channel_$N);
        data[address] = old_data;
    }
}

#pragma PY_CODE_GEN block_end [CODE.replace("$N", str(i)) for i in range(replications)]