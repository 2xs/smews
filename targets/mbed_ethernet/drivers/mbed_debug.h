/*
* Copyright or © or Copr. 2011, Michael Hauspie
*
* Author e-mail: michael.hauspie@lifl.fr
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
/*
  Author: Michael Hauspie <michael.hauspie@univ-lille1.fr>
  Created: 2011-08-31
  Time-stamp: <2011-09-14 17:08:54 (hauspie)>
*/
#ifndef __MBED_DEBUG_H__
#define __MBED_DEBUG_H__

#ifdef MBED_DEBUG_MODE
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/printf.h>


static inline void dump_bytes(const void *ptr, int count)
{
    uint8_t *bytes = (uint8_t*)ptr;
    int i,j, last_line = count - count % 16;
    for (i = 0 ; i < count / 16 ; ++i)
    {
	for (j = 0 ; j < 16 ; ++j)
	{
	    printf("%02x ", bytes[i*16+j]);
	    if (j == 7)
		printf(" ");
	}
	printf("\t");
	for (j = 0 ; j < 16 ; ++j)
	{
	    uint8_t byte = bytes[i*16+j];
	    printf("%c", (byte >= 32 && byte < 127) ? byte : '.');
	    if (j == 7)
		printf(" ");
	}
	printf("\r\n");
    }
    if (count % 16 == 0)
	return;
    for (j = 0 ; j < 16 ; ++j)
    {
	if (j < count % 16)
	    printf("%02x ", bytes[last_line + j]);
	else
	    printf("   ");
	if (j == 7)
	    printf(" ");
    }
    printf("\t");
    for (j = 0 ; j < 16 ; ++j)
    {
	uint8_t byte = bytes[(last_line % 16) + j];
	printf("%c", (byte >= 32 && byte < 127) ? byte : '.');
	if (j == 7)
	    printf(" ");
    }
    printf("\r\n");
}

#define MBED_DUMP_BYTES dump_bytes
#define MBED_DEBUG printf

extern void mbed_dump_packet(rflpc_eth_descriptor_t *d, rflpc_eth_rx_status_t *s, int dump_contents);
#else
#include <rflpc17xx/drivers/ethernet.h>

static inline void dummy_dump_bytes(const void *ptr, int count){}
static inline void dummy_debug(const char *f, ...){}
static inline void mbed_dump_packet(rflpc_eth_descriptor_t *d, rflpc_eth_rx_status_t *s, int dump_contents){}

#define MBED_DUMP_BYTES dummy_dump_bytes
#define MBED_DEBUG dummy_debug

#endif

#define MBED_ASSERT(cond) do {if (!cond) printf("%s(%d): Assertion %s failed\r\n", __FUNCTION__, __LINE__, #cond);}while(0)

#endif



