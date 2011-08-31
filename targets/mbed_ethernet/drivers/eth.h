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
  Created: 2011-07-15
  Time-stamp: <2011-08-31 13:51:40 (hauspie)>

*/
#ifndef __ETH_H__
#define __ETH_H__

#include <stdint.h>
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/debug.h>

#include "hardware.h"
#include "mbed_eth_debug.h"

typedef struct
{
    uint8_t addr[6];
} EthAddr;

extern EthAddr local_eth_addr;

/* Pointer to the current rx frame */
extern uint8_t *current_eth_rx_frame;
/* Pointer to the next byte to provide to smews */
extern uint32_t current_eth_rx_frame_idx;
extern uint32_t current_eth_rx_frame_size;

extern void process_rx_packet(rfEthDescriptor *d, rfEthRxStatus *s);

/*************************************************************
*                   RX functions                             *
*************************************************************/

/* Gets next available byte */
static inline uint8_t eth_get_next_byte()
{
    uint8_t byte;

    RFLPC_ASSERT_STACK();
    if (current_eth_rx_frame == NULL || current_eth_rx_frame_idx >= current_eth_rx_frame_size)
    {
	MBED_DEBUG("No data, but queried!\r\n");
	return 0;
    }

    byte = current_eth_rx_frame[current_eth_rx_frame_idx++];
    if (current_eth_rx_frame_idx >= current_eth_rx_frame_size)
    {
	rfEthDescriptor *d;
	rfEthRxStatus *s;
	current_eth_rx_frame = NULL;
    	rflpc_eth_done_process_rx_packet(); /* end packet processing */
	/* check if there is another packet available (in case we missed an interrupt */
	if (rflpc_eth_get_current_rx_packet_descriptor(&d, &s))
    	{
    	    /* packet received */
    	    process_rx_packet(d,s);
    	    /* done with it */
    	}
    }
/*    MBED_DEBUG("%0x (%c) %d/%d\r\n", byte, byte, current_eth_rx_frame_idx, current_eth_rx_frame_size);*/
    return byte;
}

/* tests if a new byte is available */
static inline int eth_byte_available()
{
    RFLPC_ASSERT_STACK();
    if (current_eth_rx_frame == NULL)
	return 0;
    if (current_eth_rx_frame_idx >= current_eth_rx_frame_size)
	return 0;
/*    MBED_DEBUG("Byte available\r\n");*/
    return 1;
}



/*************************************************************
*                   TX functions                             *
*************************************************************/

#include "memory.h"

/* Pointer to the current tx frame */
extern uint8_t *current_eth_tx_frame;
extern uint32_t current_eth_tx_frame_idx;
extern uint32_t current_eth_tx_frame_size;


extern void eth_send_current_frame();

static inline void eth_add_byte(uint8_t c)
{
    RFLPC_ASSERT_STACK();
    if (current_eth_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to add byte without tx buffer\r\n");
	return;
    }
    if (current_eth_tx_frame_idx >= current_eth_tx_frame_size)
    {
	MBED_DEBUG("Trying to add byte without tx buffer\r\n");
	return;
    }

    current_eth_tx_frame[current_eth_tx_frame_idx++] = c;
}

extern  void eth_prepare_output(int n);

static inline void eth_done_output()
{
    RFLPC_ASSERT_STACK();
    /* Frame is ready to be sent */
    eth_send_current_frame();
    current_eth_tx_frame = NULL;
}
#endif
