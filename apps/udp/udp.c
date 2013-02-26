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
}

char udp_init(void)
{
    return 0;
}

char udp_packet_in(const void *connection_info)
{
    uint16_t tmp,size;
    struct udp_callback *n;
    /* Source port */
    tmp = (in() << 8) | in();
    /* Destination port */
    tmp = (in() << 8) | in();
    /* Find a suitable callback */
    n = _get_callback(tmp);
    if (n == NULL) /* No callbacks, drop packet */
	return 0;
    /* Packet length */
    size = (in() << 8) | in();
    /* Checksum */
    tmp = (in() << 8) | in();
    
    n->packet_in(size - 8);
    return 0;
}

char udp_packet_out(const void *connection_info)
{
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
void udp_request_send(const  void *udp_handle, unsigned char *dst_ip, uint16_t dst_port)
{
}

char udp_in()
{
    return in();
}
void udp_outc(char c)
{
}
void udp_outa(unsigned char *array, uint16_t array_size)
{
}

