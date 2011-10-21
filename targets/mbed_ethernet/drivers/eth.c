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
  Created: 2011-07-14
  Time-stamp: <2011-07-15 17:11:32 (hauspie)>

*/
#include <rflpc17xx/drivers/ethernet.h>

#include "eth.h"
#include "connections.h" /* for local_ip_addr */

#define PROTO_MAC_HLEN  14
#define PROTO_ARP_HLEN  28

#define PROTO_ARP 0x0806
#define PROTO_IP  0x0800


#define GET_TWO(dst, src, idx) dst = src[idx++] << 8; dst |= src[idx++]
#define GET_FOUR(dst, src, idx) dst = src[idx++] << 24; dst |= src[idx++] << 16; dst |= src[idx++] << 8; dst |= src[idx++]

#define GET_MAC(dst, src, idx)			\
    dst[0] = src[idx++];			\
    dst[1] = src[idx++];			\
    dst[2] = src[idx++];			\
    dst[3] = src[idx++];			\
    dst[4] = src[idx++];			\
    dst[5] = src[idx++]


#define PUT_TWO(dst, idx, src) dst[idx++] = (src >> 8) & 0xFF; dst[idx++] = src & 0xFF
#define PUT_FOUR(dst, idx, src) dst[idx++] = (src >> 24) & 0xFF; dst[idx++] = (src >> 16) & 0xFF;dst[idx++] = (src >> 8) & 0xFF; dst[idx++] = src & 0xFF

#define PUT_MAC(dst, idx, src)			\
    dst[idx++] = src[0];			\
    dst[idx++] = src[1];			\
    dst[idx++] = src[2];			\
    dst[idx++] = src[3];			\
    dst[idx++] = src[4];			\
    dst[idx++] = src[5]


#define NTOHS(v) ((((v) >> 8)&0xFF) | (((v)&0xFF)<<8))

#define MY_IP (*((uint32_t *)local_ip_addr))

EthAddr local_eth_addr = {{2, 3, 4, 5, 6, 7}};


typedef struct
{
    EthAddr src;
    EthAddr dst;
    uint16_t type;
} EthHead;

typedef struct
{
    uint16_t hard_type;
    uint16_t protocol_type;
    uint8_t hlen;
    uint8_t plen;
    uint16_t opcode;
    EthAddr sender_mac;
    uint32_t sender_ip;
    EthAddr target_mac;
    uint32_t target_ip;
} ArpHead;

void proto_eth_demangle(EthHead *eh, const uint8_t *data)
{
    int idx = 0;
    /* Dst Address */
    GET_MAC(eh->dst.addr, data, idx);
    /* Source Address */
    GET_MAC(eh->src.addr, data, idx);
    /* Type */
    GET_TWO(eh->type, data, idx);
}
void proto_eth_mangle(EthHead *eh, uint8_t *data)
{
    int idx = 0;
    /* Dst Address */
    PUT_MAC(data, idx, eh->dst.addr);
    /* Source Address */
    PUT_MAC(data, idx, eh->src.addr);
    /* Type */
    PUT_TWO(data, idx, eh->type);
}

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

static uint8_t _arp_answer_buffer[PROTO_MAC_HLEN + PROTO_ARP_HLEN];

int transmit_buffer(void *buffer, uint32_t size)
{
    rfEthDescriptor *txd;
    rfEthTxStatus *txs;
    if (!rflpc_eth_get_current_tx_packet_descriptor(&txd, &txs))
    {
	return -1;
    }
    txd->packet = buffer;
    /* Set control bits in descriptor with size, enable padding, crc, and last
     * fragment */
    txd->control = size | (1 << 18) | (1 << 29) | (1 << 30);
    rflpc_eth_done_process_tx_packet();
    return 0;
}

void process_rx_packet(rfEthDescriptor *d, rfEthRxStatus *s)
{
    /* check if packet is ARP */
    EthHead eth;

    proto_eth_demangle(&eth, d->packet);

    if (eth.type == PROTO_ARP)
    {
	ArpHead arp_rcv;
	ArpHead arp_send;
	
	proto_arp_demangle(&arp_rcv, d->packet + PROTO_MAC_HLEN);
	if (arp_rcv.target_ip != MY_IP)
	    return;
	/* generate reply */

	/* Ethernet Header */
	eth.type = eth.type;
	eth.dst = eth.src;
	eth.src = local_eth_addr;
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

	proto_eth_mangle(&eth, _arp_answer_buffer);
	proto_arp_mangle(&arp_send, _arp_answer_buffer + PROTO_MAC_HLEN);
	transmit_buffer(_arp_answer_buffer, PROTO_MAC_HLEN + PROTO_ARP_HLEN);
	/* send packet */
	rflpc_eth_done_process_rx_packet();
	return;
    }
}
