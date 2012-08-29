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

#include "input.h"
#include "output.h"
#include "checksum.h"
#include "connections.h"
#include "timers.h"
#include "memory.h"
#include "coroutines.h"
#include "blobs.h"
#include "defines.h"

/* Used to dump the runtime stack */
#ifdef STACK_DUMP
	int16_t stack_i;
	unsigned char *stack_base;
#endif

/* "404 Not found" handler */
#define http_404_handler apps_httpCodes_404_html_handler
extern CONST_VAR(struct output_handler_t, apps_httpCodes_404_html_handler);

/* Maximal TCP MSS */
#ifndef DEV_MTU
	#define MAX_MSS 0xffff
#else
	#define MAX_MSS (DEV_MTU - (IP_HEADER_SIZE+ TCP_HEADER_SIZE))
#endif

/* IP and TCP constants */
#define HTTP_PORT 80
#define IP_PROTO_TCP 6

/* TCP pre-calculated partial pseudo-header checksum (for incoming packets)*/
#define TCP_PRECALC_CHECKSUM ((uint16_t)(0x0000ffff - (IP_PROTO_TCP - 0x15)))

/* tcp constant checksum part: ip_proto_tcp from pseudoheader*/
#define TCP_CHK_CONSTANT_PART (uint16_t)~IP_PROTO_TCP

/* Initial sequence number */
#define BASIC_SEQNO 0x42b7a491

#ifndef DISABLE_POST
	#define URL_POST_END 255
	#define HEADER_POST_END 128
	static CONST_VAR(unsigned char, blob_http_header_end[]) = {13,10,13,128};
	/* "505 HTTP Version Not Supported" handler */
	#define http_505_handler apps_httpCodes_505_html_handler
	extern CONST_VAR(struct output_handler_t, apps_httpCodes_505_html_handler);
#endif

/* gets 16 bits and checks if nothing wrong appened */
static char dev_get16(unsigned char *word) {
	int16_t tmp;
	DEV_GET(tmp);
	if(tmp == -1)
		return -1;
	word[1] = tmp;
	DEV_GET(tmp);
	if(tmp == -1)
		return -1;
	word[0] = tmp;
	return 1;
}

/* gets 16 bits and checks if nothing wrong appened */
static char dev_get32(unsigned char *dword) {
	if(dev_get16(&dword[2]) == -1)
		return -1;
	if(dev_get16(&dword[0]) == -1)
		return -1;
	return 1;
}

#ifdef IPV6
/* gets 128 bits and checks if nothing wrong appened */
static char dev_get128(unsigned char *dword) {
	if(dev_get32(&dword[12]) == -1)
		return -1;
	if(dev_get32(&dword[8]) == -1)
		return -1;
	if(dev_get32(&dword[4]) == -1)
		return -1;
	if(dev_get32(&dword[0]) == -1)
		return -1;
	return 1;
}

/* Get 2 bytes */
#define DEV_GET16(c) { \
	if(dev_get16((unsigned char*)(c)) == -1) \
		return 1; \
}
/* Get 4 bytes */
#define DEV_GET32(c) { \
	if(dev_get32((unsigned char*)(c)) == -1) \
		return 1; \
}

/* Get 16 bytes */
#define DEV_GET128(c) { \
	if(dev_get128((unsigned char*)(c)) == -1) \
		return 1; \
}
#endif

/* Get and checksum a byte */
#define DEV_GETC(c) { int16_t getc; \
		DEV_GET(getc); \
		if(getc == -1) return 1; \
		c = getc; \
		checksum_add(c);} \

/* Get and checksum 2 bytes */
#define DEV_GETC16(c) { \
	if(dev_get16((unsigned char*)(c)) == -1) \
		return 1; \
	checksum_add16(UI16(c)); \
}

/* Get and checksum 4 bytes */
#define DEV_GETC32(c) { \
	if(dev_get32((unsigned char*)(c)) == -1) \
		return 1; \
	checksum_add32(c); \
}

#ifdef IPV6
/* Get and checksum 16 bytes */
#define DEV_GETC128(c) { \
	if(dev_get128((unsigned char*)(c)) == -1) \
		return 1; \
	checksum_add32(&c[12]); \
	checksum_add32(&c[8]); \
	checksum_add32(&c[4]); \
	checksum_add32(&c[0]); \
}

#endif

#ifndef DISABLE_POST
struct curr_input_t {
	struct connection *connection;
	uint16_t length;
} curr_input;
#endif

#if defined(DISABLE_POST) && !defined(DISABLE_GP_IP_HANDLER)
	/* If support for post is not included, the in() function
	 * for general purpose ip handler is very simple */
	short in()
	{
		unsigned char tmp;
		DEV_GET(tmp);
		return tmp;
	}
#endif

#ifndef DISABLE_POST
/* called from dopostin or dopacketin */
short in(){
	unsigned char tmp;

#ifndef DISABLE_GP_IP_HANDLER
	/* If post AND general purpose ip, then check if the connection is null
	 * if so, it is called from packetin */
	if (curr_input.connection == NULL)
	{
		DEV_GET(tmp);
		return tmp;
	}
#endif

	if(coroutine_state.state == cor_out)
		return -1;
	if(!curr_input.connection->protocol.http.post_data->content_length)
		return -1;
	if(curr_input.connection->protocol.http.post_data->boundary && (curr_input.connection->protocol.http.post_data->boundary->index == (uint8_t)-1)) /* index = -1 => boundary found */
		return -1;
	if(!curr_input.length)
		cr_run(NULL,cor_type_get); /* input buffer is empty, changing context */
	if(curr_input.connection->protocol.http.post_data->boundary){
		/* getting next value in input buffer */
		tmp = curr_input.connection->protocol.http.post_data->boundary->boundary_buffer[curr_input.connection->protocol.http.post_data->boundary->index];
		DEV_GETC(curr_input.connection->protocol.http.post_data->boundary->boundary_buffer[curr_input.connection->protocol.http.post_data->boundary->index]);
		curr_input.connection->protocol.http.post_data->boundary->index++;
		/* searching boundary */
		if(curr_input.connection->protocol.http.post_data->boundary->index == curr_input.connection->protocol.http.post_data->boundary->boundary_size)
			curr_input.connection->protocol.http.post_data->boundary->index = 0;
		/* comparing first and last characters of buffer with boundary, then all characters */
		if(curr_input.connection->protocol.http.post_data->boundary->boundary_buffer[(unsigned char)(curr_input.connection->protocol.http.post_data->boundary->index - 1) % curr_input.connection->protocol.http.post_data->boundary->boundary_size]
				== curr_input.connection->protocol.http.post_data->boundary->boundary_ref[curr_input.connection->protocol.http.post_data->boundary->boundary_size-1]
				&& curr_input.connection->protocol.http.post_data->boundary->boundary_buffer[curr_input.connection->protocol.http.post_data->boundary->index]
				== curr_input.connection->protocol.http.post_data->boundary->boundary_ref[0]){
			unsigned char index = curr_input.connection->protocol.http.post_data->boundary->index + 1;
			unsigned char i;
			for(i = 1 ; i < curr_input.connection->protocol.http.post_data->boundary->boundary_size-1 ; i++){
				if(index == curr_input.connection->protocol.http.post_data->boundary->boundary_size)
					index = 0;
				if(curr_input.connection->protocol.http.post_data->boundary->boundary_ref[i] != curr_input.connection->protocol.http.post_data->boundary->boundary_buffer[index])
					break;
				index++;
			}
			if(i == curr_input.connection->protocol.http.post_data->boundary->boundary_size-1)
				curr_input.connection->protocol.http.post_data->boundary->index = -1;
		}
	}
	else
		DEV_GETC(tmp);
	/* updating counters */
	if(curr_input.connection->protocol.http.post_data->content_length != (uint16_t)-1)
		curr_input.connection->protocol.http.post_data->content_length--;
	curr_input.length--;
	return tmp;
}
#endif

