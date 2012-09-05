/*
* Copyright or © or Copr. 2008, Simon Duquennoy
*
* Author e-mail: simon.duquennoy@lifl.fr
*
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
*
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use,
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info".
*
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability.
*
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or
* data to be ensured and,  more generally, to use and operate it in the
* same conditions as regards security.
*
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/

#include "target.h"

#include "connections.h"

struct free_bloc_s {
	uint16_t size;
	uint16_t to_next;
};

static uint32_t alloc_buffer[ALLOC_SIZE / 4 + 1];
static struct free_bloc_s *first_free;

/* only used for monitoring/debugging purposes */
int get_free_mem() {
	int sum_free = 0;
	struct free_bloc_s *curr_free = first_free;
	while(curr_free->to_next) {
		sum_free += curr_free->size;
		curr_free += curr_free->to_next;
	}
	return sum_free * 4;
}

/* gets the biggest free memory slot, used only for monitoring/debugging */
int get_max_free_mem() {
	int max_free = 0;
	struct free_bloc_s *curr_free = first_free;
	while(curr_free->to_next) {
		if (curr_free->size > max_free)
			max_free = curr_free->size;
		curr_free += curr_free->to_next;
	}
	return max_free * 4;
}

/* reset the allocator (free all) */
void mem_reset(void) {
	first_free = (struct free_bloc_s *)alloc_buffer;
	alloc_buffer[0] = (uint32_t)(ALLOC_SIZE/4) << 16 | (ALLOC_SIZE/4);
}

/* try to allocate a bloc of at least size bytes (but multiple of 4)
 * return NULL if failed */
void *mem_alloc(uint16_t size) {
	struct free_bloc_s *prev_free = NULL;
	struct free_bloc_s *curr_free = first_free;

	/* memory allocation internals work with 32 bits granularity */
	if(size % 4 > 0) {
		size += 4;
	}
	size >>= 2;
	/* look for a free bloc */
	while(curr_free->size && curr_free->size < size) {
		prev_free = curr_free;
		curr_free += curr_free->to_next;
	}
	/* update the free blocs chain */
	if(curr_free->size == size) { /* use the entire bloc */
		if(prev_free == NULL) {
			first_free = curr_free + curr_free->to_next;
		} else {
			prev_free->to_next += curr_free->to_next;
		}
	} else if(curr_free->size >= size) { /* only use the beginning of the bloc */
		struct free_bloc_s *new_free = curr_free + size;
		if(prev_free == NULL) {
			first_free = new_free;
		} else {
			prev_free->to_next += size;
		}
		new_free->size = curr_free->size - size;
		new_free->to_next = curr_free->to_next - size;
	} else { /* no valid bloc has been found */

		return NULL;
	}
	return curr_free;
}

/* subroutine used to chain two consecutive (but not necessarly adjacent) blocs */
static void chain_free_blocs(struct free_bloc_s *b1, struct free_bloc_s *b2) {
	if(b1 + b1->size == b2) {
		b1->to_next = b2->to_next + b1->size;
		b1->size += b2->size;
	} else {
		b1->to_next = b2 - b1;
	}
}

/* free a bloc of at least size bytes (but multiple of 4).
 * Only use on free data. */
void mem_free(void *ptr, uint16_t size) {

	if(ptr != NULL && size != 0) {
		struct free_bloc_s *to_free = ptr;
		struct free_bloc_s *prev_free = NULL;
		struct free_bloc_s *curr_free = first_free;

		/* memory allocation internals work with 32 bits granularity */
		if(size % 4 > 0) {
			size += 4;
		}
		size >>= 2;
		/* look for a free blocs around ptr */
		while(curr_free < to_free) {
			prev_free = curr_free;
			curr_free += curr_free->to_next;
		}
		to_free->size = size;
		to_free->to_next = curr_free - to_free;
		/* test if there is a bloc after ptr */
		if(curr_free->size) {
			chain_free_blocs(to_free, curr_free);
		}
		/* test if there is a free bloc before ptr */
		if(prev_free) {
			chain_free_blocs(prev_free, to_free);
		} else {
			first_free = to_free;
		}
	}
}

/* void print_mem_state()
{
	struct free_bloc_s *curr_free = first_free;
	while(curr_free->to_next) {
		printf("free: %p, size: %d, to_next: %d (%p), current+size: %p\r\n", curr_free, curr_free->size*4,
			   curr_free->to_next*4, curr_free + curr_free->to_next, curr_free + curr_free->size);
		curr_free += curr_free->to_next;
	}
}
*/

/* very basic realloc
 * this function must be reimplemented to avoid memory fragmentation
 * free ptr
 * return NULL if failed */
void *mem_realloc(void *ptr, uint16_t size, uint16_t size_to_add){
	uint16_t i = 0;
	/* allocating new ptr */
	void *new_ptr = mem_alloc((size + size_to_add)*sizeof(char));
	if(!new_ptr){
		mem_free(ptr,size);
		return NULL;
	}
	/* copying data */
	for(i = 0 ; i < size ; i++)
		((char *)new_ptr)[i] = ((char *)ptr)[i];
	/* free ptr */
	mem_free(ptr,size);
	return new_ptr;
}
