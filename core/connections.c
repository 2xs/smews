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

#define DEFAULT_IP_ADDR_0 192
#define DEFAULT_IP_ADDR_1 168
#define DEFAULT_IP_ADDR_2 1
#define DEFAULT_IP_ADDR_3 4

/* Default IP address */
#ifndef IP_ADDR_0
	#define IP_ADDR_0 DEFAULT_IP_ADDR_0
#endif
#ifndef IP_ADDR_1
	#define IP_ADDR_1 DEFAULT_IP_ADDR_1
#endif
#ifndef IP_ADDR_2
	#define IP_ADDR_2 DEFAULT_IP_ADDR_2
#endif
#ifndef IP_ADDR_3
	#define IP_ADDR_3 DEFAULT_IP_ADDR_3
#endif

/* Local IP address */
unsigned char local_ip_addr[4] = {IP_ADDR_3,IP_ADDR_2,IP_ADDR_1,IP_ADDR_0};

/* Shared global connections structures */
struct http_connection *all_connections;
struct http_rst_connection rst_connection;

/*-----------------------------------------------------------------------------------*/
char something_to_send(const struct http_connection *connection) {

	if(!connection->output_handler)
		return 0;
		

	if(CONST_UI8(connection->output_handler->handler_type) == type_control || CONST_UI8(connection->output_handler->handler_type) == type_tls_handshake
#ifndef DISABLE_COMET
		|| connection->comet_send_ack == 1
#endif
		) {

		return 1;
	} else {

		return connection->tcp_state == tcp_established
			&& UI32(connection->next_outseqno) != UI32(connection->final_outseqno);
	}
}

/*-----------------------------------------------------------------------------------*/
void free_connection(const struct http_connection *connection) {
	connection->prev->next = connection->next;
	connection->next->prev = connection->prev;
	if(connection == connection->next) {
		all_connections = NULL;
	} else {
		all_connections = connection->next;
	}
	if(connection->generator_service) {
		clean_service(connection->generator_service, NULL);
	}
	mem_free(connection->generator_service, sizeof(struct generator_service_t));
	mem_free((void*)connection, sizeof(struct http_connection));
}
