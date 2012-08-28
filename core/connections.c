/*
* Copyright or © or Copr. 2008, Simon Duquennoy
*
* Author e-mail: simon.duquennoy@lifl.fr
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

#include "connections.h"
#include "memory.h"

/* Local IP address */
#ifdef IPV6

#ifndef IP_ADDR
	#define IPV6_ADDR 0x00
#endif

unsigned char local_ip_addr[16] = { IP_ADDR };
#else

#ifndef IP_ADDR
	#define IP_ADDR 4,1,168,192
#endif

unsigned char local_ip_addr[4] = { IP_ADDR };
#endif

/* Shared global connections structures */
struct connection *all_connections;
struct http_rst_connection rst_connection;

#ifdef IPV6
/*-----------------------------------------------------------------------------------*/

char ipcmp(unsigned char source_addr[],  unsigned char check_addr[]) {
	uint8_t i;

	for (i=0; i < (17 - ((source_addr[0])&15)); i++)
		if (source_addr[i] != check_addr[i])
			return 0;

	return 1;
}

unsigned char * decompress_ip(const unsigned char comp_ip_addr[], unsigned char full_ip_addr[], unsigned char indexes) {
	uint8_t i, zeros_nb, start_index;

	start_index = indexes>>4;
	zeros_nb = ((indexes)&15);

	for (i=0; i < start_index; i++)
		 full_ip_addr[i] = comp_ip_addr[i];

	for (; i < start_index+zeros_nb; i++)
		full_ip_addr[i] = 0;

	for (; i < 16; i++)
		full_ip_addr[i] = comp_ip_addr[i-zeros_nb];

	return full_ip_addr;
}

unsigned char * compress_ip(const unsigned char full_ip_addr[], unsigned char comp_ip_addr[], unsigned char * indexes) {
	int8_t i, curr_index=0, max_index=0, curr_nb=0, max_nb=0;

	for (i = 0; i < 16; i++) {
		comp_ip_addr[i] = 0;
		if (full_ip_addr[i] == 00) {
			if (!curr_nb)
				curr_index =  i;
			curr_nb++;
		} else {
			if (curr_nb > max_nb) {
				max_nb = curr_nb;
				max_index = curr_index;
				curr_nb = 0;
			}
		}
	}

	for (i = 0; i < max_index; i++)
		comp_ip_addr[i] = full_ip_addr[i];

	for (; i < (16-max_nb); i++)
		comp_ip_addr[i] = full_ip_addr[i+max_nb];

	*indexes = max_index;
	*indexes = *indexes<<4;
	*indexes = *indexes|max_nb;

	return comp_ip_addr;
}
#endif
/*-----------------------------------------------------------------------------------*/
char something_to_send(const struct connection *connection) {

#ifndef DISABLE_GP_IP_HANDLER
	if(IS_GPIP(connection))
		return connection->protocol.gpip.want_to_send;
#endif

	if(!connection->output_handler)
		return 0;


	if(CONST_UI8(connection->output_handler->handler_type) == type_control
#ifndef DISABLE_COMET
		|| connection->protocol.http.comet_send_ack == 1
#endif
		) {
		return 1;
	} else {
		return connection->protocol.http.tcp_state == tcp_established
			&& UI32(connection->protocol.http.next_outseqno) != UI32(connection->protocol.http.final_outseqno);
	}
}

/*-----------------------------------------------------------------------------------*/
struct connection *add_connection(const struct connection *from)
{
	struct connection *connection;
#ifdef IPV6
	/* Size of a connection + size of the IPv6 adress (+ compression indexes) */
	connection = mem_alloc((sizeof(struct connection) + (17-((from->ip_addr[0])&15))) * sizeof(unsigned char));
#else
	connection = mem_alloc(sizeof(struct connection)); /* test NULL: done */
#endif
	if (connection == NULL)
		return NULL;

	/* copy the connection */
	if (from != NULL)
		*connection = *from;
	/* insert the new connection */
	if(all_connections == NULL) {
		connection->next = connection;
		connection->prev = connection;
		all_connections = connection;
	} else {
		connection->prev = all_connections->prev;
		connection->prev->next = connection;
		connection->next = all_connections;
		all_connections->prev = connection;
	}
	return connection;
}

/*-----------------------------------------------------------------------------------*/
void free_connection(const struct connection *connection) {
	connection->prev->next = connection->next;
	connection->next->prev = connection->prev;
	if(connection == connection->next) {
		all_connections = NULL;
	} else {
		all_connections = connection->next;
	}

	if (IS_HTTP(connection))
	{
		if(connection->protocol.http.generator_service) {
#ifndef DISABLE_COROUTINES 
			clean_service(connection->protocol.http.generator_service, NULL);
#endif
			mem_free(connection->protocol.http.generator_service, sizeof(struct generator_service_t));
		}
	}

#ifdef IPV6
			/* Size of a connection + size of the IPv6 adress (+ compression indexes) */
	mem_free((void*)connection,(sizeof(struct connection) + (17-((connection->ip_addr[0])&15))) * sizeof(unsigned char));
#else
	mem_free((void*)connection, sizeof(struct connection));
#endif
}

#ifndef DISABLE_POST
struct coroutine_state_t coroutine_state = {
	.state = cor_out,
};
#endif


unsigned char *get_local_ip(const void *connection, unsigned char *ip)
{
	memcpy(ip, local_ip_addr, sizeof(local_ip_addr));
	return ip;
}

unsigned char *get_remote_ip(const void *connection, unsigned char *ip)
{
#ifdef IPV6
	decompress_ip(((const struct connection*)connection)->ip_addr+1, ip, ((const struct connection*)connection)->ip_addr[0]);
#else
	memcpy(ip, ((const struct connection*)connection)->ip_addr, sizeof(((const struct connection*)connection)->ip_addr));
#endif
	return ip;
}

unsigned char* get_current_remote_ip(unsigned char* ip)
{
	/* The current connection is all_connection->prev as the first thing made by smews_send is to round robin
	 * the connection by setting all_connection to connection->next (where connection is the current output connection)
	 */
	if (!all_connections)
		return NULL;
	return get_remote_ip(all_connections->prev, ip);
}


#ifndef DISABLE_GP_IP_HANDLER
uint16_t get_payload_size(const void *connection)
{
	if (!IS_GPIP(((const struct connection*)connection)))
		return 0;
	return ((const struct connection*)connection)->protocol.gpip.payload_size;
}

uint16_t get_protocol(const void *connection)
{
	if (!IS_GPIP(((const struct connection *)connection)))
		return 0;
	return ((const struct connection *)connection)->protocol.gpip.protocol;
}

char get_send_code(const void *connection)
{
	if (!IS_GPIP(((const struct connection *)connection)))
		return 0;
	return ((const struct connection*)connection)->protocol.gpip.want_to_send;
}

#endif
