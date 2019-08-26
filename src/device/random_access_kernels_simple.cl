#ifndef DATA_TYPE
#define DATA_TYPE long
#endif

#ifndef DATA_TYPE_UNSIGNED
#define DATA_TYPE_UNSIGNED ulong
#endif

#ifndef UPDATE_SPLIT
#define UPDATE_SPLIT 128
#endif

#ifndef GLOBAL_MEM_UNROLL
#define GLOBAL_MEM_UNROLL 8
#endif

#define POLY 7

#pragma PY_CODE_GEN block_start
#define SINGLE_KERNEL
#pragma PY_CODE_GEN block_end if_cond(replications == 1, CODE, None)

#ifdef EMBARRASSINGLY_PARALLEL
#define SINGLE_KERNEL
#endif

#define LOOP_DELAY 2

#pragma PY_CODE_GEN block_start
__kernel
void accessMemory$repl$(__global DATA_TYPE_UNSIGNED* restrict volatile data,
						DATA_TYPE_UNSIGNED m,
						DATA_TYPE_UNSIGNED data_chunk) {
	DATA_TYPE_UNSIGNED ran = 1;

	#ifndef SINGLE_KERNEL
	DATA_TYPE_UNSIGNED address_start = $repl$ * data_chunk;
	#endif

	DATA_TYPE_UNSIGNED mupdate = 4 * m;

	DATA_TYPE_UNSIGNED local_address[LOOP_DELAY];
	DATA_TYPE_UNSIGNED loaded_data[LOOP_DELAY];
	DATA_TYPE_UNSIGNED update_val[LOOP_DELAY];

	// do random accesses
	for (DATA_TYPE_UNSIGNED i=0; i< mupdate / LOOP_DELAY; i++) {

		for (int ld=0; ld< LOOP_DELAY; ld++) {
			DATA_TYPE v = 0;
			if (((DATA_TYPE) ran) < 0) {
				v = POLY;
			}
			ran = (ran << 1) ^ v;
			update_val[ld] = ran;
			DATA_TYPE_UNSIGNED address = ran & (m - 1);
			#ifndef SINGLE_KERNEL
			local_address[ld] = address - address_start;
			#else
			local_address[ld] = address;
			#endif
			#ifdef SINGLE_KERNEL
			loaded_data[ld] = data[local_address[ld]];
			#else
			if (local_address[ld] < data_chunk) {
				loaded_data[ld] = data[local_address[ld]];
			}
			#endif
		}
		for (int ld=0; ld< LOOP_DELAY; ld++) {
			#ifdef SINGLE_KERNEL
			data[local_address[ld]] = loaded_data[ld] ^update_val[ld];
			#else
			if (local_address[ld] < data_chunk) {
				data[local_address[ld]] = loaded_data[ld] ^ update_val[ld];
			}
			#endif
		}
	}
}
#pragma PY_CODE_GEN block_end [replace(replace_dict=locals()) for repl in range(replications)]
