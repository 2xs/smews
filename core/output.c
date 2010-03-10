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

#include "output.h"
#include "types.h"
#include "checksum.h"
#include "timers.h"
#include "coroutines.h"
#include "smews.h"
#include "connections.h"
#include "memory.h"
#include "random.h"

#ifndef DISABLE_TLS
	#include "tls.h"
	#include "hmac.h"
#endif

/* Common values used for IP and TCP headers */
#define MSS_OPT 0x0204
#define IP_VHL_TOS ((uint16_t)0x4500)
#define IP_ID 0x0000
#define IP_OFFSET 0x0000
#define IP_TTL_PROTOCOL 0x4006
#define TCP_HTTP_PORT 80
#define TCP_HTTPS_PORT 443
#define TCP_WINDOW 0x1000
#define TCP_URGP 0x0000

/* Pre-calculated partial IP checksum (for outgoing packets) */
#define BASIC_IP_CHK 0x8506

/* Pre-calculated partial TCP checksum (for outgoing packets) */
/* Constant parts are : Window + IP_PROTO_TCP(6) */
#define BASIC_TCP_CHK 0x1006

/* Macros for static contents partial checksum blocks */
#define CHUNCKS_SIZE (1 << CHUNCKS_NBITS)
#define DIV_BY_CHUNCKS_SIZE(l) ((l) >> CHUNCKS_NBITS)
#define MODULO_CHUNCKS_SIZE(l) ((l) & ~(~0 << (CHUNCKS_NBITS)))
#define GET_NB_BLOCKS(l) (DIV_BY_CHUNCKS_SIZE(l) + (MODULO_CHUNCKS_SIZE(l) != 0))

