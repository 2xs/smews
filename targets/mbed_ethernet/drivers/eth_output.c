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
  Created: 2011-09-02
  Time-stamp: <2011-09-29 12:55:14 (hauspie)>
*/
#include <stdint.h>
#include <string.h> /* memcpy */

#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/interrupt.h>
#include <rflpc17xx/profiling.h>

#include "hardware.h"
#include "target.h"
#include "mbed_debug.h"
#include "protocols.h"
#include "arp_cache.h"
#include "out_buffers.h"


/** This structure holds a frame fragment.
 *  This will be used to use DMA has gather
 * device to create ethernet frame from multiple fragment */
typedef struct
{
   uint8_t *ptr;
   uint32_t size;
} fragment_gather_t;

/** This structure holds the buffer where the actual fragment data will be stored */
typedef struct
{
   uint8_t *ptr;
   uint32_t fragment_idx;
   uint32_t byte_idx;
} fragment_buffer_t;

fragment_gather_t fragments[TX_MAX_FRAGMENTS];
fragment_buffer_t current_buffer; /* in the bss, so initialized at NULL,0,0,0,0 by the start routine */

/** This will be used to store the precomputed Ethernet header.
 * Then, only the dst address will have to be modified for each frame
 */
uint8_t ethernet_header[PROTO_MAC_HLEN];

int mbed_eth_fill_header(uint32_t ip)
{
   static int first = 1;
   EthAddr dst_addr;
   int idx;

   if (first)
   {
      idx = 6;
      /* First frame sent, create the ethernet header */
      /* Source Address */
      PUT_MAC(ethernet_header, idx, local_eth_addr.addr);
      /* Type */
      PUT_TWO(ethernet_header, idx, PROTO_IP);
      first = 0;
   }

   if (!arp_get_mac(ip, &dst_addr))
   {
        MBED_DEBUG("No MAC address known for %d.%d.%d.%d, dropping\r\n",
                   ip & 0xFF,
                   (ip >> 8) & 0xFF,
                   (ip >> 16) & 0xFF,
                   (ip >> 24) & 0xFF);
      return 0;
   }
   idx = 0;
   /* Put the destination addre in the frame header */
   PUT_MAC(ethernet_header, idx, dst_addr.addr);
   return 1;
}

int mbed_eth_prepare_fragment(const uint8_t *data, uint32_t size, int idx, int last)
{
   rfEthDescriptor *d;
   rfEthTxStatus *s;

   /* Wait for a descriptor to be available for this fragment */
   while (!rflpc_eth_get_current_tx_packet_descriptor(&d, &s, idx));

/* send control word (size + send options) and request interrupt */
    d->packet = (uint8_t *)data;
    s->status_info = PACKET_BEEING_SENT_MAGIC; /* this will allow the interrupt
                                                * handler to check ALL
                                                * descriptors for packets as
                                                * this status word is
                                                * overwriten when a packet is
                                                * sent. Then, if status is not
                                                * this magic AND packet is not
                                                * NULL, it has to be freed */
    rflpc_eth_set_tx_control_word(size, &d->control, last, last);
   return 1;
}

void mbed_eth_prepare_output(uint32_t size)
{
    if (current_buffer.ptr != NULL)
    {
	MBED_DEBUG("Asking to send a new packet while previous not finished\r\n");
	return;
    }
    /* allocated memory for output buffer */
    while ((current_buffer.ptr = mbed_eth_get_tx_buffer()) == NULL){mbed_eth_garbage_tx_buffers();};
    current_buffer.byte_idx = 0;
    current_buffer.fragment_idx = 0;
    fragments[0].ptr = current_buffer.ptr;
    fragments[0].size = 0;
    fragments[1].size = 0;
}

void mbed_eth_put_byte(uint8_t byte)
{
    if (current_buffer.ptr == NULL)
    {
	MBED_DEBUG("Trying to add byte %02x (%c) while prepare_output has not been successfully called\r\n", byte, byte);
	return;
    }
    fragment_gather_t *fragment = &fragments[current_buffer.fragment_idx];
    fragment->ptr[fragment->size++] = byte;
    current_buffer.byte_idx++;
}

void mbed_eth_put_nbytes(const void *bytes, uint32_t n)
{
    if (current_buffer.ptr == NULL)
    {
	MBED_DEBUG("Trying to add %d bytes while prepare_output has not been successfully called\r\n", n);
	return;
    }
    fragment_gather_t *fragment = &fragments[current_buffer.fragment_idx];
    memcpy(fragment->ptr + fragment->size, bytes, n);
    fragment->size += n;
    current_buffer.byte_idx += n;
}

void mbed_eth_put_const_nbytes(const void *bytes, uint32_t n)
{
   /* Check if it is worth to user gather DMA for this const fragment or if we reach the maximum fragment count*/
   //if (n < PUTN_BYTES_CONST_DMA_THRESHOLD || (current_buffer.fragment_idx + 2 >= TX_MAX_FRAGMENTS))
   {
      mbed_eth_put_nbytes(bytes, n);
      return;
   }
   /* The previous fragment is finished, skip to next */
   if (fragments[current_buffer.fragment_idx].size != 0) /* Test if the previous fragment was a const fragment */
      current_buffer.fragment_idx++;
   /* Create a fragment with the current const ptr */
   fragments[current_buffer.fragment_idx].ptr = (uint8_t*)bytes;
   fragments[current_buffer.fragment_idx].size = n;
   /* Prepare next fragment */
   current_buffer.fragment_idx++;
   fragments[current_buffer.fragment_idx].ptr = current_buffer.ptr + current_buffer.byte_idx;
   fragments[current_buffer.fragment_idx].size = 0;
   if (current_buffer.fragment_idx+1 < TX_MAX_FRAGMENTS)
      fragments[current_buffer.fragment_idx+1].size = 0;
}

void mbed_eth_output_done()
{
   /* Generate frame from fragment using gather DMA */
   /* First get the DST IP to fill the DST MAC address */
   uint32_t ip = proto_ip_get_dst(current_buffer.ptr);
   /* Fill the ethernet header */
   mbed_eth_fill_header(ip);
   /* Fragments are ready to be handed to the DMA */
   int i = 0;
   /* first, ethernet header */
   mbed_eth_prepare_fragment(ethernet_header, PROTO_MAC_HLEN, 0, 0);
   /* Now, the payload */
   while ((i+1) < TX_MAX_FRAGMENTS &&  fragments[i+1].size != 0)
   {
      /* non last fragments */
      mbed_eth_prepare_fragment(fragments[i].ptr, fragments[i].size, i+1, 0);
      ++i;
   }
   /* last fragment */
   mbed_eth_prepare_fragment(fragments[i].ptr, fragments[i].size, i+1, 1);
   /* Done, give all fragments to ethernet DMA */
   rflpc_eth_done_process_tx_packet(i+2); /* i + 2 descriptors */
   current_buffer.ptr = 0;
   current_buffer.fragment_idx = 0;
   current_buffer.byte_idx = 0;
}
