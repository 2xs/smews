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
  Time-stamp: <2011-09-29 10:30:30 (hauspie)>
*/


#ifdef MBED_DEBUG_MODE
#include <rflpc17xx/drivers/ethernet.h>
#include <rflpc17xx/debug.h>
#include <rflpc17xx/printf.h>

#include "mbed_debug.h"
#include "protocols.h"

void mbed_dump_packet(rflpc_eth_descriptor_t *d, rflpc_eth_rx_status_t *s, int dump_contents)
{
    EthHead eth;

    proto_eth_demangle(&eth, d->packet);


    printf("= %02x:%02x:%02x:%02x:%02x:%02x -> ",
	   eth.src.addr[0],
	   eth.src.addr[1],
	   eth.src.addr[2],
	   eth.src.addr[3],
	   eth.src.addr[4],
	   eth.src.addr[5]);

    printf("%02x:%02x:%02x:%02x:%02x:%02x : ",
	   eth.dst.addr[0],
	   eth.dst.addr[1],
	   eth.dst.addr[2],
	   eth.dst.addr[3],
	   eth.dst.addr[4],
	   eth.dst.addr[5]);

    if (eth.type == PROTO_IP)
    {
	uint32_t src = proto_ip_get_src(d->packet + PROTO_MAC_HLEN);
	uint32_t dst = proto_ip_get_dst(d->packet + PROTO_MAC_HLEN);;
	printf("IP %d.%d.%d.%d -> ",
	       (src >> 24) & 0xFF,
	       (src >> 16) & 0xFF,
	       (src >> 8) & 0xFF,
	       (src) & 0xFF);
	printf("%d.%d.%d.%d ",
	       (dst >> 24) & 0xFF,
	       (dst >> 16) & 0xFF,
	       (dst >> 8) & 0xFF,
	       (dst) & 0xFF);
	printf("%d bytes ", proto_ip_get_size(d->packet + PROTO_MAC_HLEN));
    }

    if (s->status_info & (1 << 18))
	printf("cf ");
    else
	printf("   ");
    if (s->status_info & (1 << 21))
	printf("mf ");
    else
	printf("   ");
    if (s->status_info & (1 << 22))
	printf("bf ");
    else
	printf("   ");
    if (s->status_info & (1 << 23))
	printf("crc ");
    else
	printf("    ");
    if (s->status_info & (1 << 24))
	printf("se ");
    else
	printf("   ");
    if (s->status_info & (1 << 25))
	printf("le ");
    else
	printf("   ");
    if (s->status_info & (1 << 27))
	printf("ae ");
    else
	printf("   ");
    if (s->status_info & (1 << 28))
	printf("ov ");
    else
	printf("   ");
    if (s->status_info & (1 << 29))
	printf("nd ");
    else
	printf("   ");
    if (s->status_info & (1 << 30))
	printf("lf ");
    else
	printf("   ");

    printf("size:%d \r\n", rflpc_eth_get_packet_size(s->status_info));
    if (dump_contents)
	MBED_DUMP_BYTES(d->packet, rflpc_eth_get_packet_size(s->status_info));
}
#endif