/* Connection handler callback */
#ifndef DISABLE_ARGS
#define HANDLER_CALLBACK(connection,handler) { \
	if(CONST_ADDR(GET_SERVICE((connection)->output_handler).handler) != NULL) \
		((handler ## _service_func_t*)CONST_ADDR(GET_SERVICE((connection)->output_handler).handler))((connection)->args);}
#else
#define HANDLER_CALLBACK(connection,handler) { \
	if(CONST_ADDR(GET_SERVICE((connection)->output_handler).handler) != NULL) \
		((handler ## _service_func_t*)CONST_ADDR(GET_SERVICE((connection)->output_handler).handler))(NULL);}
#endif

/* Partially pre-calculated HTTP/1.1 header with checksum */
static CONST_VAR(char, serviceHttpHeader[]) = "HTTP/1.1 200 OK\r\nContent-Length:";
static CONST_VAR(char, serviceHttpHeaderPart2[]) = "\r\nContent-Type: text/plain\r\n\r\n";
static CONST_VAR(char, serviceHttpHeaderChunked[]) = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nTransfer-Encoding:chunked\r\n\r\n";

#define SERVICE_HTTP_HEADER_CHK 0x1871u
#define SERVICE_HTTP_HEADER_CHUNKED_CHK 0x2876u

struct curr_output_t {
	struct generator_service_t *service;
	char *buffer;
	unsigned char checksum[2];
	uint16_t content_length;
	unsigned char next_outseqno[4];
	enum service_header_e service_header: 2;
};
static struct curr_output_t curr_output;

/* default DEV_PUT16 */


void dev_put16(unsigned char *word) {
        DEV_PUT(word[1]);
        DEV_PUT(word[0]);
}

void dev_put16_val(uint16_t word) {
	DEV_PUT(word >> 8);
	DEV_PUT(word);
}


/* default DEV_PUT32 */

void dev_put32(unsigned char *dword) {
	DEV_PUT16(dword+2);
	DEV_PUT16(dword);
}


/* default DEV_PUTN */
#ifndef DEV_PUTN
	#define DEV_PUTN(ptr,n) { uint16_t i; \
		for(i = 0; i < (n); i++) { \
			DEV_PUT(ptr[i]); \
		} \
	}
#endif

/* default DEV_PUTN_CONST */
#ifndef DEV_PUTN_CONST
	#define DEV_PUTN_CONST(ptr,n) { uint16_t i; \
		for(i = 0; i < (n); i++) { \
			DEV_PUT(CONST_READ_UI8((ptr) + i)); \
		} \
	}
#endif

/*-----------------------------------------------------------------------------------*/
char out_c(char c) {
	if(curr_output.content_length == OUTPUT_BUFFER_SIZE) {
		cr_run(NULL);
	}
	checksum_add(c);
	curr_output.buffer[curr_output.content_length++] = c;
	return 1;
}

/*-----------------------------------------------------------------------------------*/
void smews_send_packet(struct http_connection *connection) {
	uint32_t index_in_file;
	uint16_t segment_length;
	unsigned char *ip_addr;
	unsigned char *port;
	unsigned char *next_outseqno;
	unsigned char *current_inseqno;
	const struct output_handler_t * /*CONST_VAR*/ output_handler;
	enum handler_type_e handler_type;
	/* buffer used to store the current content-length */
	#define CONTENT_LENGTH_SIZE 6
	#define CHUNK_LENGTH_SIZE 4
	char content_length_buffer[CONTENT_LENGTH_SIZE];
	unsigned char source_port[2];


#ifndef DISABLE_TLS
	/* used to construct TLS record */
	uint8_t *tls_record;
	//uint16_t record_length;

	/* shameless variable TODO revise */
	uint8_t *record_buffer;
#endif

#ifdef SMEWS_SENDING
	SMEWS_SENDING;
#endif

	if(connection) {
#ifndef DISABLE_TIMERS
		connection->transmission_time = last_transmission_time;
#endif
		ip_addr = connection->ip_addr;
		port = connection->port;
		next_outseqno = connection->next_outseqno;
		current_inseqno = connection->current_inseqno;
		output_handler = connection->output_handler;
		if(connection->tls_active == 1){
			UI16(source_port) = TCP_HTTPS_PORT;
		} else {
			UI16(source_port) = TCP_HTTP_PORT;
		}
	} else {
		ip_addr = rst_connection.ip_addr;
		port = rst_connection.port;
		next_outseqno = rst_connection.next_outseqno;
		current_inseqno = rst_connection.current_inseqno;
		output_handler = &ref_rst;
		if(rst_connection.tls_active == 1){
			UI16(source_port) = TCP_HTTPS_PORT;
		} else {
			UI16(source_port) = TCP_HTTP_PORT;
		}
	}
	handler_type = CONST_UI8(output_handler->handler_type);

	/* compute the length of the TCP segment to be sent */
	switch(handler_type) {
		case type_control:
			segment_length = CONST_UI8(GET_CONTROL(output_handler).length);
			break;
		case type_file: {
			uint16_t max_out_size;

			/* bytes remaining to be sent on this connection for the output handler currently active (file size, TCP and TLS headers) */
			uint32_t remaining_bytes;
			max_out_size = MAX_OUT_SIZE((uint16_t)connection->tcp_mss);

			remaining_bytes = UI32(connection->final_outseqno) - UI32(next_outseqno);

			/* segment length contains TLS OVERHEAD! */
			segment_length = remaining_bytes > max_out_size ? max_out_size : remaining_bytes;


#ifndef DISABLE_TLS
			if(connection->tls_active == 1){

				/* we reposition taking in account that remaining_bytes contains all TLS OVERHEAD */
				index_in_file = CONST_UI32(GET_FILE(output_handler).length) -
						(remaining_bytes - TLS_OVERHEAD*((remaining_bytes +  MAX_OUT_SIZE((uint16_t)connection->tcp_mss) - 1) / MAX_OUT_SIZE((uint16_t)connection->tcp_mss)));
				//printf("index in file %d\n",index_in_file);

			} else
#endif
			{

				index_in_file = CONST_UI32(GET_FILE(output_handler).length) - remaining_bytes;

			}

			//printf("file rem bytes %d, seg len %d\n",remaining_bytes, segment_length);
			//printf("I'm going to send %d octets as data (type file):\n",segment_length);
			break;
		}
		case type_generator:
			segment_length = curr_output.content_length;
			switch(curr_output.service_header) {
				case header_standard:
					segment_length += sizeof(serviceHttpHeader) - 1 + sizeof(serviceHttpHeaderPart2) - 1 + CONTENT_LENGTH_SIZE;
					break;
				case header_chunks:
					segment_length += sizeof(serviceHttpHeaderChunked) - 1;
				case header_none:
					segment_length += CHUNK_LENGTH_SIZE + 4;
					break;
			}
			break;
#ifndef DISABLE_TLS
		case type_tls_handshake:

			switch(connection->tls->tls_state) {
				case server_hello:
					/* todo limitation of mss discuss(fie las asa ceea ce nu ar putea fi o pb, fie il fac dynamic content) */
					segment_length = TLS_HELLO_CERT_DONE_LEN;
					break;

				/* sending the CCS & Finished message in one segment */
				case ccs_fin_send:
					segment_length = TLS_CHANGE_CIPHER_SPEC_LEN + TLS_FINISHED_MSG_LEN + TLS_RECORD_HEADER_LEN;
					break;

			}

			break;
#endif
	}

	DEV_PREPARE_OUTPUT(segment_length + 40);

	/* start to send IP header */

	/* send vhl, tos, IP header length */
	DEV_PUT16_VAL(IP_VHL_TOS);
	
	/* send IP packet length */
	DEV_PUT16_VAL(segment_length + 40);

	/* send IP ID, offset, ttl and protocol (TCP) */
	DEV_PUT16_VAL(IP_ID);
	DEV_PUT16_VAL(IP_OFFSET);
	DEV_PUT16_VAL(IP_TTL_PROTOCOL);

	/* complete IP precalculated checksum */
	checksum_init();
	UI16(current_checksum) = BASIC_IP_CHK;
	checksum_add32(local_ip_addr);
	checksum_add16(segment_length + 40);

	checksum_add32(ip_addr);
	checksum_end();

	/* send IP checksum */
	DEV_PUT16_VAL(~UI16(current_checksum));
	
	/* send IP source address */
	DEV_PUT32(local_ip_addr);
	
	/* send IP destination address */
	DEV_PUT32(ip_addr);
	
	/* start to send TCP header */

	/* send TCP source port */

	DEV_PUT16(source_port);

	/* send TCP destination port */
	DEV_PUT16(port);

	/* send TCP sequence number */
	DEV_PUT32(next_outseqno);
	
	/* send TCP acknowledgement number */
	DEV_PUT32(current_inseqno);
	
	/* send TCP header length & flags */
	DEV_PUT(GET_FLAGS(output_handler) & TCP_SYN ? 0x60 : 0x50);
	DEV_PUT(GET_FLAGS(output_handler));

	/* send TCP window */
	DEV_PUT16_VAL(TCP_WINDOW);
	
	/* complete precalculated TCP checksum */

	checksum_init();
	/* start to construct TCP checksum starting from a precalculated value */
	UI16(current_checksum) = BASIC_TCP_CHK;
	checksum_add32(local_ip_addr);

	checksum_add16(segment_length + 20);

	checksum_add32(next_outseqno);
	
	checksum_add(GET_FLAGS(output_handler) & TCP_SYN ? 0x60 : 0x50);
	checksum_add(GET_FLAGS(output_handler));

	checksum_add32(ip_addr);

	/* checksum source port */
	checksum_add16(UI16(source_port));
	
	/* checksum destination port */
	checksum_add16(UI16(port));

	checksum_add32(current_inseqno);

	/* HTTP contents checksum part */
	switch(handler_type) {
		case type_generator:
			/* add service checksum */
			checksum_add16(UI16(curr_output.checksum));
			/* add HTTP header checksum */
			switch(curr_output.service_header) {
				uint16_t length;
				int16_t i;
				case header_standard:					
					checksum_add16(SERVICE_HTTP_HEADER_CHK);
					/* create the HTTP Content-Length string on a even number of chars and start computing service checksum */
					checksum_add(0); /* odd bytes alignement */
					length = curr_output.content_length;
					for(i = CONTENT_LENGTH_SIZE - 1; i >= 0; i--) {
						unsigned char c = (length % 10) + '0';
						content_length_buffer[i]= c;
						checksum_add(c);
						length /= 10;
					}
					checksum_add(0); /* remove odd bytes alignement */
					break;
				case header_chunks:
					checksum_add16(SERVICE_HTTP_HEADER_CHUNKED_CHK);
				case header_none:
					checksum_add(0); /* odd bytes alignement */
					length = curr_output.content_length;
					for(i = CHUNK_LENGTH_SIZE - 1; i >= 0; i--) {
						unsigned char c = (length & 0x0f) + '0';
						if(c > '9')
							c += -'0' + 'a' - 10;
						content_length_buffer[i]= c;
						checksum_add(c);
						length >>= 4;
					}
					checksum_add(0); /* remove odd bytes alignement */
					checksum_add16(0x0d0a);
					if(curr_output.content_length % 2) {
						checksum_add16(0x0a0d);
					} else {
						checksum_add16(0x0d0a);
					}
					break;
			}
			break;
		case type_control:
			if(GET_FLAGS(output_handler) & TCP_SYN){ /* Checksum the syn ack tcp options (MSS) */
				checksum_add16(MSS_OPT);
				checksum_add16((uint16_t)connection->tcp_mss);
			}
			break;
		case type_file: {
#ifndef DISABLE_TLS
			if(connection->tls_active == 1){
				uint16_t i;
				uint16_t data_length = segment_length - TLS_OVERHEAD;
				const char *tmpptr = (const char*)(CONST_ADDR(GET_FILE(output_handler).data) + index_in_file);
				tls_record = mem_alloc(data_length);
				tls_record = CONST_READ_NBYTES(tls_record,tmpptr,data_length);

				//printf("Sending %d from a total of %d static file\n",data_length,(output_handler->handler_contents).file.length);
				/*printf("\n\nTCP data before sending (part of file contents) (%d bytes) :",data_length);
				for (i = 0 ; i < data_length; i++){
					printf("%02x", tls_record[i]);
				}
				printf("\n");*/

				/* preparing the HMAC hash for calculation */
				hmac_init(SHA1,connection->tls->server_mac,SHA1_KEYSIZE);
				hmac_preamble(connection->tls, data_length, ENCODE);

				/* checksuming TLS Record header */
				checksum_add(TLS_CONTENT_TYPE_APPLICATION_DATA);
				checksum_add(TLS_SUPPORTED_MAJOR);
				checksum_add(TLS_SUPPORTED_MINOR);
				checksum_add((data_length + MAC_KEYSIZE) >> 8);
				checksum_add((uint8_t)(data_length + MAC_KEYSIZE));

				/* Hash, encrypt and checksum data */
				for(i = 0; i < data_length; i++){
					//diferenta s-ar putea sa fie la marimea cat fac hmac
					hmac_update(tls_record[i]);
					rc4_crypt(&tls_record[i],MODE_ENCRYPT);
					checksum_add(tls_record[i]);

				}
				hmac_finish(SHA1);

				/* checksuming MAC */
				for(i = 0; i < MAC_KEYSIZE; i++){
					rc4_crypt(&sha1.buffer[i],MODE_ENCRYPT);
					checksum_add(sha1.buffer[i]);
				}


			} else
#endif
			{
				uint16_t i;
				uint32_t tmp_sum = 0;
				uint16_t *tmpptr = (uint16_t *)CONST_ADDR(GET_FILE(output_handler).chk) + DIV_BY_CHUNCKS_SIZE(index_in_file);

				for(i = 0; i < GET_NB_BLOCKS(segment_length); i++) {
					tmp_sum += CONST_READ_UI16(tmpptr++);
				}
				checksum_add32((const unsigned char*)&tmp_sum);
				break;
			}
		}
#ifndef DISABLE_TLS
		case type_tls_handshake:

			switch(connection->tls->tls_state) {
				uint16_t i;
				case server_hello:

					init_rand(0xABCDEF12); /* TODO move random init somewhere else */
					rand_next(connection->tls->server_random.lfsr_int);

					/* checksumming handshake records
					 * TODO this should be precalculated */
					for(i = 0; i < TLS_HELLO_CERT_DONE_LEN - 32; i++) {
						checksum_add(s_hello_cert_done[i]);
					}

					/* checksumming random value */
					for(i = 0; i < 32 ; i++){
						checksum_add(connection->tls->server_random.lfsr_char[i]);
					}


					break;

				case ccs_fin_send:
					/* calculating checksum for CCS */
					for(i = 0; i < TLS_CHANGE_CIPHER_SPEC_LEN; i++) {
						checksum_add(tls_ccs_msg[i]);
					}
					/* calculating checksum for Finished message */
					record_buffer = mem_alloc(TLS_FINISHED_MSG_LEN + START_BUFFER);
					build_finished(connection->tls,record_buffer);
					checksum_add(TLS_CONTENT_TYPE_HANDSHAKE);
					checksum_add(TLS_SUPPORTED_MAJOR);
					checksum_add(TLS_SUPPORTED_MINOR);
					checksum_add(0);
					checksum_add(TLS_FINISHED_MSG_LEN);
					for(i = 0; i < TLS_FINISHED_MSG_LEN; i++)
						checksum_add(record_buffer[START_BUFFER + i]);

					break;



			}
			break;
#endif

	}
	
	checksum_end();

	/* send TCP checksum (complemented)*/
	DEV_PUT16_VAL(~UI16(current_checksum));

	/* send TCP urgent pointer */
	DEV_PUT16_VAL(TCP_URGP);

	/* start sending HTTP contents */
	switch(handler_type) {
		case type_generator:
			switch(curr_output.service_header) {
				case header_standard:
					DEV_PUTN_CONST(serviceHttpHeader, sizeof(serviceHttpHeader)-1);
					DEV_PUTN(content_length_buffer, CONTENT_LENGTH_SIZE);
					DEV_PUTN_CONST(serviceHttpHeaderPart2, sizeof(serviceHttpHeaderPart2)-1);
					break;
				case header_chunks:
					DEV_PUTN_CONST(serviceHttpHeaderChunked, sizeof(serviceHttpHeaderChunked)-1);
				case header_none:
					DEV_PUTN(content_length_buffer, CHUNK_LENGTH_SIZE);
					DEV_PUT16_VAL(0x0d0a);
					break;
			}
			DEV_PUTN(curr_output.buffer, curr_output.content_length);
			if(curr_output.service_header != header_standard) {
				DEV_PUT16_VAL(0x0d0a);
			}
			break;
		case type_control:
			if(GET_FLAGS(output_handler) & TCP_SYN) {
				/* Send the syn ack tcp options (MSS) */
				DEV_PUT16_VAL(MSS_OPT);
				DEV_PUT16_VAL((uint16_t)connection->tcp_mss);
			}
			break;
		case type_file: {
			/* Send the payload of the packet */
#ifndef DISABLE_TLS
			if(connection->tls_active == 1){

				uint16_t i;
				uint16_t record_len = segment_length - TLS_RECORD_HEADER_LEN;
				/* sending TLS Record Header */
				DEV_PUT(TLS_CONTENT_TYPE_APPLICATION_DATA);
				DEV_PUT(TLS_SUPPORTED_MAJOR);
				DEV_PUT(TLS_SUPPORTED_MINOR);
				DEV_PUT((record_len) >> 8);
				DEV_PUT((uint8_t)(record_len));

				/* sending TLS Payload */
				//printf("Sending TLS data to network (%d bytes)\n ",record_len);
				for(i = 0; i < (record_len - MAC_KEYSIZE); i++){
					DEV_PUT(tls_record[i]);
					//printf("%02x",tls_record[i]);
				}

				/* sending MAC */
				for(i = 0; i < MAC_KEYSIZE; i++){
					DEV_PUT(sha1.buffer[i]);
					//printf("%02x",sha1.buffer[i]);
				}
				//printf("\n");

				/* free payload allocated for bringing data from external memory */
				mem_free(tls_record, record_len - MAC_KEYSIZE);

				/* update number of record sent */
				connection->tls->encode_seq_no.long_int++;

			} else

#endif
			{
				const char *tmpptr = (const char*)(CONST_ADDR(GET_FILE(output_handler).data) + index_in_file);
				DEV_PUTN_CONST(tmpptr, segment_length);
				break;
			}
		}

#ifndef DISABLE_TLS
		case type_tls_handshake:

			switch(connection->tls->tls_state) {
				case server_hello:
					tls_send_hello_cert_done(connection->tls);
					connection->tls->tls_state = key_exchange;
					break;

				case ccs_fin_send:

					tls_send_change_cipher(connection->tls);
					tls_send_finished(record_buffer + START_BUFFER);

					connection->tls->tls_state = established;
					mem_free(record_buffer,TLS_FINISHED_MSG_LEN + START_BUFFER);
					break;
			}
#endif

	}
	
	/* update next sequence number and inflight segments */
	if(GET_FLAGS(output_handler) & TCP_SYN) {
		UI32(connection->next_outseqno)++;
	} else if(connection) {

		UI32(connection->next_outseqno) += segment_length;
#ifndef DISABLE_TLS
		//if(connection->tls_active == 1) UI32(connection->next_outseqno)+= MAC_KEYSIZE + TLS_RECORD_HEADER_LEN;
#endif

		UI16(connection->inflight) += segment_length;
#ifndef DISABLE_TLS
		//if(connection->tls_active == 1) UI16(connection->inflight) += MAC_KEYSIZE + TLS_RECORD_HEADER_LEN;
#endif

		if(handler_type == type_generator) {
			if(curr_output.service_header == header_standard || curr_output.content_length == 0) {
				/* set final_outseqno as soon as it is known */
				UI32(connection->final_outseqno) = UI32(connection->next_outseqno);
			}
		}
	}

	//if(UI32(connection->final_outseqno) == UI32(connection->next_outseqno))
		//printf("am terminat \n");

	if(handler_type==type_generator){
		static int cpt=0;
		if(++cpt==2)return;
	}

		DEV_OUTPUT_DONE;
}

/*-----------------------------------------------------------------------------------*/
static inline int32_t able_to_send(const struct http_connection *connection) {

	return something_to_send(connection)
		&& UI16(connection->inflight) + (uint16_t)connection->tcp_mss <= UI16(connection->cwnd);
}

/*-----------------------------------------------------------------------------------*/
char smews_send(void) {
	struct http_connection *connection = NULL;
#ifndef DISABLE_COMET
	const struct output_handler_t * /*CONST_VAR*/ old_output_handler = NULL;
#endif

	/* sending reset has the highest priority */
	if(UI16(rst_connection.port)) {

		smews_send_packet(NULL);
		UI16(rst_connection.port) = 0;
		return 1;
	}

	/* we first choose a valid connection */
#ifndef DISABLE_COMET
	FOR_EACH_CONN(conn, {

		if(able_to_send(conn) && conn->comet_passive == 0) {
			connection = conn;
			break;
		}
	})
#else
		FOR_EACH_CONN(conn, {
		if(able_to_send(conn)) {
			connection = conn;
			break;
		}
	})
#endif

	if(connection == NULL) {
		return 0;
	}
	/* enable a round robin */
	all_connections = connection->next;
	
#ifndef DISABLE_COMET
	/* do we need to acknowledge a comet request without answering it? */
	if(connection->comet_send_ack == 1) {
		old_output_handler = connection->output_handler;
		connection->output_handler = &ref_ack;
	}
#endif

	/* get the type of contents */
	switch(CONST_UI8(connection->output_handler->handler_type)) {
		case type_control:
			/* preparing to send TCP control data */
			smews_send_packet(connection);
			connection->output_handler = NULL;
			if(connection->tcp_state == tcp_closing) {
				free_connection(connection);
			}
#ifndef DISABLE_COMET
			if(old_output_handler) {
				/* restore the right old output handler */
				connection->output_handler = old_output_handler;
				connection->comet_send_ack = 0;
				UI32(connection->final_outseqno) = UI32(connection->next_outseqno);
				/* call initget (which must not generate any content) */
				HANDLER_CALLBACK(connection,initget);
			}
#endif
			break;
		case type_file:

			smews_send_packet(connection);
			//connection->output_handler = NULL;
			break;
#ifndef DISABLE_TLS
		case type_tls_handshake:
			smews_send_packet(connection);
			connection->output_handler = NULL;
			break;
#endif
		case type_generator: {
			char is_persistent;
			char is_retransmitting;
			char has_ended;
			struct in_flight_infos_t *if_infos = NULL;

			/* creation and initialization of the generator_service if needed */
			if(connection->generator_service == NULL) {
				connection->generator_service = mem_alloc(sizeof(struct generator_service_t)); /* test NULL: done */
				if(connection->generator_service == NULL) {
					return 1;
				}
				curr_output.service = connection->generator_service;
				/* init generator service */
				curr_output.service->service_header = header_standard;
				curr_output.service->in_flight_infos = NULL;
				curr_output.service->is_persistent = CONST_UI8(GET_SERVICE(connection->output_handler).prop) == prop_persistent;
				UI32(curr_output.service->curr_outseqno) = UI32(connection->next_outseqno);
				/* init coroutine */
				cr_init(&curr_output.service->coroutine);
				curr_output.service->coroutine.func = CONST_ADDR(GET_SERVICE(connection->output_handler).doget);
#ifndef DISABLE_ARGS
				curr_output.service->coroutine.args = connection->args;
#endif
				/* we don't know yet the final output sequence number for this service */
				UI32(connection->final_outseqno) = UI32(connection->next_outseqno) - 1;
#ifndef DISABLE_COMET
				/* if this is a comet generator, manage all listenning clients */
				if(CONST_UI8(connection->output_handler->handler_comet)) {
					const struct output_handler_t *current_handler = connection->output_handler;
					/* if this is a comet handler, this connection is active, others are set as passive */
					FOR_EACH_CONN(conn, {
						if(conn->output_handler == current_handler) {
							conn->comet_passive = (conn != connection);
						}
					})
				}
				/* manage streamed comet content */
				if(CONST_UI8(connection->output_handler->handler_stream)) {
					if(connection->comet_streaming) {
						curr_output.service->service_header = header_none;
					}
					connection->comet_streaming = 1;
				}
#endif
			}
			
			/* init the global curr_output structure (usefull for out_c) */
			curr_output.service = connection->generator_service;
			UI32(curr_output.next_outseqno) = UI32(connection->next_outseqno);
			
			/* is the service persistent or not? */
			is_persistent = curr_output.service->is_persistent;
			/* are we retransmitting a lost segment? */
			is_retransmitting = UI32(curr_output.next_outseqno) != UI32(curr_output.service->curr_outseqno);

			/* put the coroutine (little) stack in the shared (big) stack if needed.
			 * This step has to be done before before any context_restore or context_backup */
			if(cr_prepare(&curr_output.service->coroutine) == NULL) { /* test NULL: done */
				return 1;
			}

			/* check if the segment need to be generated or can be resent from a buffer */
			if(!is_persistent || !is_retransmitting) { /* segment generation is needed */

				/* setup the http header for this segment */
				if(!is_persistent && is_retransmitting) {
					/* is the current context is not the right one, restore it */
					if_infos = context_restore(curr_output.service, connection->next_outseqno);
					/* if_infos is NULL if the segment to be sent is the last void http chunck */
					if(if_infos != NULL) {
						curr_output.service_header = if_infos->service_header;
					} else {
						curr_output.service_header = header_none;
					}
				} else {
					curr_output.service_header = curr_output.service->service_header;
				}
				
				/* initializations before generating the segment */
				curr_output.content_length = 0;
				checksum_init();
				curr_output.buffer = NULL;
	
				has_ended = curr_output.service->coroutine.curr_context.status == cr_terminated;
				/* is has_ended is true, the segment is a void chunk: no coroutine call is needed.
				 * else, run the coroutine to generate one segment */
				if(!has_ended) {
					/* allocate buffer for data generation */
					curr_output.buffer = mem_alloc(OUTPUT_BUFFER_SIZE); /* test NULL: done */
					if(curr_output.buffer == NULL) {
						return 1;
					}

					/* we will generate a segment for the first time, so we will need to store new in-flight information */
					if(!is_retransmitting) {
						if_infos = mem_alloc(sizeof(struct in_flight_infos_t)); /* test NULL: done */
						if(if_infos == NULL) {
							mem_free(curr_output.buffer, OUTPUT_BUFFER_SIZE);
							return 1;
						}
					}
										
					/* we generate new non persistent data. backup the context before running it */
					if(!is_persistent && !is_retransmitting) {
						if_infos->service_header = curr_output.service_header;
						if(context_backup(curr_output.service, curr_output.next_outseqno, if_infos) == NULL) { /* test NULL: done */
							mem_free(curr_output.buffer, OUTPUT_BUFFER_SIZE);
							mem_free(if_infos, sizeof(struct in_flight_infos_t));
							return 1;
						}
					}

					/* run the coroutine (generate one segment) */
					cr_run(&curr_output.service->coroutine);
					has_ended = curr_output.service->coroutine.curr_context.status == cr_terminated;
					
					/* save the generated buffer after generation if persistent */
					if(is_persistent) {
						/* add it to the in-flight segments list */
						if_infos->service_header = curr_output.service_header;
						UI32(if_infos->next_outseqno) = UI32(connection->next_outseqno);
						if_infos->infos.buffer = curr_output.buffer;
						if_infos->next = curr_output.service->in_flight_infos;	
						curr_output.service->in_flight_infos = if_infos;
						
					}
				}
				
				/* finalizations after the segment generation */
				checksum_end();
				UI16(curr_output.checksum) = UI16(current_checksum);
				/* save the current checksum in the in flight segment infos */
				if(if_infos) {
					UI16(if_infos->checksum) = UI16(current_checksum);
				}
			} else { /* the segment has to be resent from a buffer: restore it */
				if_infos = if_select(curr_output.service, connection->next_outseqno);
				curr_output.buffer = if_infos->infos.buffer;
				curr_output.service_header = if_infos->service_header;
				UI16(curr_output.checksum) = UI16(if_infos->checksum);
			}
			
			/* select the right HTTP header */
			switch(curr_output.service_header) {
				case header_standard:
					if(!has_ended || CONST_UI8(connection->output_handler->handler_stream)) {
						curr_output.service_header = header_chunks;
					}
					break;
				case header_chunks:
					curr_output.service_header = header_none;
					break;
				default:
					break;
			}
			
			/* send the segment */
#ifndef DISABLE_COMET
			/* if this is a comet handler, send the segment to all listening clients */
			if(CONST_UI8(connection->output_handler->handler_comet)) {
				const struct output_handler_t *current_handler = connection->output_handler;
				FOR_EACH_CONN(conn, {
					if(conn->output_handler == current_handler && able_to_send(conn)) {
						if(!CONST_UI8(conn->output_handler->handler_stream)) {
							smews_send_packet(conn);
						} else {
							if(curr_output.content_length > 0) {
								smews_send_packet(conn);
							}
							if(has_ended) {
								UI32(conn->final_outseqno) = UI32(conn->next_outseqno);
							}
						}
					}
				})
			} else
#endif
			{
			/* simply send the segment */
			   smews_send_packet(connection);
			}

			/* free the tmp buffer used for non persistent data generation */
			if(!is_persistent) {
				mem_free(curr_output.buffer, OUTPUT_BUFFER_SIZE);
			}

			if(!is_retransmitting) {
				/* update next_outseqno for the current context */
				curr_output.service->service_header = curr_output.service_header;
				UI32(curr_output.service->curr_outseqno) = UI32(connection->next_outseqno);
			}
			
#ifndef DISABLE_COMET
			/* clean comet service here (this is quite dirty) */
			if(CONST_UI8(connection->output_handler->handler_stream) && has_ended && UI16(connection->inflight) == 0) {
				clean_service(connection->generator_service, NULL);
				mem_free(connection->generator_service, sizeof(struct generator_service_t));
				connection->generator_service = NULL;
			}
#endif
			break;
		}
	}
#ifdef SMEWS_ENDING
	SMEWS_ENDING;
#endif
	return 1;
}
