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
  Time-stamp: <2011-09-27 17:45:58 (hauspie)>
*/
#ifndef __PROTOCOLS_H__
#define __PROTOCOLS_H__
#include <stdint.h>


#define PROTO_MAC_HLEN  14
#define PROTO_ARP_HLEN  28
#define PROTO_ICMP_HLEN  8
#define PROTO_ICMP_CSUM_OFFSET 2

#define PROTO_ARP 0x0806
#define PROTO_IP  0x0800

#define PROTO_ICMP 1
#define PROTO_ICMP_ECHO_REQUEST 8
#define PROTO_ICMP_ECHO_REPLY   0


#define PROTO_IP_SRCIP_OFFSET 12
#define PROTO_IP_DSTIP_OFFSET 16


#define GET_IP(data, offset)


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

typedef struct
{
    uint8_t addr[6];
} EthAddr;



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

typedef struct
{
    uint8_t version_length;
    uint8_t dscp_ecn;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_frag_offset;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t header_checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} IpHead;

typedef struct
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union
    {
	struct
	{
	    uint16_t identifier;
	    uint16_t sn;
	} echo;
	uint32_t raw;
    } data;
}IcmpHead;

void proto_eth_demangle(EthHead *eh, const uint8_t *data);
void proto_eth_mangle(EthHead *eh, uint8_t *data);

void proto_arp_demangle(ArpHead *ah, const uint8_t *data);
void proto_arp_mangle(ArpHead *ah, uint8_t *data);

static inline uint32_t proto_ip_get_dst(const uint8_t *data)
{
    int idx = 16;
    uint32_t ip;
    GET_FOUR(ip, data, idx);
    return ip;
}
static inline uint32_t proto_ip_get_src(const uint8_t *data)
{
    int idx = 12;
    uint32_t ip;
    GET_FOUR(ip, data, idx);
    return ip;
}

static inline uint16_t proto_ip_get_size(const uint8_t *data)
{
    int idx = 2;
    uint16_t size;
    GET_TWO(size, data, idx);
    return size;
}

#if 0
void proto_ip_demangle(IpHead *ih, const uint8_t *data);
void proto_ip_mangle(IpHead *ih, uint8_t *data);
void proto_icmp_demangle(IcmpHead *ih, const uint8_t *data);
void proto_icmp_mangle(IcmpHead *ih, uint8_t *data);
#endif

/* 16 bits checksum */
uint16_t checksum(uint8_t *buffer, unsigned int bytes_count);

static inline int ethaddr_equal(EthAddr *a1, EthAddr *a2)
{
    return a1->addr[0] == a2->addr[0] &&
	a1->addr[1] == a2->addr[1] &&
	a1->addr[2] == a2->addr[2] &&
	a1->addr[3] == a2->addr[3] &&
	a1->addr[4] == a2->addr[4] &&
	a1->addr[5] == a2->addr[5];
}

#endif
