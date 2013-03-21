/*
* Copyright or © or Copr. 2013, Michael Hauspie
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
#ifndef __UDP_H__
#define __UDP_H__

struct udp_args_t
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t payload_size;
    uint16_t chk;
    const void *connection_info;
    const void *udp_handle;
};


typedef char (*udp_packet_in_t)(struct udp_args_t *);
typedef char (*udp_packet_out_t)(struct udp_args_t *);

void *udp_listen(uint16_t port, udp_packet_in_t packet_in_callback, udp_packet_out_t packet_out_callback);
void udp_request_send(unsigned char *dst_ip, uint16_t dst_port, uint16_t source_port);

char udp_in();
void udp_outc(char c);
void udp_outa(const void *array, uint16_t array_size);

#endif
