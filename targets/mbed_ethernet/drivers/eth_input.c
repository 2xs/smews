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
  Time-stamp: <2011-08-31 17:41:42 (hauspie)>
*/
#include <rflpc17xx/printf.h>

#include "connections.h"

#include "eth_input.h"
#include "mbed_debug.h"
#include "protocols.h"
#include "hardware.h"
#include "arp_cache.h"


void mbed_process_arp(EthHead *eth, const uint8_t *packet, int size)
{
    ArpHead arp_rcv;
    ArpHead arp_send;
    rfEthDescriptor *d;
    rfEthTxStatus *s;
	
    proto_arp_demangle(&arp_rcv, packet + PROTO_MAC_HLEN);
    if (arp_rcv.target_ip != MY_IP)
	return;

    if (arp_rcv.opcode == 1) /* ARP_REQUEST */
    {
	/* generate reply */
	if (!rflpc_eth_get_current_tx_packet_descriptor(&d, &s))
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
    
	proto_eth_mangle(eth, d->packet);
	proto_arp_mangle(&arp_send, d->packet + PROTO_MAC_HLEN);
	rflpc_eth_set_tx_control_word(PROTO_MAC_HLEN + PROTO_ARP_HLEN, &d->control);
	/* send packet */
	rflpc_eth_done_process_tx_packet();
    }
    /* record entry in arp cache */
    arp_add_mac(arp_rcv.sender_ip, &arp_rcv.sender_mac);
}

int mbed_process_input(const uint8_t *packet, int size)
{
    EthHead eth;

    proto_eth_demangle(&eth, packet);

    /* printf("From\t%02x:%02x:%02x:%02x:%02x:%02x\r\n",  */
    /* 	   eth.src.addr[0], */
    /* 	   eth.src.addr[1], */
    /* 	   eth.src.addr[2], */
    /* 	   eth.src.addr[3], */
    /* 	   eth.src.addr[4], */
    /* 	   eth.src.addr[5]); */

    /* printf("To\t%02x:%02x:%02x:%02x:%02x:%02x\r\n",  */
    /* 	   eth.dst.addr[0], */
    /* 	   eth.dst.addr[1], */
    /* 	   eth.dst.addr[2], */
    /* 	   eth.dst.addr[3], */
    /* 	   eth.dst.addr[4], */
    /* 	   eth.dst.addr[5]); */
    
/* /\*    printf("Type\t%x\r\n", eth.type);*\/ */

    if (eth.type == PROTO_ARP)
    {
	mbed_process_arp(&eth, packet, size);
	return ETH_INPUT_FREE_PACKET;
    }
    if (eth.type != PROTO_IP)
	return ETH_INPUT_FREE_PACKET; /* drop packet */
    return ETH_INPUT_FREE_PACKET;
}
