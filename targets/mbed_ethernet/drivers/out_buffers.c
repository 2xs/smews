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
  Created: 2011-09-27
  Time-stamp: <2011-09-27 13:55:31 (hauspie)>
*/
#include "target.h"

#include <rflpc17xx/rflpc17xx.h>

typedef struct
{
    uint8_t buff[TX_BUFFER_SIZE];
    int in_use;

} out_buffer_t;

/* transmission buffers */
out_buffer_t  _tx_buffers[TX_BUFFER_COUNT] __attribute__ ((section(".out_ram")));;

int mbed_eth_is_releasable_buffer(const void *p)
{
   int i;
   for (i = 0 ; i < TX_BUFFER_COUNT ; ++i)
      if (p == (const void*)_tx_buffers[i].buff)
         return 1;
   return 0;
}

/* needed because the out_ram section is not zeroed at the start */
void mbed_eth_init_tx_buffers()
{
    int i;
    for (i = 0 ; i < TX_BUFFER_COUNT ; ++i)
	_tx_buffers[i].in_use = 0;
}

uint8_t *mbed_eth_get_tx_buffer()
{
    int i;
    for (i = 0 ; i < TX_BUFFER_COUNT ; ++i)
    {
	if (!_tx_buffers[i].in_use)
	{
	    _tx_buffers[i].in_use = 1;
	    return _tx_buffers[i].buff;
	}
    }
    return NULL;
}

void mbed_eth_release_tx_buffer(uint8_t *buffer)
{
    out_buffer_t *obuf = (out_buffer_t*)buffer;
    obuf->in_use = 0;
}

void mbed_eth_dump_tx_buffer_status()
{
    int i;
    printf("TX buffers state\r\n");
    for (i = 0 ; i < TX_BUFFER_COUNT ; ++i)
    {
	printf("%s", _tx_buffers[i].in_use ? "X" : ".");
    }
    printf("\r\n");
}
