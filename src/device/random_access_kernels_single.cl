/*
Copyright (c) 2019 Marius Meyer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*
The data type used for the data array.
*/
#ifndef DATA_TYPE
#define DATA_TYPE long
#endif

/*
Unsigned version of the data type used for the data array.
*/
#ifndef DATA_TYPE_UNSIGNED
#define DATA_TYPE_UNSIGNED ulong
#endif

/*
Unrolling factor for the read and write pipelines of each kernel.
*/
#ifndef GLOBAL_MEM_UNROLL
#define GLOBAL_MEM_UNROLL 4
#endif

#define POLY 7

#pragma PY_CODE_GEN block_start
#define SINGLE_KERNEL
#pragma PY_CODE_GEN block_end if_cond(replications == 1, CODE, None)

/*
The size of the local memory buffer.
If it is chosen too big, the experienced error might increase because Multiple
updates to the same memory address within this range will be overridden.
*/
#define LOOP_DELAY 1024

/*
Kernel, that will update the given data array accoring to a predefined pseudo-
random access scheme. The overall data array might be equally split between
multiple kernels. In that case, the index of the current split can be given
to the kernel.

@param data The data array that will be updated
@param m The size of the data array
@param data_chunk The chunk index that has to be updated by the kernel
*/
#pragma PY_CODE_GEN block_start
__kernel
void accessMemory$repl$(__global DATA_TYPE_UNSIGNED* restrict volatile data,
						DATA_TYPE_UNSIGNED m,
						DATA_TYPE_UNSIGNED data_chunk) {
	// Initiate the pseudo random number
	DATA_TYPE_UNSIGNED ran = 1;

	// calculate the start of the address range this kernel is responsible for
	#ifndef SINGLE_KERNEL
	DATA_TYPE_UNSIGNED const address_start = $repl$ * data_chunk;
	#endif

	DATA_TYPE_UNSIGNED const mupdate = 4 * m;

	// do random accesses
	#pragma ivdep
	for (DATA_TYPE_UNSIGNED i=0; i< mupdate / LOOP_DELAY; i++) {

		DATA_TYPE_UNSIGNED local_address[LOOP_DELAY];
		DATA_TYPE_UNSIGNED loaded_data[LOOP_DELAY];
		DATA_TYPE_UNSIGNED writeback_data[LOOP_DELAY];
		DATA_TYPE_UNSIGNED update_val[LOOP_DELAY];

		// calculate next addresses
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
		}

		// load the data of the calculated addresses from global memory
		#pragma unroll GLOBAL_MEM_UNROLL
		for (int ld=0; ld< LOOP_DELAY; ld++) {
			#ifdef SINGLE_KERNEL
			loaded_data[ld] = data[local_address[ld]];
			#else
			if (local_address[ld] < data_chunk) {
				loaded_data[ld] = data[local_address[ld]];
			}
			#endif
		}

		// store back the calculated addresses from global memory
		#pragma unroll GLOBAL_MEM_UNROLL
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
