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
  Time-stamp: <2011-09-02 10:51:03 (hauspie)>
*/
#include "protocols.h"



uint16_t checksum(uint8_t *buffer, unsigned int bytes_count)
{
    uint32_t csum = 0;
    while (bytes_count >= 2)
    {
	csum += NTOHS(*((uint16_t*)buffer));
	buffer += 2;
	bytes_count -= 2;
    }
    if (bytes_count)
	csum += (*buffer << 8);
    /* add carry */
    while (csum & 0xFFFF0000)
	csum = (csum & 0xFFFF) + ((csum >> 16) & 0xFFFF);
    return (~csum) & 0xFFFF;
}

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


#if 0
void proto_ip_demangle(IpHead *ih, const uint8_t *data)
{
    int idx = 0;
    ih->version_length = data[idx++];
    ih->dscp_ecn = data[idx++];
    GET_TWO(ih->total_length, data, idx);
    GET_TWO(ih->identification, data, idx);
    GET_TWO(ih->flags_frag_offset,data,idx);
    ih->ttl = data[idx++];
    ih->protocol = data[idx++];
    GET_TWO(ih->header_checksum, data, idx);
    GET_FOUR(ih->src_addr, data, idx);
    GET_FOUR(ih->dst_addr, data, idx);
}
void proto_ip_mangle(IpHead *ih, uint8_t *data)
{
    int idx = 0;
    data[idx++] = ih->version_length;
    data[idx++] = ih->dscp_ecn;
    PUT_TWO(data, idx, ih->total_length);
    PUT_TWO(data, idx, ih->identification);
    PUT_TWO(data, idx,ih->flags_frag_offset);
    data[idx++] = ih->ttl;
    data[idx++] = ih->protocol;
    PUT_TWO(data, idx, ih->header_checksum);
    PUT_FOUR(data, idx, ih->src_addr);
    PUT_FOUR(data, idx, ih->dst_addr);
    /* set checksum */
    if (ih->header_checksum  == 0) /* need to compute checksum */
    {
	ih->header_checksum = checksum(data, (ih->version_length & 0xF)<<2);
	idx = 10;
	PUT_TWO(data, idx, ih->header_checksum);
    }
}

void proto_icmp_demangle(IcmpHead *ih, const uint8_t *data)
{
    int idx = 0;
    ih->type = data[idx++];
    ih->code = data[idx++];
    GET_TWO(ih->checksum, data, idx);
    if (ih->type == PROTO_ICMP_ECHO_REQUEST || ih->type == PROTO_ICMP_ECHO_REPLY)
    {
	GET_TWO(ih->data.echo.identifier, data, idx);
	GET_TWO(ih->data.echo.sn, data, idx);
	return;
    }
    GET_FOUR(ih->data.raw, data, idx);

}
void proto_icmp_mangle(IcmpHead *ih, uint8_t *data)
{
    int idx = 0;
    data[idx++] = ih->type;
    data[idx++] = ih->code;
    PUT_TWO(data, idx, ih->checksum);
    if (ih->type == PROTO_ICMP_ECHO_REQUEST || ih->type == PROTO_ICMP_ECHO_REPLY)
    {
	PUT_TWO(data, idx, ih->data.echo.identifier);
	PUT_TWO(data, idx, ih->data.echo.sn);
	return;
    }
    PUT_FOUR(data, idx, ih->data.raw);
}
#endif
