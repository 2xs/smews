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
/*
<generator>
        <handlers init="udp_init" doPacketIn="udp_packet_in" doPacketOut="udp_packet_out"/>
        <properties protocol="17" />
</generator>
 */
#include "udp.h"

struct udp_callback
{
    uint16_t src_port;
    udp_packet_in_t packet_in;
    udp_packet_out_t packet_out;
    struct udp_args_t args; /* the arguments used for this binding */

    struct udp_callback *next;
    struct udp_callback *prev;
};

static struct udp_callback *_head = NULL;

static struct udp_callback *_new_callback(void)
{
    struct udp_callback *n = mem_alloc(sizeof(struct udp_callback));
    n->next = n->prev = NULL;
    if (n == NULL)
	return NULL;
    if (_head == NULL)
	n->prev = n->next = _head = n;
    else
    {
	_head->prev->next = n;
	n->prev = _head->prev;
	_head->prev = n;
	n->next = _head;
    }
    n->args.udp_handle = n;
    return n;
}

static struct udp_callback *_get_callback(uint16_t src_port)
{
    struct udp_callback *n = _head;
    if (_head == NULL)
	return NULL;

    do{
	if (n->src_port == src_port)
	    return n;
	n = n->next;
    } while (n != _head);
    return NULL;
}

static struct udp_callback *_get_callback_from_connection(const void *connection)
{
    struct udp_callback *n = _head;
    if (_head == NULL)
	return NULL;
    
    do{
	if (n->args.connection_info == connection)
	    return n;
	n = n->next;
    } while (n != _head);
    return NULL;
}

char udp_init(void)
{
    return 0;
}

char udp_packet_in(const void *connection_info)
{
    struct udp_callback *n;
    uint16_t sp, dp;
    /* Source port */
    sp = (in() << 8) | in();
    /* Destination port */
    dp = (in() << 8) | in();
    /* Find a suitable callback */
    n = _get_callback(dp);
    if (n == NULL) /* No callbacks, drop packet */
	return 0;
    n->args.src_port = sp;
    n->args.dst_port = dp;
    /* Packet length */
    n->args.payload_size = ((in() << 8) | in()) - 8;
    /* Checksum */
    n->args.chk = (in() << 8) | in();
    n->args.connection_info = connection_info;
    return n->packet_in(&(n->args));
}

static uint8_t _udp_output_buffer[OUTPUT_BUFFER_SIZE];
static uint16_t _current_payload_size;

char udp_packet_out(const void *connection_info)
{
    struct udp_callback *n = _get_callback_from_connection(connection_info);
    uint16_t i;
    if (n == NULL)
	return 0;
    /* Source port */
    out_c(n->args.src_port >> 8);
    out_c(n->args.src_port & 0xff);
    /* Destination port */
    out_c(n->args.dst_port >> 8);
    out_c(n->args.dst_port & 0xff);
    _current_payload_size = 0;
    n->packet_out(&(n->args));
    /* Length */
    out_c((_current_payload_size+8) >> 8);
    out_c((_current_payload_size+8) & 0xff);
    /* Checksum... TODO */
    out_c(0);
    out_c(0);
    /* Payload */
    for (i = 0 ; i < _current_payload_size ; ++i)
	out_c(_udp_output_buffer[i]);
    return 0;
}


void *udp_listen(uint16_t port, udp_packet_in_t packet_in_callback, udp_packet_out_t packet_out_callback)
{
    struct udp_callback *n;
    n = _get_callback(port);
    if (n != NULL) /* Already has a callback registered */
	return NULL;
    n = _new_callback();
    if (n == NULL)
	return NULL; /* No more memory */
    n->src_port = port;
    n->packet_in = packet_in_callback;
    n->packet_out = packet_out_callback;
    return n;
}
void udp_request_send(unsigned char *dst_ip, uint16_t dst_port, uint16_t src_port)
{
    struct udp_callback *n = _get_callback(src_port);
    if (n == NULL)
	return;
    n->args.connection_info = request_packet_out_call(17, dst_ip);
    n->args.src_port = src_port;
    n->args.dst_port = dst_port;
}

char udp_in()
{
    return in();
}
void udp_outc(char c)
{
    if (_current_payload_size >= OUTPUT_BUFFER_SIZE)
	return;
    _udp_output_buffer[_current_payload_size++] = c;
}
void udp_outa(const void *array, uint16_t array_size)
{
    if (_current_payload_size + array_size >= OUTPUT_BUFFER_SIZE)
	return;
    memcpy(_udp_output_buffer+_current_payload_size, array, array_size);
    _current_payload_size += array_size;
}

