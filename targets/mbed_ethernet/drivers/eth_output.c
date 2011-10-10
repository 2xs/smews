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

uint8_t  * volatile current_tx_frame = NULL;
volatile uint32_t current_tx_frame_idx = 0;
volatile uint32_t current_tx_frame_size = 0;
volatile int current_tx_frame_header_filled = 0;
volatile uint32_t bytes_to_sent = 0;

RFLPC_PROFILE_DECLARE_COUNTER(tx_copy);
RFLPC_PROFILE_DECLARE_COUNTER(tx_eth);

int mbed_eth_fill_header()
{
   EthHead eth;
   uint32_t ip;
   eth.src = local_eth_addr;
   /* Get the IP from the current frame */
   ip = proto_ip_get_dst(current_tx_frame + PROTO_MAC_HLEN);
   if (!arp_get_mac(ip, &eth.dst))
   {
        MBED_DEBUG("No MAC address known for %d.%d.%d.%d, dropping\r\n",
                   ip & 0xFF,
                   (ip >> 8) & 0xFF,
                   (ip >> 16) & 0xFF,
                   (ip >> 24) & 0xFF);
      return 0;
   }
   eth.type = PROTO_IP;
   proto_eth_mangle(&eth, current_tx_frame);
   current_tx_frame_header_filled = 1;
   return 1;
}

inline void mbed_eth_check_and_fill_header()
{
   if (current_tx_frame_header_filled)
      return;
   if (current_tx_frame_idx >= PROTO_MAC_HLEN + PROTO_IP_HLEN)
      mbed_eth_fill_header();
}

void mbed_eth_prepare_output(uint32_t size)
{
    if (current_tx_frame != NULL)
    {
	MBED_DEBUG("Asking to send a new packet while previous not finished\r\n");
	return;
    }
    /* allocated memory for output buffer */
    while ((current_tx_frame = mbed_eth_get_tx_buffer()) == NULL){mbed_eth_garbage_tx_buffers();};
    current_tx_frame_idx = PROTO_MAC_HLEN; /* put the idx at the first IP byte */
    current_tx_frame_size = size + PROTO_MAC_HLEN;
    bytes_to_sent = size + PROTO_MAC_HLEN;
}

void mbed_eth_put_byte(uint8_t byte)
{
   RFLPC_PROFILE_START_COUNTER(tx_copy, RFLPC_TIMER1);
    if (current_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to add byte %02x (%c) while prepare_output has not been successfully called\r\n", byte, byte);
        RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
	return;
    }
    if (current_tx_frame_idx >= current_tx_frame_size)
    {
	MBED_DEBUG("Trying to add byte %02x (%c) and output buffer is full\r\n", byte, byte);
        RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
	return;
    }
    current_tx_frame[current_tx_frame_idx++] = byte;
    RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
}

void mbed_eth_put_nbytes(const void *bytes, uint32_t n)
{
   RFLPC_PROFILE_START_COUNTER(tx_copy, RFLPC_TIMER1);
    if (current_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to add %d bytes while prepare_output has not been successfully called\r\n", n);
        RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
	return;
    }
    if (current_tx_frame_idx + n > current_tx_frame_size)
    {
	MBED_DEBUG("Trying to add %d bytes and output buffer is full (%d/%d)\r\n", n, current_tx_frame_idx, current_tx_frame_size);
        RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
	return;
    }
    memcpy(current_tx_frame + current_tx_frame_idx, bytes, n);
    current_tx_frame_idx += n;
    RFLPC_PROFILE_STOP_COUNTER(tx_copy, RFLPC_TIMER1);
}

int mbed_eth_send_fragment(const uint8_t *data, uint32_t size, int last)
{
   rfEthDescriptor *d;
   rfEthTxStatus *s;

   if (!rflpc_eth_get_current_tx_packet_descriptor(&d, &s))
   {
      MBED_DEBUG("Failed to get current output descriptor, dropping\r\n");
      return 0;
   }
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
    rflpc_eth_set_tx_control_word(size, &d->control, 1, last);
    rflpc_eth_done_process_tx_packet(); /* send packet */

   return 1;
}

void mbed_eth_output_done()
{
    RFLPC_PROFILE_START_COUNTER(tx_eth, RFLPC_TIMER1);

    if (current_tx_frame == NULL) /* Pointer can be NULL if put_const_nbytes has sent the last bytes */
    {
        RFLPC_PROFILE_STOP_COUNTER(tx_eth, RFLPC_TIMER1);
	return;
    }

   mbed_eth_check_and_fill_header();

   if (mbed_eth_send_fragment(current_tx_frame, current_tx_frame_idx, 1) == -1)
      mbed_eth_release_tx_buffer(current_tx_frame);
   current_tx_frame = NULL;
   current_tx_frame_idx = 0;
   current_tx_frame_header_filled = 0;
   RFLPC_PROFILE_STOP_COUNTER(tx_eth, RFLPC_TIMER1);
}

void mbed_eth_put_const_nbytes(const void *bytes, uint32_t n)
{
   //if (n <= 100) /* use multi descriptors only if it is worth */
   {
      mbed_eth_put_nbytes(bytes, n);
      return;
   }
   /* A big fragment can be sent directly.
    * First, send what has already been prepared
    */
   /* fill ethernet header if needed */
   mbed_eth_check_and_fill_header();
   /* send current fragment */
   if (!mbed_eth_send_fragment(current_tx_frame, current_tx_frame_idx, 0))
   {
      /* drop if fragment can not be sent */
      mbed_eth_release_tx_buffer(current_tx_frame);
      current_tx_frame = NULL;
      current_tx_frame_idx = 0;
      current_tx_frame_header_filled = 0;
      return;
   }
   /* some bytes have been sent, report */
   bytes_to_sent -= current_tx_frame_idx;
   /* Fragment is now given to the DMA, prepare the const fragment */
   bytes_to_sent -= n;
   mbed_eth_send_fragment(bytes, n, (bytes_to_sent == 0) ? 1 : 0); /* last fragment bit is set if needed */
   if (bytes_to_sent == 0) /* done with the packet */
   {
      current_tx_frame = NULL;
      current_tx_frame_header_filled = 0;
   }
   else
   {
      /* if there is still data to send, allocate a new buffer */
      while ((current_tx_frame = mbed_eth_get_tx_buffer()) == NULL){mbed_eth_garbage_tx_buffers();};
   }
   current_tx_frame_size = bytes_to_sent;
   current_tx_frame_idx = 0;
}
