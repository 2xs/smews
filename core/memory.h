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

/* This module provides a simple dynamic memory allocator. Free blocs are
 * chained, but no information is kept about allocated blocs. When freeing
 * the size is required. Whatever the size argment, only multiple of
 * 4 bytes are managed.
 * BE CAREFULL:
 * This memory allocator has been designed to be as small as possible, and
 * has to be used only inside the Smews kernel. It has to be used only
 * with corrects arguments (don't try to free non allocated data)
*/

/* reset the allocator (free all) */
extern void mem_reset(void);
/* try to allocate a bloc of at least size bytes (but multiple of 4)
 * return NULL if failed */
extern void *mem_alloc(uint16_t size);
/* free a bloc of at least size bytes (but multiple of 4).
 * Only use on free data. */
extern void mem_free(void *ptr, uint16_t size);
/* very basic realloc
 * this function must be reimplemented to avoid memory fragmentation
 * free ptr
 * return NULL if failed */
extern void *mem_realloc(void *ptr, uint16_t size, uint16_t size_to_add);

extern int debug_mem_buffers;
extern int debug_mem_infos;
