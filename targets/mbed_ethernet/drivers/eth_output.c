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
  Time-stamp: <2011-09-08 17:26:38 (hauspie)>
*/
#include <stdint.h>
#include <string.h> /* memcpy */

#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/interrupt.h>

#include "hardware.h"
#include "target.h"
#include "mbed_debug.h"
#include "protocols.h"
#include "arp_cache.h"


typedef struct
{
    uint8_t in_use;
    uint8_t buffer[TX_BUFFER_SIZE];
} output_buffer_t;


#define OUTPUT_BUFFER_COUNT TX_DESCRIPTORS

output_buffer_t _output_buffers[OUTPUT_BUFFER_COUNT];


output_buffer_t *current_tx_frame = NULL;
uint32_t current_tx_frame_idx = 0;

volatile int get_count = 0;
volatile int release_count = 0;

output_buffer_t *get_free_output_buffer(uint32_t size)
{
    int i;
    output_buffer_t *b = NULL;
    rflpc_irq_global_disable();
    for (i = 0 ; i < OUTPUT_BUFFER_COUNT ; ++i)
    {
	if (!_output_buffers[i].in_use)
	{
	    _output_buffers[i].in_use = 1;
	    b = _output_buffers + i;
	    break;
	}
    }
    rflpc_irq_global_enable();
    return b;
}

void release_output_buffer(output_buffer_t *buffer)
{
    rflpc_irq_global_disable();
    release_count++;
    buffer->in_use = 0;
    rflpc_irq_global_enable();
}

void release_output_buffer_from_packet(uint8_t *packet)
{
    output_buffer_t *b = (output_buffer_t *) (packet - (_output_buffers[0].buffer - (uint8_t*) &_output_buffers[0]));
    release_output_buffer(b);
}

void dump_output_buffers()
{
    int i;
    for (i = 0 ; i < OUTPUT_BUFFER_COUNT ; ++i)
    {
	MBED_DEBUG("%p: %c, ", &_output_buffers[i], _output_buffers[i].in_use ? 'o' : '.');
    }
    MBED_DEBUG(" (%d/%d) \r\n", get_count, release_count);
}


void mbed_eth_prepare_output(uint32_t size)
{
    if (current_tx_frame != NULL)
    {
	MBED_DEBUG("Asking to send a new packet while previous not finished\r\n");
	return;
    }
    if (size + PROTO_MAC_HLEN > TX_BUFFER_SIZE)
    {
	MBED_DEBUG("Trying to send a %d bytes packet. Dropping\r\n", size);
	return;
    }
    /* Wait for previous packets to be sent */
    while ((current_tx_frame = get_free_output_buffer(size)) == NULL);
    get_count++;
    current_tx_frame_idx = PROTO_MAC_HLEN; /* put the idx at the first IP byte */
}

void mbed_eth_put_byte(uint8_t byte)
{
    if (current_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to add byte %02x (%c) while prepare_output has not been successfully called\r\n", byte, byte);
	return;
    }
    if (current_tx_frame_idx >= TX_BUFFER_SIZE)
    {
	MBED_DEBUG("Trying to add byte %02x (%c) and output buffer is full\r\n", byte, byte);
	return;
    }
/*    MBED_DEBUG("O: %02x (%c)\r\n", byte, byte);*/
    current_tx_frame->buffer[current_tx_frame_idx++] = byte;
}

void mbed_eth_put_nbytes(const void *bytes, uint32_t n)
{
    if (current_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to add %d bytes while prepare_output has not been successfully called\r\n", n);
	return;
    }
    if (current_tx_frame_idx + n >= TX_BUFFER_SIZE)
    {
	MBED_DEBUG("Trying to add %d bytes and output buffer is full\r\n", n);
	return;
    }
    memcpy(current_tx_frame->buffer + current_tx_frame_idx, bytes, n);
    current_tx_frame_idx += n;
}

void mbed_eth_output_done()
{
    EthHead eth;
    rfEthDescriptor *d;
    rfEthTxStatus *s;
    uint32_t ip;
    if (current_tx_frame == NULL)
    {
	MBED_DEBUG("Trying to send packet before preparing it\r\n");
	return;
    }

    eth.src = local_eth_addr;
    ip = proto_ip_get_dst(current_tx_frame->buffer + PROTO_MAC_HLEN);
    if (!arp_get_mac(ip, &eth.dst))
    {
	MBED_DEBUG("No MAC address known for %d.%d.%d.%d, dropping\r\n", 
		   ip & 0xFF, 
		   (ip >> 8) & 0xFF,
		   (ip >> 16) & 0xFF,
		   (ip >> 24) & 0xFF
	    );
	release_output_buffer(current_tx_frame);
	current_tx_frame = NULL;
	current_tx_frame_idx = 0;
	return;
    }
    eth.type = PROTO_IP;
    proto_eth_mangle(&eth, current_tx_frame->buffer);


    if (!rflpc_eth_get_current_tx_packet_descriptor(&d, &s))
    {
	MBED_DEBUG("Failed to get current output descriptor, dropping\r\n");
	release_output_buffer(current_tx_frame);
	current_tx_frame = NULL;
	current_tx_frame_idx = 0;
	return;
    }
/* send control word (size + send options) and request interrupt */
    d->packet = current_tx_frame->buffer;
    s->status_info = PACKET_BEEING_SENT_MAGIC; /* this will allow the interrupt
						* handler to check ALL
						* descriptors for packets as
						* this status word is
						* overwriten when a packet is
						* sent. This, if status is not
						* this magic AND packet is not
						* NULL, it has to be freed */
    rflpc_eth_set_tx_control_word(current_tx_frame_idx, &d->control, 1);

    rflpc_eth_done_process_tx_packet(); /* send packet */
    current_tx_frame = NULL;
    current_tx_frame_idx = 0;
}
