/*
* Copyright or © or Copr. 2012, Michael Hauspie
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
*/

#ifndef IPV6

#include <rflpc17xx/rflpc17xx.h>

#include "types.h"
#include "protocols.h"
#include "ethernet.h"
#include "arp.h"
#include "ip.h"
#include "link_layer_cache.h"

uint8_t _arp_reply_buffer[PROTO_MAC_HLEN + PROTO_ARP_HLEN] __attribute__ ((section(".out_ram")));;

void proto_arp_demangle(ArpHead *ah, const uint8_t *data)
{
    int idx = 0;
    /* hardware type */
    GET_TWO(ah->hard_type, data, idx);
    /* protocol type */
    GET_TWO(ah->protocol_type, data, idx);
    ah->hlen = data[idx++];
    ah->plen = data[idx++];
    /* opcode */
    GET_TWO(ah->opcode, data, idx);
    /* sender mac */
    GET_MAC(ah->sender_mac.addr, data, idx);
    /* sender IP */
    GET_FOUR(ah->sender_ip, data, idx);
    /* target mac */
    GET_MAC(ah->target_mac.addr, data, idx);
    /* target IP */
    GET_FOUR(ah->target_ip, data, idx);
}

void proto_arp_mangle(ArpHead *ah, uint8_t *data)
{
    int idx = 0;
    PUT_TWO(data, idx, ah->hard_type);
    PUT_TWO(data, idx, ah->protocol_type);
    data[idx++] = ah->hlen;
    data[idx++] = ah->plen;
    PUT_TWO(data, idx, ah->opcode);
    PUT_MAC(data, idx, ah->sender_mac.addr);
    PUT_FOUR(data, idx, ah->sender_ip);
    PUT_MAC(data, idx, ah->target_mac.addr);
    PUT_FOUR(data, idx, ah->target_ip);
}

extern unsigned char local_ip_addr[];
#define MY_IP (*((uint32_t *)local_ip_addr))

void mbed_process_arp(EthHead *eth, const uint8_t *packet, int size)
{
    ArpHead arp_rcv;
    ArpHead arp_send;
    rflpc_eth_descriptor_t *d;
    rflpc_eth_tx_status_t *s;

    proto_arp_demangle(&arp_rcv, packet + PROTO_MAC_HLEN);
    if (arp_rcv.target_ip != MY_IP)
	return;

    if (arp_rcv.opcode == 1) /* ARP_REQUEST */
    {
	/* generate reply */
	if (!rflpc_eth_get_current_tx_packet_descriptor(&d, &s,0))
	{
	    /* no more descriptor available */
	    return;
	}
	/* Ethernet Header */
	eth->dst = eth->src;
	eth->src = local_eth_addr;
	/* ARP  */
	arp_send.hard_type = 1;
	arp_send.protocol_type = PROTO_IP;
	arp_send.hlen = 6;
	arp_send.plen = 4;
	arp_send.opcode = 2; /* reply */
	arp_send.sender_mac = local_eth_addr;
	arp_send.sender_ip = arp_rcv.target_ip;
	arp_send.target_mac = arp_rcv.sender_mac;
	arp_send.target_ip = arp_rcv.sender_ip;

	proto_eth_mangle(eth, _arp_reply_buffer);
	proto_arp_mangle(&arp_send, _arp_reply_buffer + PROTO_MAC_HLEN);

	rflpc_eth_set_tx_control_word(PROTO_MAC_HLEN + PROTO_ARP_HLEN, &d->control, 0, 1);

	d->packet = _arp_reply_buffer;
	/* send packet */
	rflpc_eth_done_process_tx_packet(1);
    }
    /* record entry in arp cache */
    add_link_layer_address((unsigned char*)&arp_rcv.sender_ip, arp_rcv.sender_mac.addr );
}
#endif