#ifndef DISABLE_GP_IP_HANDLER
static const struct output_handler_t *smews_gpip_get_output_handler(uint8_t protocol)
{
	/* Try to find a handler that match the protocol number */
	uint8_t i;
	for (i = 0 ; resources_index[i] != NULL ; ++i)
	{
		if (!IS_GPIP_HANDLER(resources_index[i]))
			continue;
		if (resources_index[i]->handler_data.generator.handlers.gp_ip.protocol == protocol) /* found one */
			return resources_index[i];
	}
	return NULL;
}

/*static struct connection *smews_gpip_get_connection(uint8_t protocol, unsigned char *source_ip)
{
	FOR_EACH_CONN(conn, {
			if (IS_GPIP(conn))
			{
				if (conn->protocol.gpip.protocol == protocol)
				{
					if (!source_ip || IP_CMP(source_ip, conn->ip_addr))
						return conn;
				}
			}
	})
	return NULL;
}*/
#endif

/*-----------------------------------------------------------------------------------*/
char smews_receive(void) {
	/* local variables */
	unsigned char current_inseqno[4];
	unsigned char current_inack[4];
	unsigned char tmp_ui32[4];
	unsigned char tmp_ui16[2];
	uint16_t packet_length;
	unsigned char tcp_header_length;
	uint16_t segment_length;
	struct connection *connection;
	unsigned char tmp_char;
	uint16_t x;
	unsigned char new_tcp_data;
#ifndef DISABLE_GP_IP_HANDLER
	uint8_t protocol;
#endif
#ifdef IPV6
	/* Full and compressed IPv6 adress of the received packet */
	unsigned char full_ipv6_addr[16];
	unsigned char comp_ipv6_addr[17]; /* 16 bytes (addr) + 1 byte (compression indexes) = 17 bytes */
#endif

	/* variables used to defer processing with side effects until the incomming packet has been checked (with checksum) */
	unsigned char defer_clean_service = 0;
	unsigned char defer_free_handler = 0;
#ifndef DISABLE_ARGS
	struct args_t *defer_free_args;
	uint16_t defer_free_args_size;
#endif

	/* tmp connection used to store the current state until checksums are checked */
	struct connection tmp_connection;

	if(!DEV_DATA_TO_READ)
		return 0;

#ifdef IPV6
	/* Get IP Version (and 4 bits of the traffic class) */
	DEV_GET(tmp_char);
#else
	checksum_init();

	/* get IP version & header length */
	DEV_GETC(tmp_char);
#endif


#ifdef SMEWS_RECEIVING
	SMEWS_RECEIVING;
#endif
	/* Starting to decode IP */
#ifdef IPV6
	/* 0x06 : IPV6, 4 bits right because we don't care (yet?)
		about the Traffic Class */
	if(tmp_char>>4 != 0x6)
#else
	/* 0x45 : IPv4, no option in header */
	if(tmp_char != 0x45)
#endif
		return 1;

#ifdef IPV6
	/* Discard the rest of the traffic class and the Flow Label (still unused) */
	DEV_GET(tmp_char);
	DEV_GET16(tmp_ui16);

	/* Get IP packet payload length in len */
	DEV_GET16(((uint16_t *)&packet_length));

	DEV_GET(tmp_char);
	/* What's the next header? */
#ifndef DISABLE_GP_IP_HANDLER
	protocol = tmp_char;
#else
	if (tmp_char != IP_PROTO_TCP) /* If not TCP and general purpose IP is disabled, drop packet */
		return 1;
#endif


	/* We don't care about the Hop Limit (TTL) */
	DEV_GET(tmp_char);

	/* get & store IP source address */
	DEV_GET128(&full_ipv6_addr[0]);

	/* Compress the received IPv6 adress
		compress_ip(FullIPv6, IP's Offset, Indexe's offset) */
	compress_ip(full_ipv6_addr, comp_ipv6_addr+1, &comp_ipv6_addr[0]);
	/* copy compressed ip */
	copy_compressed_ip(tmp_connection.ip_addr, comp_ipv6_addr);
	/* discard the dest IP */
	DEV_GET32(tmp_ui32);
	DEV_GET32(tmp_ui32);
	DEV_GET32(tmp_ui32);
	DEV_GET32(tmp_ui32);
#else
	/* discard IP type of service */
	DEV_GETC(tmp_char);

	/* get IP packet length in len */
	DEV_GETC16(((uint16_t *)&packet_length));
	/* The packet_length is the size of the IP payload, without the IP header size.
	 * This make the value homegeneous for IPv4 and IPv6 and simplifies code for TCP segment length calculation (no more #ifndef hacks) */
	packet_length = packet_length - IP_HEADER_SIZE;

	/* discard IP ID */
	DEV_GETC16(tmp_ui16);

	/* get IP fragmentation flags (fragmentation is not supported) */
	DEV_GETC16(tmp_ui16);
	if((tmp_ui16[S0] & 0x20) || (tmp_ui16[S0] & 0x1f) != 0)
		return 1;

	/* get IP fragmentation offset */
	if(tmp_ui16[S1] != 0)
		return 1;

	/* discard IP TTL */
	DEV_GETC16(tmp_ui16);

#ifndef DISABLE_GP_IP_HANDLER
	protocol = tmp_ui16[S1];
#else
	if(tmp_ui16[S1] != IP_PROTO_TCP) /* If not TCP and general purpose IP is disabled, drop packet */
		return 1;
#endif

	/* discard IP checksum */
	DEV_GETC16(tmp_ui16);

	/* get & store IP source address */
	DEV_GETC32(tmp_connection.ip_addr);

	/* discard the IP destination address 2*16*/
	DEV_GETC32(tmp_ui32);

	/* check IP checksum */
	checksum_end();

	if(UI16(current_checksum) != 0xffff)
		return 1;
#endif

#ifndef DISABLE_GP_IP_HANDLER
	if (protocol != IP_PROTO_TCP)
	{
		/* Finaly, the decision has been made to create a new connection for each packet and to
		 * free it after having called the output handler.
		 * Previous code left in comments if we change our mind later */
		connection = NULL; /* smews_gpip_get_connection(protocol, NULL);*/

		if (connection == NULL) /* no connection found, create one */
		{
			tmp_connection.output_handler = smews_gpip_get_output_handler(protocol);
			if (tmp_connection.output_handler == NULL)
				return 1;
			tmp_connection.protocol.gpip.protocol = protocol;
			/* add the connection */
			connection = add_connection(&tmp_connection);
			if (connection == NULL)
				return 1;
		}
		/* update the connection */
#ifdef IPV6
			copy_compressed_ip(connection->ip_addr, comp_ipv6_addr);
#else
			memcpy(connection->ip_addr, tmp_connection.ip_addr, sizeof(tmp_connection.ip_addr));
#endif

#ifndef DISABLE_POST /* if post is available, must set the curr_input connection to null
						so that the in function knows that we are in dopacketin */
			curr_input.connection = NULL;
#endif
		connection->protocol.gpip.payload_size = packet_length;
		connection->protocol.gpip.want_to_send = connection->output_handler->handler_data.generator.handlers.gp_ip.dopacketin(connection);
		if (!connection->protocol.gpip.want_to_send)
			free_connection(connection);
		return 1;
	}
#endif
	/* End of IP, starting TCP */
	checksum_init();

	/* get TCP source port */
	DEV_GETC16(tmp_connection.protocol.http.port);

	/* current connection selection */
	connection = NULL;
	/* search an existing TCP connection using the current port */

	FOR_EACH_CONN(conn, {
		/* Only search http connection */
		if (!IS_HTTP(conn))
		{
			NEXT_CONN(conn);
			continue;
		}
		if(UI16(conn->protocol.http.port) == UI16(tmp_connection.protocol.http.port) &&
#ifdef IPV6
			ipcmp(conn->ip_addr, comp_ipv6_addr)) {
#else
			UI32(conn->ip_addr) == UI32(tmp_connection.ip_addr)) {
#endif
				/* connection already existing */
				connection = conn;
			break;
		}
	})

	if(connection) {
		/* a connection has been found */
		tmp_connection = *connection;
	} else {
		tmp_connection.protocol.http.tcp_state = tcp_listen;
		tmp_connection.output_handler = NULL;
		UI32(tmp_connection.protocol.http.next_outseqno) = BASIC_SEQNO;
		UI32(tmp_connection.protocol.http.current_inseqno) = 0;
		tmp_connection.protocol.http.parsing_state = parsing_out;
		UI16(tmp_connection.protocol.http.inflight) = 0;
		tmp_connection.protocol.http.generator_service = NULL;
		tmp_connection.protocol.http.ready_to_send = 1;
#ifndef DISABLE_COMET
		tmp_connection.protocol.http.comet_send_ack = 0;
		tmp_connection.protocol.http.comet_passive = 0;
#endif
#ifndef DISABLE_TIMERS
		tmp_connection.protocol.http.transmission_time = last_transmission_time;
#endif
#ifndef DISABLE_POST
		tmp_connection.protocol.http.post_data = NULL;
		tmp_connection.protocol.http.post_url_detected = 0;
#endif
	}

	/* get and check the destination port */

	DEV_GETC16(tmp_ui16);
	if(tmp_ui16[S1] != HTTP_PORT) {
#ifdef STACK_DUMP
		DEV_PREPARE_OUTPUT(STACK_DUMP_SIZE);
		for(stack_i = 0; stack_i < STACK_DUMP_SIZE ; stack_i++) {
			DEV_PUT(stack_base[-stack_i]);
		}
		DEV_OUTPUT_DONE;
#endif
		return 1;
	}

	/* get TCP sequence number */
	DEV_GETC32(current_inseqno);

	/* get TCP ack */
	DEV_GETC32(current_inack);

	/* duplicate ACK: set nextoutseqno for retransmission */
	if(UI32(tmp_connection.protocol.http.next_outseqno) - UI16(tmp_connection.protocol.http.inflight) == UI32(current_inack)) {
		UI32(tmp_connection.protocol.http.next_outseqno) = UI32(current_inack);
	}

	/* TCP ack management */
	if(UI32(current_inack) && UI32(current_inack) <= UI32(tmp_connection.protocol.http.next_outseqno)) {
		UI16(tmp_connection.protocol.http.inflight) = UI32(tmp_connection.protocol.http.next_outseqno) - UI32(current_inack);
#ifdef DISABLE_COROUTINES
        if(curr_output.service != NULL)
            curr_output.has_received_dyn_ack = 1;
        printf("%d,%X\n", current_inack, curr_output.service);
#endif
		if(tmp_connection.protocol.http.generator_service) {
			/* deferred because current segment has not yet been checked */
			defer_clean_service = 1;
		}
	}

	/* clear output_handler if needed */
	if(tmp_connection.output_handler && UI16(tmp_connection.protocol.http.inflight) == 0 && !something_to_send(&tmp_connection)) {
		if(tmp_connection.protocol.http.generator_service) {
			/* deferred because current segment has not yet been checked */
			defer_free_handler = 1;
#ifndef DISABLE_ARGS
			defer_free_args = tmp_connection.protocol.http.args;
			defer_free_args_size = CONST_UI16(tmp_connection.output_handler->handler_args.args_size);
#endif
		}
#ifndef DISABLE_COMET
		if(/*tmp_connection.protocol.http.parsing_state == parsing_out &&*/ tmp_connection.protocol.http.comet_streaming == 0) {
		  tmp_connection.output_handler = NULL;
		}
#endif
	}

	/* get TCP offset and flags */
	DEV_GETC16(tmp_ui16);
	tcp_header_length = (tmp_ui16[S0] >> 4) * 4;

	/* TCP segment length calculation */

	/* packet_length is the length of the IP payload *without* IP headers */
	segment_length = packet_length - tcp_header_length;

	if(packet_length - tcp_header_length > 0)
		UI32(current_inseqno) += segment_length;

	new_tcp_data = UI32(current_inseqno) > UI32(tmp_connection.protocol.http.current_inseqno);

	if(UI32(current_inseqno) >= UI32(tmp_connection.protocol.http.current_inseqno)) {
		UI32(tmp_connection.protocol.http.current_inseqno) = UI32(current_inseqno);
		/* TCP state machine management */
		switch(tmp_connection.protocol.http.tcp_state) {
			case tcp_established:
				if(tmp_ui16[S1] & TCP_FIN) {
					tmp_connection.protocol.http.tcp_state = tcp_last_ack;
					tmp_connection.output_handler = &ref_finack;
					UI32(tmp_connection.protocol.http.current_inseqno)++;
				} else if(tmp_ui16[S1] & TCP_RST) {
					tmp_connection.protocol.http.tcp_state = tcp_listen;
				}
				break;
			case tcp_listen:
				if(tmp_ui16[S1] & TCP_SYN) {
					tmp_connection.protocol.http.tcp_state = tcp_syn_rcvd;
					tmp_connection.output_handler = &ref_synack;
					UI32(tmp_connection.protocol.http.current_inseqno)++;
				}
				break;
			case tcp_syn_rcvd:
				if(UI16(tmp_connection.protocol.http.inflight) == 0) {
					tmp_connection.protocol.http.tcp_state = tcp_established;
				} else {
					tmp_connection.output_handler = &ref_synack;
				}
				break;
			case tcp_last_ack:
				tmp_connection.protocol.http.tcp_state = tcp_listen;
				break;
			default:
				break;
		}
	}

	/* get the advertissed TCP window in order to limit our sending rate if needed */
	DEV_GETC16(tmp_connection.protocol.http.cwnd);

	/* discard the checksum (which is checksummed as other data) and the urgent pointer */
	DEV_GETC32(tmp_ui32);

	/* add the changing part of the TCP pseudo header checksum */
#ifdef IPV6
	checksum_add32(&local_ip_addr[0]);
	checksum_add32(&local_ip_addr[4]);
	checksum_add32(&local_ip_addr[8]);
	checksum_add32(&local_ip_addr[12]);

	checksum_add32(&full_ipv6_addr[0]);
	checksum_add32(&full_ipv6_addr[4]);
	checksum_add32(&full_ipv6_addr[8]);
	checksum_add32(&full_ipv6_addr[12]);
#else
	checksum_add32(local_ip_addr);
	checksum_add32(tmp_connection.ip_addr);

#endif
	checksum_add16(packet_length);

	/* get TCP mss (for initial negociation) */
	tcp_header_length -= TCP_HEADER_SIZE;
	if(tcp_header_length >= 4) {
		tcp_header_length -= 4;
		DEV_GETC16(tmp_ui16);
		DEV_GETC16(tmp_ui16);
		tmp_connection.protocol.http.tcp_mss = UI16(tmp_ui16) > MAX_MSS ? MAX_MSS : UI16(tmp_ui16);
	}

	/* discard the remaining part of the TCP header */
	for(; tcp_header_length > 0; tcp_header_length-=4) {
		DEV_GETC32(tmp_ui32);
	}

	/* End of TCP, starting HTTP */
	x = 0;
	if(segment_length && tmp_connection.protocol.http.tcp_state == tcp_established && (new_tcp_data || tmp_connection.output_handler == NULL)) {
		const struct output_handler_t * /*CONST_VAR*/ output_handler = NULL;

		/* parse the eventual GET request */
		unsigned const char * /*CONST_VAR*/ blob;
		unsigned char blob_curr;
#ifndef DISABLE_ARGS
		struct arg_ref_t tmp_arg_ref = {0,0,0};
		uint16_t tmp_args_size_ref;
#endif

		if(tmp_connection.protocol.http.parsing_state == parsing_out) {
#ifndef DISABLE_ARGS
			tmp_connection.protocol.http.args = NULL;
			tmp_connection.protocol.http.arg_ref_index = 128;
#endif
			tmp_connection.protocol.http.blob = blob_http_rqt;
			tmp_connection.protocol.http.parsing_state = parsing_cmd;
			tmp_connection.protocol.http.ready_to_send = 1;
			tmp_connection.output_handler = NULL;
#ifndef DISABLE_POST
			tmp_connection.protocol.http.post_data = NULL;
			tmp_connection.protocol.http.post_url_detected = 0;
#endif
		}
		else if(tmp_connection.output_handler)
			output_handler = tmp_connection.output_handler;
		blob = tmp_connection.protocol.http.blob;

#ifndef DISABLE_ARGS
		if(tmp_connection.protocol.http.arg_ref_index != 128) {
			struct arg_ref_t * /*CONST_VAR*/ tmp_arg_ref_ptr;
			tmp_arg_ref_ptr = &(((struct arg_ref_t*)CONST_ADDR(output_handler->handler_args.args_index))[tmp_connection.protocol.http.arg_ref_index]);
			tmp_arg_ref.arg_type = CONST_UI8(tmp_arg_ref_ptr->arg_type);
			tmp_arg_ref.arg_size = CONST_UI8(tmp_arg_ref_ptr->arg_size);
			tmp_arg_ref.arg_offset = CONST_UI8(tmp_arg_ref_ptr->arg_offset);
			tmp_args_size_ref = CONST_UI16(output_handler->handler_args.args_size);
		}
#endif
		while(x < segment_length && output_handler != &http_404_handler
#ifndef DISABLE_POST
				&& output_handler != &http_505_handler
#endif
				) {
			blob_curr = CONST_READ_UI8(blob);
#ifndef DISABLE_POST
			/* testing end multipart */
			if(tmp_connection.protocol.http.post_data
					&& tmp_connection.protocol.http.post_data->boundary
					&& tmp_connection.protocol.http.post_data->boundary->ready_to_count
					&& tmp_connection.protocol.http.post_data->content_length < 3){
				tmp_connection.protocol.http.ready_to_send = 1;
				tmp_connection.protocol.http.parsing_state = parsing_end;
				break;
			}
			if(tmp_connection.protocol.http.parsing_state != parsing_post_data){
#endif
				x++;
				DEV_GETC(tmp_char);
#ifndef DISABLE_POST
				/* updating content length */
				if((tmp_connection.protocol.http.parsing_state == parsing_init_buffer
						|| tmp_connection.protocol.http.parsing_state == parsing_post_args
						||(tmp_connection.protocol.http.post_data
						&& tmp_connection.protocol.http.post_data->boundary
						&& tmp_connection.protocol.http.post_data->boundary->ready_to_count))
						&& tmp_connection.protocol.http.post_data->content_length != (uint16_t)-1)
					tmp_connection.protocol.http.post_data->content_length--;
			}
			/* initializing buffer before parsing data */
			if(tmp_connection.protocol.http.parsing_state == parsing_init_buffer){
				tmp_connection.protocol.http.post_data->boundary->boundary_buffer[tmp_connection.protocol.http.post_data->boundary->index] = tmp_char;
				tmp_connection.protocol.http.post_data->boundary->index++;
				if(tmp_connection.protocol.http.post_data->boundary->index == tmp_connection.protocol.http.post_data->boundary->boundary_size){
					tmp_connection.protocol.http.parsing_state = parsing_post_data;
					/* verifying buffer */
					if(tmp_connection.protocol.http.post_data->boundary->boundary_buffer[(tmp_connection.protocol.http.post_data->boundary->index-1) % tmp_connection.protocol.http.post_data->boundary->boundary_size]
							== tmp_connection.protocol.http.post_data->boundary->boundary_ref[tmp_connection.protocol.http.post_data->boundary->boundary_size-1]
							&& tmp_connection.protocol.http.post_data->boundary->boundary_buffer[tmp_connection.protocol.http.post_data->boundary->index]
							== tmp_connection.protocol.http.post_data->boundary->boundary_ref[0]){
						unsigned char index = tmp_connection.protocol.http.post_data->boundary->index + 1;
						unsigned char i;
						for(i = 1 ; i < tmp_connection.protocol.http.post_data->boundary->boundary_size-1 ; i++){
							if(index == tmp_connection.protocol.http.post_data->boundary->boundary_size)
								index = 0;
							if(tmp_connection.protocol.http.post_data->boundary->boundary_ref[i] != tmp_connection.protocol.http.post_data->boundary->boundary_buffer[index])
								break;
							index++;
						}
						if(i == tmp_connection.protocol.http.post_data->boundary->boundary_size-1){
							tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
							blob = blob_http_header_content;
							tmp_connection.protocol.http.post_data->boundary->index = -1;
						}
					}
				}
				else
					continue;
			}
			/* parsing boundary */
			else if(tmp_connection.protocol.http.parsing_state == parsing_boundary){
				if(tmp_connection.protocol.http.post_data->boundary->index == tmp_connection.protocol.http.post_data->boundary->boundary_size)
					tmp_connection.protocol.http.post_data->boundary->index = 0; /* update index */
				/* comparing first and last characters then all */
				if(tmp_connection.protocol.http.post_data->boundary->boundary_buffer[(tmp_connection.protocol.http.post_data->boundary->index-1) % tmp_connection.protocol.http.post_data->boundary->boundary_size]
						== tmp_connection.protocol.http.post_data->boundary->boundary_ref[tmp_connection.protocol.http.post_data->boundary->boundary_size-1]
						&& tmp_connection.protocol.http.post_data->boundary->boundary_buffer[tmp_connection.protocol.http.post_data->boundary->index]
						== tmp_connection.protocol.http.post_data->boundary->boundary_ref[0]){
					unsigned char index = tmp_connection.protocol.http.post_data->boundary->index + 1;
					unsigned char i;
					for(i = 1 ; i < tmp_connection.protocol.http.post_data->boundary->boundary_size-1 ; i++){
						if(index == tmp_connection.protocol.http.post_data->boundary->boundary_size)
							index = 0;
						if(tmp_connection.protocol.http.post_data->boundary->boundary_ref[i] != tmp_connection.protocol.http.post_data->boundary->boundary_buffer[index])
							break;
						index++;
					}
					if(i == tmp_connection.protocol.http.post_data->boundary->boundary_size-1){
						tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
						blob = blob_http_header_content;
						tmp_connection.protocol.http.post_data->boundary->index = -1;
						continue;
					}
				}
				tmp_connection.protocol.http.post_data->boundary->boundary_buffer[tmp_connection.protocol.http.post_data->boundary->index] = tmp_char;
				tmp_connection.protocol.http.post_data->boundary->index++;
				continue;
			}
#endif
			/* search for the web applicative resource to send */
			if(blob_curr >= 128 && output_handler != &http_404_handler
#ifndef DISABLE_POST
					&& output_handler != &http_505_handler
#endif
				) {
#ifndef DISABLE_POST
				/* "\n\r\n\r" detected (end header or end part of multipart )*/
				if(tmp_connection.protocol.http.parsing_state == parsing_post_end){
					if(tmp_connection.protocol.http.post_data->content_type == (uint8_t)-1){ /* no content type */
						output_handler = &http_505_handler;
						break;
					}
					if(tmp_connection.protocol.http.post_data->boundary){
						tmp_connection.protocol.http.post_data->boundary->ready_to_count = 1;
						/* parsing header part of multipart */
						if(tmp_connection.protocol.http.post_data->content_type == CONTENT_TYPE_MULTIPART_47_FORM_45_DATA){
							tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
							blob = blob_http_header_content;
							continue;
						}
						else
							/* parsing data */
							tmp_connection.protocol.http.parsing_state = parsing_post_data;
					}
					/* parsing form */
					else if(tmp_connection.protocol.http.post_data->content_type == CONTENT_TYPE_APPLICATION_47_X_45_WWW_45_FORM_45_URLENCODED)
						tmp_connection.protocol.http.parsing_state = parsing_post_args;
					else{
						/* no content length detected = boundary is "\r\n\r\n" */
						if(tmp_connection.protocol.http.post_data->content_length == (uint16_t)-1){
							tmp_connection.protocol.http.post_data->boundary = mem_alloc(sizeof(struct boundary_t));
							if(!tmp_connection.protocol.http.post_data->boundary){
								output_handler = &http_404_handler;
								break;
							}
							tmp_connection.protocol.http.post_data->boundary->boundary_size = 4;
							tmp_connection.protocol.http.post_data->boundary->index = -1;
							tmp_connection.protocol.http.post_data->boundary->multi_part_counter = 0;
							tmp_connection.protocol.http.post_data->boundary->boundary_ref = "\r\n\r\n";
							tmp_connection.protocol.http.post_data->boundary->boundary_buffer = mem_alloc(4*sizeof(char));
							if(!tmp_connection.protocol.http.post_data->boundary->boundary_buffer){
								/* If boundary buffer can not be allocated, also free the boundary itself */
								mem_free(tmp_connection.protocol.http.post_data->boundary, sizeof(struct boundary_t));
								output_handler = &http_404_handler;
								break;
							}
						}
						tmp_connection.protocol.http.parsing_state = parsing_post_data;
					}
				}
				/* parsing post data => run coroutine */
				if(tmp_connection.protocol.http.parsing_state == parsing_post_data){
					/* starting buffer init */
					if(tmp_connection.protocol.http.post_data->coroutine.curr_context.status == cr_terminated
							&& tmp_connection.protocol.http.post_data->boundary
							&& tmp_connection.protocol.http.post_data->boundary->index == 255){
						tmp_connection.protocol.http.post_data->boundary->index = 0;
						tmp_connection.protocol.http.parsing_state = parsing_init_buffer;
						continue;
					}
					uint16_t length_temp = segment_length - x;
					/* initialization coroutine */
					if(tmp_connection.protocol.http.post_data->coroutine.curr_context.status == cr_terminated){
						cr_init(&(tmp_connection.protocol.http.post_data->coroutine));
						tmp_connection.protocol.http.post_data->coroutine.func.func_post_in = CONST_ADDR(GET_GENERATOR(output_handler).handlers.post.dopostin);
						tmp_connection.protocol.http.post_data->coroutine.params.in.content_type = tmp_connection.protocol.http.post_data->content_type;
						tmp_connection.protocol.http.post_data->coroutine.params.in.filename = tmp_connection.protocol.http.post_data->filename;
						tmp_connection.protocol.http.post_data->coroutine.params.in.post_data = tmp_connection.protocol.http.post_data->post_data;
						if(tmp_connection.protocol.http.post_data->boundary){
							tmp_connection.protocol.http.post_data->coroutine.params.in.part_number = tmp_connection.protocol.http.post_data->boundary->multi_part_counter;
							tmp_connection.protocol.http.post_data->boundary->index = 0;
						}
						else
							tmp_connection.protocol.http.post_data->coroutine.params.in.part_number = 0;
						if(cr_prepare(&(tmp_connection.protocol.http.post_data->coroutine)) == NULL)
						{
							/* TODO: shoudn't the boundary and boundary_buffer be free ? */
							return 1;
						}
					}
					curr_input.length = length_temp;
					curr_input.connection = &tmp_connection;
					coroutine_state.state = cor_in;
					/* running coroutine */
					cr_run(&(tmp_connection.protocol.http.post_data->coroutine),cor_type_post_in);
					coroutine_state.state = cor_out;
					x += (length_temp-curr_input.length);
					if(tmp_connection.protocol.http.post_data->coroutine.curr_context.status == cr_terminated){ /* if is terminated */
						tmp_connection.protocol.http.post_data->post_data = tmp_connection.protocol.http.post_data->coroutine.params.in.post_data;
						cr_clean(&(tmp_connection.protocol.http.post_data->coroutine));
						/* cleaning filename memory */
						if(tmp_connection.protocol.http.post_data->filename){
							uint8_t i = 0;
							while(tmp_connection.protocol.http.post_data->filename[i++] != '\0');
							mem_free(tmp_connection.protocol.http.post_data->filename,i*sizeof(char));
							tmp_connection.protocol.http.post_data->filename = NULL;
						}
						/* if no boundary, there is no data to parse */
						if(!tmp_connection.protocol.http.post_data->boundary){
							tmp_connection.protocol.http.parsing_state = parsing_end;
							tmp_connection.protocol.http.ready_to_send = 1;
						}
						else{
							/* starting search of boundary if no found, or ready for next part */
							tmp_connection.protocol.http.post_data->boundary->multi_part_counter++;
							if(tmp_connection.protocol.http.post_data->boundary->index != (uint8_t)-1)
								tmp_connection.protocol.http.parsing_state = parsing_boundary;
							else{
								tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
								blob = blob_http_header_content;
							}
							/* if no content length, parsing is terminated */
							if(tmp_connection.protocol.http.post_data->content_length == (uint16_t)-1){
								tmp_connection.protocol.http.parsing_state = parsing_end;
								tmp_connection.protocol.http.ready_to_send = 1;
								break;
							}
						}
					}
					if(tmp_connection.protocol.http.post_data->boundary)
						continue;
					break;
				}
				/* searching end character of post url detection */
				if(tmp_connection.protocol.http.parsing_state == parsing_url && blob_curr == URL_POST_END){
					if(tmp_connection.protocol.http.post_data){
						tmp_connection.protocol.http.post_url_detected = 1;
						blob_curr = CONST_READ_UI8(++blob);
					}
					else{ /* get unauthorized */
						output_handler = &http_404_handler;
						break;
					}
				}
			 	/* parsing attributes of post header */
				if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes){
					if(blob_curr == ATTRIBUT_CONTENT_45_TYPE + 128) { /* content-type */
						blob = mimes_tree;
						tmp_connection.protocol.http.parsing_state = parsing_post_content_type;
					}
					else if(blob_curr == ATTRIBUT_CONTENT_45_LENGTH + 128){ /* content-length */
						if(tmp_connection.protocol.http.post_data->content_length == (uint8_t)-1)
							tmp_connection.protocol.http.post_data->content_length = 0;
						if(tmp_char != ' ' && tmp_char != '\r'){
							tmp_connection.protocol.http.post_data->content_length *= 10;
							tmp_connection.protocol.http.post_data->content_length += tmp_char - '0';
						}
					}
					else if(blob_curr == ATTRIBUT_FILENAME_61_ + 128){ /* filename */
						if(!tmp_connection.protocol.http.post_data->filename){
							tmp_connection.protocol.http.post_data->filename = mem_alloc(10 * sizeof(char));
							if(!tmp_connection.protocol.http.post_data->filename){
								output_handler = &http_404_handler;
								break;
							}
							tmp_connection.protocol.http.post_data->filename[0] = 2; /* first value is the size of filename */
							tmp_connection.protocol.http.post_data->filename[1] = 10; /* seconde value is the size allocated */
						}
						else if(tmp_char == '\"'){ /* end of filename, the indexes are removed */
							uint8_t i = 0;
							char *new_tab = mem_alloc((tmp_connection.protocol.http.post_data->filename[0]-1)*sizeof(char));
							if(!new_tab){
								output_handler = &http_404_handler;
								break;
							}
							for(i = 0 ; i < tmp_connection.protocol.http.post_data->filename[0]-2 ; i++)
								new_tab[i] = tmp_connection.protocol.http.post_data->filename[i+2];
							new_tab[i]='\0';
							mem_free(tmp_connection.protocol.http.post_data->filename,(tmp_connection.protocol.http.post_data->filename[1])*sizeof(char));
							tmp_connection.protocol.http.post_data->filename = new_tab;
							tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
							blob = blob_http_header_content;
						}
						else{
							/* reallocating filename */
							if(tmp_connection.protocol.http.post_data->filename[0] == tmp_connection.protocol.http.post_data->filename[1]){
								tmp_connection.protocol.http.post_data->filename = mem_realloc(tmp_connection.protocol.http.post_data->filename,tmp_connection.protocol.http.post_data->filename[1],10);
								if(!tmp_connection.protocol.http.post_data->filename){
									output_handler = &http_404_handler;
									break;
								}
								/* updating size */
		 						tmp_connection.protocol.http.post_data->filename[1] = tmp_connection.protocol.http.post_data->filename[1] + 10;
							}
							/* add current character */
							tmp_connection.protocol.http.post_data->filename[tmp_connection.protocol.http.post_data->filename[0]] = tmp_char;
							tmp_connection.protocol.http.post_data->filename[0]++;
						}
					}
					/* parsing boundary */
					else if(blob_curr == ATTRIBUT_BOUNDARY_61_ + 128){
						if(tmp_char != '\r'){
							/* reallocating tab */
							if(tmp_connection.protocol.http.post_data->boundary->index == tmp_connection.protocol.http.post_data->boundary->boundary_size){
								tmp_connection.protocol.http.post_data->boundary->boundary_ref = mem_realloc(tmp_connection.protocol.http.post_data->boundary->boundary_ref,tmp_connection.protocol.http.post_data->boundary->boundary_size,10);
								if(!tmp_connection.protocol.http.post_data->boundary->boundary_ref){
									output_handler = &http_404_handler;
									break;
								}
								/* updating size */
								tmp_connection.protocol.http.post_data->boundary->boundary_size += 10;
							}
							tmp_connection.protocol.http.post_data->boundary->boundary_ref[tmp_connection.protocol.http.post_data->boundary->index++] = tmp_char; /* saving boundary char */
						}
						else{
							/* final reallocation */
							uint8_t i = 0;
							char *new_tab = mem_alloc((tmp_connection.protocol.http.post_data->boundary->index+5)*sizeof(char));
							if(!new_tab){
								output_handler = &http_404_handler;
								break;
							}
							new_tab[0] = '\n';
							new_tab[1] = '\r';
							new_tab[2] = '\n';
							new_tab[3] = '-';
							new_tab[4] = '-';
							/* copying boundary */
							for(i = 0 ; i < tmp_connection.protocol.http.post_data->boundary->index ; i++)
								new_tab[i+5] = tmp_connection.protocol.http.post_data->boundary->boundary_ref[i];
							mem_free(tmp_connection.protocol.http.post_data->boundary->boundary_ref,(tmp_connection.protocol.http.post_data->boundary->boundary_size)*sizeof(char));
							tmp_connection.protocol.http.post_data->boundary->boundary_size = tmp_connection.protocol.http.post_data->boundary->index + 5;
							tmp_connection.protocol.http.post_data->boundary->boundary_ref = new_tab;
							tmp_connection.protocol.http.post_data->boundary->index = -1;
							tmp_connection.protocol.http.post_data->boundary->boundary_buffer = mem_alloc(tmp_connection.protocol.http.post_data->boundary->boundary_size*sizeof(char));
							if(!tmp_connection.protocol.http.post_data->boundary->boundary_buffer){
								/* If no space for boundary_ref, free boundary */
								mem_free(tmp_connection.protocol.http.post_data->boundary, sizeof(struct boundary_t));
								output_handler = &http_404_handler;
								break;
							}
						}
					}
				}
				/* parsing content-type */
				else if(tmp_connection.protocol.http.parsing_state == parsing_post_content_type){
					if(blob_curr - 128 == CONTENT_TYPE_MULTIPART_47_FORM_45_DATA){ /* multipart */
						/* allocating boundary structure */
						tmp_connection.protocol.http.post_data->boundary = mem_alloc(sizeof(struct boundary_t));
						if(!tmp_connection.protocol.http.post_data->boundary){
							output_handler = &http_404_handler;
							break;
						}
						tmp_connection.protocol.http.post_data->boundary->boundary_size = 10;
						tmp_connection.protocol.http.post_data->boundary->index = 0;
						tmp_connection.protocol.http.post_data->boundary->multi_part_counter = 0;
						tmp_connection.protocol.http.post_data->boundary->ready_to_count = 0;
						tmp_connection.protocol.http.post_data->boundary->boundary_ref = mem_alloc(10*sizeof(char));
						if(!tmp_connection.protocol.http.post_data->boundary->boundary_ref){
							/* If no space for boundary_ref, free boundary */
							mem_free(tmp_connection.protocol.http.post_data->boundary, sizeof(struct boundary_t));
							output_handler = &http_404_handler;
							break;
						}
						tmp_connection.protocol.http.post_data->content_type = CONTENT_TYPE_MULTIPART_47_FORM_45_DATA;
					}
					else{ /* searching content-type in application to verify it */
						tmp_connection.protocol.http.post_data->content_type = (uint8_t) -1;
						if(!output_handler->handler_mimes.mimes_size)
							tmp_connection.protocol.http.post_data->content_type = blob_curr - 128;
						else{
							uint8_t i;
							for(i = 0 ; i < output_handler->handler_mimes.mimes_size ; i++){
								if(output_handler->handler_mimes.mimes_index[i] == blob_curr - 128)
									tmp_connection.protocol.http.post_data->content_type = blob_curr - 128;
							}
						}
						if(tmp_connection.protocol.http.post_data->content_type == (uint8_t) -1){
							output_handler = &http_404_handler;
							break;
						}
					}
					tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
					blob = blob_http_header_content;
				} else
#endif
				if(tmp_connection.protocol.http.parsing_state == parsing_cmd) {
#ifndef DISABLE_POST
					if(blob_curr == REQUEST_POST_32_ + 128){ /* post request */
						/* allocating post data structure */
						tmp_connection.protocol.http.post_data = mem_alloc(sizeof(struct post_data_t));
						if(!tmp_connection.protocol.http.post_data){
							output_handler = &http_404_handler;
							break;
						}
						tmp_connection.protocol.http.post_data->content_type = -1;
						tmp_connection.protocol.http.post_data->content_length = -1;
						tmp_connection.protocol.http.post_data->boundary = NULL;
						tmp_connection.protocol.http.post_data->post_data = NULL;
						tmp_connection.protocol.http.post_data->filename = NULL;
						tmp_connection.protocol.http.post_data->coroutine.curr_context.status = cr_terminated;
					}
#endif
					tmp_connection.protocol.http.parsing_state = parsing_url;
					blob = urls_tree;
				} else {
					if(tmp_char == ' ') {
#ifndef DISABLE_POST
						if(tmp_connection.protocol.http.post_data && tmp_connection.protocol.http.post_url_detected == 0) { /* post unauthorized */
							output_handler = &http_404_handler;
							break;
						}
						else{
#endif
							if(!output_handler)
								output_handler = (struct output_handler_t*)CONST_ADDR(resources_index[blob_curr - 128]);
#ifndef DISABLE_POST
							if(tmp_connection.protocol.http.post_data){
								tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
								blob = blob_http_header_content;
								tmp_connection.protocol.http.ready_to_send = 0;
							}
							else{
#endif
								tmp_connection.protocol.http.parsing_state = parsing_end;
								break;
#ifndef DISABLE_POST
							}
						}
#endif
					} else {
#ifndef DISABLE_ARGS
						if(tmp_char == '?'
#ifndef DISABLE_POST
						||	(tmp_connection.protocol.http.parsing_state == parsing_post_args && tmp_char == 10)
#endif
						){
							uint16_t tmp_args_size;
							if(!output_handler)
								output_handler = (struct output_handler_t*)CONST_ADDR(resources_index[blob_curr - 128]);
							tmp_args_size = CONST_UI16(output_handler->handler_args.args_size);
							tmp_args_size_ref = tmp_args_size;
							tmp_connection.protocol.http.ready_to_send = 0;
							if(tmp_args_size) {
								uint16_t i;
								blob = (const unsigned char *)CONST_ADDR(output_handler->handler_args.args_tree);
								tmp_connection.protocol.http.args = mem_alloc(tmp_args_size); /* test NULL: done */
								if(tmp_connection.protocol.http.args == NULL) {
									output_handler = &http_404_handler;
									break;
								}
								for(i = 0; i < tmp_args_size ; i++) {
									((unsigned char *)tmp_connection.protocol.http.args)[i] = 0;
								}
								continue;
							}
						} else if(tmp_char == '=' && tmp_connection.protocol.http.args) {
							struct arg_ref_t * /*CONST_VAR*/ tmp_arg_ref_ptr;
							tmp_connection.protocol.http.arg_ref_index = blob_curr - 128;
							tmp_arg_ref_ptr = &(((struct arg_ref_t*)CONST_ADDR(output_handler->handler_args.args_index))[tmp_connection.protocol.http.arg_ref_index]);
							tmp_arg_ref.arg_type = CONST_UI8(tmp_arg_ref_ptr->arg_type);
							tmp_arg_ref.arg_size = CONST_UI8(tmp_arg_ref_ptr->arg_size);
							tmp_arg_ref.arg_offset = CONST_UI8(tmp_arg_ref_ptr->arg_offset);
							tmp_connection.protocol.http.curr_arg = ((unsigned char*)tmp_connection.protocol.http.args) + tmp_arg_ref.arg_offset;
							if(tmp_arg_ref.arg_type == arg_str)
								(*((unsigned char*)tmp_connection.protocol.http.curr_arg + tmp_arg_ref.arg_size - 1)) = tmp_arg_ref.arg_size - 1;
							continue;
						} else if(tmp_char == '&') {
							blob = (const unsigned char *)CONST_ADDR(output_handler->handler_args.args_tree);
						} else {
							blob++;
						}
#else
					blob++;
#endif
					}
				}
			blob_curr = CONST_READ_UI8(blob);
			}
#ifndef DISABLE_POST
			/* end header detection */
			if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes && tmp_char == 13){
				blob = blob_http_header_end;
				tmp_connection.protocol.http.parsing_state = parsing_post_end;
			}
			else if(tmp_connection.protocol.http.parsing_state == parsing_post_end){
				if(tmp_char != blob_curr){
					blob = blob_http_header_content;
					tmp_connection.protocol.http.parsing_state = parsing_post_attributes;
				}
			}
#endif
#ifndef DISABLE_ARGS
			if(tmp_connection.protocol.http.arg_ref_index != 128) {
				if(tmp_char == '&') {
					tmp_connection.protocol.http.arg_ref_index = 128;
					blob = (const unsigned char *)CONST_ADDR(output_handler->handler_args.args_tree);
					continue;
				} else if(tmp_char == ' ') {
					tmp_connection.protocol.http.ready_to_send = 1;
					tmp_connection.protocol.http.parsing_state = parsing_end;
					break;
				} else {
					switch(tmp_arg_ref.arg_type) {
						case arg_str: {
							unsigned char *tmp_size_ptr = ((unsigned char*)tmp_connection.protocol.http.curr_arg + tmp_arg_ref.arg_size - 1);
							if(*tmp_size_ptr) {
								*((unsigned char*)tmp_connection.protocol.http.curr_arg + (tmp_arg_ref.arg_size - *tmp_size_ptr - 1)) = tmp_char;
								(*tmp_size_ptr)--;
							}
							break;
						}
						case arg_ui8:
							*((unsigned char*)tmp_connection.protocol.http.curr_arg) *= 10;
							*((unsigned char*)tmp_connection.protocol.http.curr_arg) += tmp_char - '0';
							break;
						case arg_ui16:
							*((uint16_t*)tmp_connection.protocol.http.curr_arg) *= 10;
							*((uint16_t*)tmp_connection.protocol.http.curr_arg) += tmp_char - '0';
							break;
					}
				}
			} else
#endif
#ifndef DISABLE_POST
				if(tmp_connection.protocol.http.parsing_state == parsing_post_end && blob_curr != HEADER_POST_END){
					blob++;
				}
				else if(
						!(tmp_connection.protocol.http.parsing_state == parsing_post_content_type && (tmp_char == 32 || tmp_char == 58))
						&& !(tmp_connection.protocol.http.parsing_state == parsing_post_attributes && blob_curr == ATTRIBUT_CONTENT_45_LENGTH + 128)
						&& !(tmp_connection.protocol.http.parsing_state == parsing_post_attributes && blob_curr == ATTRIBUT_BOUNDARY_61_ + 128)
						&& !(tmp_connection.protocol.http.parsing_state == parsing_post_attributes && blob_curr == ATTRIBUT_FILENAME_61_ + 128)
				)
#endif
			{
				do {

					unsigned char offsetInf = 0;
					unsigned char offsetEq = 0;
					unsigned char blob_next;
#ifndef DISABLE_POST
					if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes && tmp_char < 123 && tmp_char > 96)
						tmp_char = tmp_char - 'a' + 'A';
#endif
					blob_curr = CONST_READ_UI8(blob);
					blob_next = CONST_READ_UI8(++blob);
					if (tmp_char != blob_curr && blob_next >= 128) {
						blob_next = CONST_READ_UI8(++blob);
					}
					if (blob_next < 32) {
						offsetInf += ((blob_next>>2) & 1) + ((blob_next>>1) & 1) + (blob_next & 1);
						offsetEq = offsetInf + ((blob_next & 2)?CONST_READ_UI8(blob+1):0);
					}
					if (tmp_char == blob_curr) {
						if (blob_next < 32) {
							if (blob_next & 2) {
								blob += offsetEq;
							} else {
#ifndef DISABLE_POST
								if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes)
									blob = blob_http_header_content;
								else
#endif
									output_handler = &http_404_handler;
								break;
							}
						}
						break;
					} else if (tmp_char < blob_curr) {
						if (blob_next < 32 && blob_next & 1) {
							blob += offsetInf;
						} else {
#ifndef DISABLE_POST
							if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes)
								blob = blob_http_header_content;
							else
#endif
								output_handler = &http_404_handler;
							break;
						}
					} else {
						if (blob_next < 32 && blob_next & 4) {
							unsigned char offsetSup = offsetEq + ((blob_next & 3)?CONST_READ_UI8(blob+(offsetInf-1)):0);
							blob += offsetSup;
						} else {
#ifndef DISABLE_POST
							if(tmp_connection.protocol.http.parsing_state == parsing_post_attributes)
								blob = blob_http_header_content;
							else
#endif
								output_handler = &http_404_handler;
							break;
						}
					}
				} while(1);
			}
		}
		/* detecting parsing_end */
		if(
#ifndef DISABLE_POST
				(tmp_connection.protocol.http.parsing_state == parsing_post_args && !tmp_connection.protocol.http.post_data->content_length) ||
#endif
				((output_handler == &http_404_handler
#ifndef DISABLE_POST
					|| output_handler == &http_505_handler
#endif
					) && tmp_connection.protocol.http.parsing_state != parsing_cmd)){
			tmp_connection.protocol.http.parsing_state = parsing_end;
			tmp_connection.protocol.http.ready_to_send = 1;
		}
		if(!output_handler)
			tmp_connection.protocol.http.blob = blob;
		else {
			if(tmp_connection.protocol.http.parsing_state != parsing_cmd) {
				tmp_connection.output_handler = output_handler;
				UI32(tmp_connection.protocol.http.next_outseqno) = UI32(current_inack);
				if(CONST_UI8(output_handler->handler_type) == type_file) {
					UI32(tmp_connection.protocol.http.final_outseqno) = UI32(tmp_connection.protocol.http.next_outseqno) + CONST_UI32(GET_FILE(output_handler).length);
				} else {
					UI32(tmp_connection.protocol.http.final_outseqno) = UI32(tmp_connection.protocol.http.next_outseqno) - 1;
				}
#ifndef DISABLE_COMET
				tmp_connection.protocol.http.comet_send_ack = CONST_UI8(output_handler->handler_comet) ? 1 : 0;
				tmp_connection.protocol.http.comet_passive = 0;
				tmp_connection.protocol.http.comet_streaming = 0;
#endif
			}
			if(tmp_connection.protocol.http.parsing_state == parsing_end || tmp_connection.protocol.http.parsing_state == parsing_cmd){
#ifndef DISABLE_POST
				/* cleaning memory */
				if(tmp_connection.protocol.http.post_data){ /* Warning : don't clean post_data, useful in output function (unless error 404 or 505) */
					if(tmp_connection.protocol.http.post_data->filename){
						uint8_t i = 0;
						while(tmp_connection.protocol.http.post_data->filename[i++] != '\0');
						mem_free(tmp_connection.protocol.http.post_data->filename,i*sizeof(char));
						tmp_connection.protocol.http.post_data->filename = NULL;
					}
					if(tmp_connection.protocol.http.post_data->boundary){
						if(tmp_connection.protocol.http.post_data->boundary->boundary_ref && tmp_connection.protocol.http.post_data->content_length != (uint16_t)-1)
							mem_free(tmp_connection.protocol.http.post_data->boundary->boundary_ref,tmp_connection.protocol.http.post_data->boundary->boundary_size*sizeof(char));
						if(tmp_connection.protocol.http.post_data->boundary->boundary_buffer)
							mem_free(tmp_connection.protocol.http.post_data->boundary->boundary_buffer,(tmp_connection.protocol.http.post_data->boundary->boundary_size)*sizeof(char));
						mem_free(tmp_connection.protocol.http.post_data->boundary,sizeof(struct boundary_t));
					}
					tmp_connection.protocol.http.post_data->boundary = NULL;
				}
				if(output_handler == &http_404_handler
#ifndef DISABLE_POST
						|| output_handler == &http_505_handler
#endif
						){
					/* cleaning post_data if error */
					if(tmp_connection.protocol.http.post_data){
						mem_free(tmp_connection.protocol.http.post_data,sizeof(struct post_data_t));
						tmp_connection.protocol.http.post_data = NULL;
					}
#ifndef DISABLE_ARGS
					/* cleaning args data if error */
					if(tmp_connection.protocol.http.args){
						mem_free(tmp_connection.protocol.http.args,tmp_args_size_ref);
						tmp_connection.protocol.http.args = NULL;
					}
#endif
				}
#endif
				tmp_connection.protocol.http.parsing_state = parsing_out;
				tmp_connection.protocol.http.blob = blob_http_rqt;
#ifndef DISABLE_ARGS
				tmp_connection.protocol.http.arg_ref_index = 128;
#endif
			}
			else
				tmp_connection.protocol.http.blob = blob;
		}
	}
	/* drop remaining TCP data */
	while(x++ < segment_length)
		DEV_GETC(tmp_char);

	/* acknowledge received and processed TCP data if no there is no current output_handler */
	if(!tmp_connection.output_handler && tmp_connection.protocol.http.tcp_state == tcp_established && segment_length) {
		tmp_connection.output_handler = &ref_ack;
	}

	/* check TCP checksum using the partially precalculated pseudo header checksum */
	checksum_end();
	if(UI16(current_checksum) == TCP_CHK_CONSTANT_PART) {
		if(defer_clean_service) { /* free in-flight segment information for acknowledged segments */
			clean_service(tmp_connection.protocol.http.generator_service, current_inack);
			if(defer_free_handler) { /* free handler and generator service is the service is completely acknowledged */
#ifndef DISABLE_COROUTINES
				cr_clean(&tmp_connection.protocol.http.generator_service->coroutine);
#endif
				mem_free(tmp_connection.protocol.http.generator_service, sizeof(struct generator_service_t));
				tmp_connection.protocol.http.generator_service = NULL;
#ifndef DISABLE_ARGS
				mem_free(defer_free_args, defer_free_args_size);
#endif
			}
		}

		if(!connection && tmp_connection.protocol.http.tcp_state == tcp_syn_rcvd) {
			connection = add_connection(&tmp_connection);
			/* update the pointer in the tmp_connection because
			 * it will be copied later so if the pointers do not have the right value, the list
			 * will be screwed */
			if (connection)
			{
				tmp_connection.prev = connection->prev;
				tmp_connection.next = connection->next;
			}
		}

		if(!connection) {
			/* no valid connection has been found for this packet, send a reset */
			UI32(tmp_connection.protocol.http.next_outseqno) = UI32(current_inack);
#ifdef IPV6
			memcpy(rst_connection.ip_addr, full_ipv6_addr, 16);
#else
			UI32(rst_connection.ip_addr) = UI32(tmp_connection.ip_addr);
#endif
			UI16(rst_connection.port) = UI16(tmp_connection.protocol.http.port);
			UI32(rst_connection.current_inseqno) = UI32(tmp_connection.protocol.http.current_inseqno);
			UI32(rst_connection.next_outseqno) = UI32(tmp_connection.protocol.http.next_outseqno);
		} else {
			if(tmp_connection.protocol.http.tcp_state == tcp_listen) {
				free_connection(connection);
			} else {
				/* update the current connection */
				*connection = tmp_connection;
#ifdef IPV6
				copy_compressed_ip(connection->ip_addr, comp_ipv6_addr);
#endif
			}
		}
	}
	return 1;
}
