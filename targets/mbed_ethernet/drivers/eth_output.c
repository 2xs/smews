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
  Time-stamp: <2013-03-22 10:43:18 (hauspie)>
*/
#include <stdint.h>
#include <string.h> /* memcpy */

#include "target.h"

#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/interrupt.h>
#include <rflpc17xx/profiling.h>
#include <rflpc17xx/drivers/dma.h>


#include "hardware.h"
#include "mbed_debug.h"
#include "ip.h"
#include "ethernet.h"
#include "protocols.h"

#include "out_buffers.h"
#include "connections.h"
#include "link_layer_cache.h"


/** This structure holds the buffer where the actual fragment data will be stored */
typedef struct
{
   uint8_t *ptr;
   uint32_t size;
} fragment_buffer_t;

fragment_buffer_t current_buffer; /* in the bss, so initialized at NULL,0 by the start routine */

/** This will be used to store the precomputed Ethernet header.
 * Then, only the dst address will have to be modified for each frame
 */
uint8_t ethernet_header[PROTO_MAC_HLEN];


int mbed_eth_fill_header(const unsigned char *ip)
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

   if (!get_link_layer_address(ip, dst_addr.addr))
   {
       /* A nice and polite implementation would be to make an ARP request
	  (or IPv6 neighbor discovery) and to queue packet for later sending.
	  However... we are not polite :p */
       memset(dst_addr.addr, 0xff, 6);
   }
   idx = 0;
   /* Put the destination addr in the frame header */
   PUT_MAC(ethernet_header, idx, dst_addr.addr);
   return 1;
}

int mbed_eth_prepare_fragment(const uint8_t *data, uint32_t size, int idx, int last)
{
   rflpc_eth_descriptor_t *d;
   rflpc_eth_tx_status_t *s;

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
    rflpc_eth_set_tx_control_word(size, &d->control, 0, last);
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
    current_buffer.size = 0;
}

void mbed_eth_put_byte(uint8_t byte)
{
    if (current_buffer.ptr == NULL)
    {
		MBED_DEBUG("Trying to add byte %02x (%c) while prepare_output has not been successfully called\r\n", byte, byte);
	return;
    }
    current_buffer.ptr[current_buffer.size] = byte;
    current_buffer.size++;
}

/* Use the GPDMA to copy the buffer */
static void memcpy_dma(void *dest, const void *src, uint32_t size)
{
   rflpc_dma_channel_t channel = RFLPC_DMAC0;

   /* Wait for a DMA channel to become ready */
   while (!rflpc_dma_channel_ready(channel))
   {
      if (channel == RFLPC_DMAC7)
         channel = RFLPC_DMAC0;
      else
         channel++;
   }
   /* A channel is ready, launch the dma */
   if (!rflpc_dma_start(channel, dest, src, size))
      MBED_DEBUG("DMA Channel %d was supposed to be ready, but was not!\r\n", channel);
}

void mbed_eth_put_nbytes(const void *bytes, uint32_t n)
{
    if (current_buffer.ptr == NULL)
    {
		MBED_DEBUG("Trying to add %d bytes while prepare_output has not been successfully called\r\n", n);
		return;
    }
    if (n >= DMA_THRESHOLD)
       memcpy_dma(current_buffer.ptr + current_buffer.size, bytes, n);
    else
       memcpy(current_buffer.ptr + current_buffer.size, bytes, n);
    current_buffer.size += n;
}
void mbed_display_ip(const unsigned char *ip);

void mbed_eth_output_done()
{
   /* Generate frame from fragment using gather DMA */
   /* First get the DST IP to fill the DST MAC address */
#ifdef IPV6
	unsigned char dst_ip[16];
#else
	unsigned char dst_ip[4];
#endif
   get_current_remote_ip(dst_ip);
   /* Fill the ethernet header */
   mbed_eth_fill_header(dst_ip);

   /* first, ethernet header */
   mbed_eth_prepare_fragment(ethernet_header, PROTO_MAC_HLEN, 0, 0);
   /* Now, the payload */
   mbed_eth_prepare_fragment(current_buffer.ptr, current_buffer.size, 1, 1);

   /* Done, give all fragments to ethernet DMA */
   rflpc_eth_done_process_tx_packet(2); /* 2 descriptors */
   current_buffer.ptr = 0;
   current_buffer.size = 0;
}
