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

#ifndef __CONNECTIONS_H__
#define __CONNECTIONS_H__

#include "handlers.h"
#include "coroutines.h"
#include "tls.h"

/** TCP **/

/* TCP flags */
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

/** Connections **/

/* Connection structure */
struct http_connection {
	/* sequence number of the next segment to send */
	unsigned char next_outseqno[4];
	/* sequence number after sending all content */
	unsigned char final_outseqno[4];
	/* sequence number from the currently received packet */
	unsigned char current_inseqno[4];
	unsigned char ip_addr[4];
	const struct output_handler_t * /*CONST_VAR*/ output_handler;

	unsigned char port[2];
	unsigned char cwnd[2];
	unsigned char inflight[2];

	unsigned const char * /*CONST_VAR*/ blob;
#ifndef DISABLE_ARGS
	struct args_t *args;
	unsigned char *curr_arg;
	unsigned char arg_ref_index;
#endif
#ifndef DISABLE_TIMERS
	unsigned char transmission_time;
#endif
	uint16_t tcp_mss: 12;
	enum tcp_state_e {tcp_listen, tcp_syn_rcvd, tcp_established, tcp_closing, tcp_last_ack} tcp_state: 3;
	enum parsing_state_e {parsing_out, parsing_cmd, parsing_url, parsing_end} parsing_state: 2;
#ifndef DISABLE_COMET
	unsigned char comet_passive: 1;
	unsigned char comet_send_ack: 1;
	unsigned char comet_streaming: 1;
#endif
	struct generator_service_t *generator_service;
	
	struct http_connection *next;
	struct http_connection *prev;
      
#ifndef DISABLE_TLS
	struct tls_connection *tls;
	/* flag which says that a TLS connection has been established on this connection */
	unsigned char tls_active: 1;
#endif

};


/* Loop on each connection */
#define FOR_EACH_CONN(item, code) \
	if(all_connections) { \
		struct http_connection *(item) = all_connections; \
		do { \
			{code} \
			(item) = (item)->next; \
		} while((item) != all_connections); \
	} \

/* Pseudo connection for RST */
struct http_rst_connection {
	unsigned char ip_addr[4];
	unsigned char current_inseqno[4];
	unsigned char next_outseqno[4];
	unsigned char port[2];
};

/* Shared global connections structures */
extern struct http_connection *all_connections;
extern struct http_rst_connection rst_connection;

/* Local IP address */
extern unsigned char local_ip_addr[4];

/* Shared funuctions */
extern char something_to_send(const struct http_connection *connection);
extern void free_connection(const struct http_connection *connection);

#endif /* __CONNECTIONS_H__ */
