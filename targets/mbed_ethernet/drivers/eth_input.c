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
  Time-stamp: <2015-03-31 16:03:37 (hauspie)>
*/
#include <rflpc17xx/rflpc17xx.h>


#include "connections.h"
#include "eth_input.h"
#include "mbed_debug.h"
#include "ip.h"
#include "ethernet.h"
#include "hardware.h"
#include "link_layer_cache.h"

#ifndef IPV6
#include "arp.h"
#endif


/* These are information on the current frame read by smews */
const uint8_t * volatile current_rx_frame = NULL;
volatile uint32_t current_rx_frame_size = 0;
volatile uint32_t current_rx_frame_idx = 0;

volatile static int byte_count = 0;

int16_t mbed_eth_get_byte()
{
    uint8_t byte;
    if (current_rx_frame == NULL)
    {
	MBED_DEBUG("SMEWS Required a byte but none available!\r\n");
	return -1;
    }

    byte = current_rx_frame[current_rx_frame_idx++];
    ++byte_count;
    if (current_rx_frame_idx >= current_rx_frame_size)
    {
	current_rx_frame = NULL;
	current_rx_frame_size = 0;
	current_rx_frame_idx = 0;
	rflpc_eth_done_process_rx_packet();
	byte_count = 0;
	if (rflpc_eth_rx_available()) /* If packet have been received but not yet handled, force IRQ generation */
	    rflpc_eth_irq_trigger(RFLPC_ETH_IRQ_EN_RX_DONE);
    }
    return byte;
}

int mbed_eth_byte_available()
{
    if (current_rx_frame == NULL && rflpc_eth_rx_available()) /* No frame but something has been received */
	rflpc_eth_irq_trigger(RFLPC_ETH_IRQ_EN_RX_DONE);
    return current_rx_frame != NULL;
}

int mbed_process_input(const uint8_t *packet, int size)
{
    EthHead eth;
#ifdef IPV6
    unsigned char src_ip[16];
#else
    unsigned char src_ip[4];
#endif

    MBED_ASSERT(packet);
    if (packet == current_rx_frame)
        return ETH_INPUT_KEEP_PACKET; /* already processing this packet */

    proto_eth_demangle(&eth, packet);

#ifndef IPV6
    if (eth.type == PROTO_ARP)
    {
	mbed_process_arp(&eth, packet, size);
	return ETH_INPUT_FREE_PACKET;
    }
#endif

    if (eth.type != PROTO_IP)
	return ETH_INPUT_FREE_PACKET; /* drop packet */


    if (!proto_eth_addr_is_multicast(&eth.dst) && !proto_eth_addr_equal(&eth.dst, &local_eth_addr)) /* not for me */
	return ETH_INPUT_FREE_PACKET;

    /* IP Packet received */
    proto_ip_get_src(packet + PROTO_MAC_HLEN, src_ip);
    add_link_layer_address(src_ip, eth.src.addr);

    current_rx_frame = packet;
    current_rx_frame_size = proto_ip_get_size(packet + PROTO_MAC_HLEN) + PROTO_MAC_HLEN;
    if (current_rx_frame_size >= DEV_MTU) /* prevent faulty IP packets */
       current_rx_frame_size = DEV_MTU;
    current_rx_frame_idx = PROTO_MAC_HLEN; /* idx points to the first IP byte */
    return ETH_INPUT_KEEP_PACKET; /* do not get the packet back to the driver,
				   * keep it while smews process it. The packet
				   * will be "freed" when smews gets its last
				   * byte */
}
