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

#include "tls.h"
#include "hmac.h"

/* Used to dump the runtime stack */
#ifdef STACK_DUMP
	int16_t stack_i;
	unsigned char *stack_base;
#endif

/* "404 Not found" handler */
#define http_404_handler webContents_httpCodes_404_html_handler
extern CONST_VAR(struct output_handler_t, webContents_httpCodes_404_html_handler);

/* Maximal TCP MSS */
#ifndef DEV_MTU
	#define MAX_MSS 0xffff
#else
	#define MAX_MSS (DEV_MTU - 40)
#endif

/* IP and TCP constants */
#define HTTP_PORT 80
#define HTTPS_PORT 443
#define IP_PROTO_TCP 6
#define IP_HEADER_SIZE 20
#define TCP_HEADER_SIZE 20

/* tcp constant checksum part: ip_proto_tcp from pseudoheader*/
#define TCP_CHK_CONSTANT_PART (uint16_t)~IP_PROTO_TCP

/* Initial sequence number */
#define BASIC_SEQNO 0x42b7a491

/* Blob containing supported HTTP commands */
static CONST_VAR(unsigned char, blob_http_rqt[]) = {'G','E','T',' ',128};

/* gets 16 bits and checks if nothing wrong appened */
char dev_get16(unsigned char *word) {
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
char dev_get32(unsigned char *dword) {
	if(dev_get16(&dword[2]) == -1)
		return -1;
	if(dev_get16(&dword[0]) == -1)
		return -1;
	return 1;
}


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
	struct http_connection *connection;
	unsigned char tmp_char;
	uint16_t x;
	unsigned char new_tcp_data;
	
	/* variables used to defer processing with side effects until the incomming packet has been checked (with checksum) */
	unsigned char defer_clean_service = 0;
	unsigned char defer_free_handler = 0;
#ifndef DISABLE_ARGS
	struct args_t *defer_free_args;
	uint16_t defer_free_args_size;
#endif	

	/* tmp connection used to store the current state until checksums are checked */
	struct http_connection tmp_connection;

	if(!DEV_DATA_TO_READ)
		return 0;

	checksum_init();

	/* get IP version & header length */
	DEV_GETC(tmp_char);

	
#ifdef SMEWS_RECEIVING
	SMEWS_RECEIVING;
#endif
	/* Starting to decode IP */
	/* 0x45 : IPv4, no option in header */
	if(tmp_char != 0x45)
		return 1;

	/* discard IP type of service */
	DEV_GETC(tmp_char);

	/* get IP packet length in len */
	DEV_GETC16(((uint16_t *)&packet_length));

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
	/* get IP protocol, only TCP is supported */
	if(tmp_ui16[S1] != IP_PROTO_TCP)
		return 1;

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

	/* End of IP, starting TCP */
	checksum_init();

	/* get TCP source port */
	DEV_GETC16(tmp_connection.port);
		
	/* current connection selection */
	connection = NULL;
	/* search an existing TCP connection using the current port */
	FOR_EACH_CONN(conn, {
		if(UI32(conn->ip_addr) == UI32(tmp_connection.ip_addr) &&
			UI16(conn->port) == UI16(tmp_connection.port)) {
				/* connection already existing */
				connection = conn;
			break;
		}
	})

	if(connection) {
		/* a connection has been found */
		tmp_connection = *connection;
	} else {
		tmp_connection.tcp_state = tcp_listen;
		tmp_connection.output_handler = NULL;
		UI32(tmp_connection.next_outseqno) = BASIC_SEQNO;
		UI32(tmp_connection.current_inseqno) = 0;
		tmp_connection.parsing_state = parsing_out;
		UI16(tmp_connection.inflight) = 0;
		tmp_connection.generator_service = NULL;
#ifndef DISABLE_COMET
		tmp_connection.comet_send_ack = 0;
		tmp_connection.comet_passive = 0;
#endif
#ifndef DISABLE_TIMERS
		tmp_connection.transmission_time = last_transmission_time;
#endif

#ifndef DISABLE_TLS
		tmp_connection.tls_active = 0;
#endif

	}

	/* get and check the destination port */

	DEV_GETC16(tmp_ui16);


	if(tmp_ui16[S1] != HTTP_PORT) {
		/* check to see if it's TLS */
		if(UI16(tmp_ui16) == HTTPS_PORT){
			
			if(tmp_connection.tcp_state == tcp_listen) {

				/* if we expect a TLS connection allocate memory now */
				tmp_connection.tls = mem_alloc(sizeof(struct tls_connection));

				if(tmp_connection.tls != NULL){
					(tmp_connection.tls)->tls_state = client_hello; /* waiting for client hello */
					(tmp_connection.tls)->parsing_state = parsing_hdr;
					tmp_connection.tls_active = 1;

				} else {
					return 1;
				}
			}

		} else {
			/* neither 80 nor 443 */
#ifdef STACK_DUMP
			DEV_PREPARE_OUTPUT(STACK_DUMP_SIZE);
			for(stack_i = 0; stack_i < STACK_DUMP_SIZE ; stack_i++) {
				DEV_PUT(stack_base[-stack_i]);
			}
			DEV_OUTPUT_DONE;
#endif
			return 1;
		}

	}

	
	/* get TCP sequence number */
	DEV_GETC32(current_inseqno);

	/* get TCP ack */
	DEV_GETC32(current_inack);

	/* duplicate ACK: set nextoutseqno for retransmission */
	if(UI32(tmp_connection.next_outseqno) - UI16(tmp_connection.inflight) == UI32(current_inack)) {
		UI32(tmp_connection.next_outseqno) = UI32(current_inack);
	}

	/* TCP ack management */
	if(UI32(current_inack) && UI32(current_inack) <= UI32(tmp_connection.next_outseqno)) {
		UI16(tmp_connection.inflight) = UI32(tmp_connection.next_outseqno) - UI32(current_inack);
		if(tmp_connection.generator_service) {
			/* deferred because current segment has not yet been checked */
			defer_clean_service = 1;
		}
	}

	/* clear output_handler if needed */
	if(tmp_connection.output_handler && UI16(tmp_connection.inflight) == 0 && !something_to_send(&tmp_connection)) {
		if(tmp_connection.generator_service) {
			/* deferred because current segment has not yet been checked */
			defer_free_handler = 1;
#ifndef DISABLE_ARGS
			defer_free_args = tmp_connection.args;
			defer_free_args_size = CONST_UI16(tmp_connection.output_handler->handler_args.args_size);
#endif
		}
		tmp_connection.output_handler = NULL;
	}

	/* get TCP offset and flags */
	DEV_GETC16(tmp_ui16);
	tcp_header_length = (tmp_ui16[S0] >> 4) * 4;

	/* TCP segment length calculation */
	segment_length = packet_length - IP_HEADER_SIZE - tcp_header_length;

	/* calculation of the next sequence number we have to acknowledge */
	if(packet_length - IP_HEADER_SIZE - tcp_header_length > 0)
		UI32(current_inseqno) += segment_length;

	new_tcp_data = UI32(current_inseqno) > UI32(tmp_connection.current_inseqno);

	if(UI32(current_inseqno) >= UI32(tmp_connection.current_inseqno)) {
		UI32(tmp_connection.current_inseqno) = UI32(current_inseqno);
		/* TCP state machine management */
		switch(tmp_connection.tcp_state) {
			case tcp_established:
				if(tmp_ui16[S1] & TCP_FIN) {
					tmp_connection.tcp_state = tcp_last_ack;
					tmp_connection.output_handler = &ref_finack;
					UI32(tmp_connection.current_inseqno)++;
				} else if(tmp_ui16[S1] & TCP_RST) {
					tmp_connection.tcp_state = tcp_listen;
				}
				break;
			case tcp_listen:
				if(tmp_ui16[S1] & TCP_SYN) {
					tmp_connection.tcp_state = tcp_syn_rcvd;
					tmp_connection.output_handler = &ref_synack;
					UI32(tmp_connection.current_inseqno)++;
				}
				break;
			case tcp_syn_rcvd:
				if(UI16(tmp_connection.inflight) == 0) {
					tmp_connection.tcp_state = tcp_established;
				} else {
					tmp_connection.output_handler = &ref_synack;
				}
				break;
			case tcp_last_ack:
				tmp_connection.tcp_state = tcp_listen;
				break;
			default:
				break;
		}
	}

	/* get the advertissed TCP window in order to limit our sending rate if needed */
	DEV_GETC16(tmp_connection.cwnd);

	/* discard the checksum (which is checksummed as other data) and the urgent pointer */
	DEV_GETC32(tmp_ui32);

	/* add the changing part of the TCP pseudo header checksum */
	checksum_add32(local_ip_addr);
	checksum_add32(tmp_connection.ip_addr);
	checksum_add16(packet_length - IP_HEADER_SIZE);
	
	/* get TCP mss (for initial negociation) */
	tcp_header_length -= TCP_HEADER_SIZE;
	if(tcp_header_length >= 4) {
		tcp_header_length -= 4;
		DEV_GETC16(tmp_ui16);
		DEV_GETC16(tmp_ui16);
		tmp_connection.tcp_mss = UI16(tmp_ui16) > MAX_MSS ? MAX_MSS : UI16(tmp_ui16);
	}

	/* discard the remaining part of the TCP header */
	for(; tcp_header_length > 0; tcp_header_length-=4) {
		DEV_GETC32(tmp_ui32);
	}

	x = 0;

	/* TLS Handshake Layer processing*/
	if(segment_length && tmp_connection.tcp_state == tcp_established && tmp_connection.output_handler == NULL && tmp_connection.tls_active == 1 && (tmp_connection.tls)->tls_state != established ) {
		
		/* TLS state machine management*/
		switch(  (tmp_connection.tls)->tls_state ){

			case client_hello:

				if(tls_get_client_hello(tmp_connection.tls) == HNDSK_OK){
					(tmp_connection.tls)->tls_state = server_hello;
					tmp_connection.output_handler = &ref_tlshandshake;
				}
				x+= segment_length;
				if(segment_length == x )
					break;;


			case key_exchange:
				//printf("Waiting for key exchange \n");
				if(tls_get_client_keyexch(tmp_connection.tls) == HNDSK_OK){
					(tmp_connection.tls)->tls_state = ccs_recv;

				}
				x+= TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN + TLS_RECORD_HEADER_LEN;
				if(segment_length == x )
					break;

			case ccs_recv:
				//printf("Waiting for Change Cipher Spec\n");
				if(tls_get_change_cipher(tmp_connection.tls) == HNDSK_OK){
					(tmp_connection.tls)->tls_state = fin_recv;
				}
				x+= TLS_CHANGE_CIPHER_SPEC_LEN;
				if(segment_length == x )
					break;

			case fin_recv:
				printf("Waiting for Finished Message\n");
				if(tls_get_finished(tmp_connection.tls) == HNDSK_OK){
					(tmp_connection.tls)->tls_state = ccs_fin_send;
					tmp_connection.output_handler = &ref_tlshandshake;
				}
				x+= TLS_FINISHED_MSG_LEN + TLS_RECORD_HEADER_LEN;
				if(segment_length == x )
					break;


			default:
				  printf("Record not implemented\n");
			      break;	 	
    

		}


	} else {

		printf("TCP Segment Length: %d\n",segment_length);
		/* End of TCP, starting HTTP*/
		/* TLS record layer operation if TLS active */
		if(segment_length && tmp_connection.tcp_state == tcp_established && (new_tcp_data || tmp_connection.output_handler == NULL)) {
			const struct output_handler_t * /*CONST_VAR*/ output_handler = NULL;

			/* parse the eventual GET request */
			unsigned const char * /*CONST_VAR*/ blob;
			unsigned char blob_curr;
	#ifndef DISABLE_ARGS
			struct arg_ref_t tmp_arg_ref = {0,0,0};
	#endif

			if(tmp_connection.parsing_state == parsing_out) {
	#ifndef DISABLE_ARGS
				tmp_connection.args = NULL;
				tmp_connection.arg_ref_index = 128;
	#endif
				tmp_connection.blob = blob_http_rqt;
				tmp_connection.parsing_state = parsing_cmd;
			}

			blob = tmp_connection.blob;

	#ifndef DISABLE_ARGS
			if(tmp_connection.arg_ref_index != 128) {
				struct arg_ref_t * /*CONST_VAR*/ tmp_arg_ref_ptr;
				tmp_arg_ref_ptr = &(((struct arg_ref_t*)CONST_ADDR(output_handler->handler_args.args_index))[tmp_connection.arg_ref_index]);
				tmp_arg_ref.arg_type = CONST_UI8(tmp_arg_ref_ptr->arg_type);
				tmp_arg_ref.arg_size = CONST_UI8(tmp_arg_ref_ptr->arg_size);
				tmp_arg_ref.arg_offset = CONST_UI8(tmp_arg_ref_ptr->arg_offset);
			}
	#endif

			while(x < segment_length && output_handler != &http_404_handler) {

#ifndef DISABLE_TLS
				if(tmp_connection.tls_active == 1){

					if((tmp_connection.tls)->parsing_state == parsing_hdr){

						uint8_t i;
						x+=5;
						/* TODO parse ALERT TYPE */
						if( ((tmp_connection.tls)->record_size = read_header(TLS_CONTENT_TYPE_APPLICATION_DATA)) == HNDSK_ERR){
							break;
						}
						/* preparing the HMAC hash for calculation */
						hmac_init(SHA1,(tmp_connection.tls)->server_mac,SHA1_KEYSIZE);
						hmac_preamble(tmp_connection.tls);

						(tmp_connection.tls)->parsing_state = parsing_data;
						continue;
					}
				}
#endif
				DEV_GETC(tmp_char);
				x++;
				printf("E %02x",tmp_char);

#ifndef DISABLE_TLS
				if(tmp_connection.tls_active == 1){

					/* decrypt current character */
					rc4_crypt(&tmp_char,MODE_DECRYPT);
					printf("D %02x",tmp_char);

					/* updating remaining bytes to parse from payload of the current record */
					(tmp_connection.tls)->record_size--;

					if((tmp_connection.tls)->parsing_state == parsing_data){

						hmac_update(tmp_char);

						/* entering MAC portion */
						if((tmp_connection.tls)->record_size == MAC_KEYSIZE){
							(tmp_connection.tls)->parsing_state = parsing_mac;
							hmac_finish(SHA1);
						}


					} else if((tmp_connection.tls)->parsing_state == parsing_mac){

						if(sha1.buffer[MAC_KEYSIZE - (tmp_connection.tls)->record_size - 1] != tmp_char){
							tmp_connection.output_handler = NULL;
							printf("MAC is not good\n");
							break;
						} else {
							/* finished MAC parsing and checking */
							if((tmp_connection.tls)->record_size == 0){
								/* prepare header parsing for next record */
								(tmp_connection.tls)->parsing_state = parsing_hdr;
							}
						}

						continue;

					}
				}
#endif

				blob_curr = CONST_READ_UI8(blob);
				/* search for the content to send */
				if(blob_curr >= 128 && output_handler != &http_404_handler) {
					if(tmp_connection.parsing_state == parsing_cmd) {
						tmp_connection.parsing_state = parsing_url;
						blob = urls_tree;
					} else {
						if(tmp_char == ' ') {
							if(!output_handler)
								output_handler = (struct output_handler_t*)CONST_ADDR(files_index[blob_curr - 128]);
							break;
						} else {
	#ifndef DISABLE_ARGS
							if(tmp_char == '?') {
								uint16_t tmp_args_size;
								output_handler = (struct output_handler_t*)CONST_ADDR(files_index[blob_curr - 128]);
								tmp_args_size = CONST_UI16(output_handler->handler_args.args_size);
								if(tmp_args_size) {
									uint16_t i;
									blob = (const unsigned char *)CONST_ADDR(output_handler->handler_args.args_tree);
									tmp_connection.args = mem_alloc(tmp_args_size); /* test NULL: done */
									if(tmp_connection.args == NULL) {
										output_handler = &http_404_handler;
										break;
									}
									for(i = 0; i < tmp_args_size ; i++) {
										((unsigned char *)tmp_connection.args)[i] = 0;
									}
									continue;
								}
							} else if(tmp_char == '=' && tmp_connection.args) {
								struct arg_ref_t * /*CONST_VAR*/ tmp_arg_ref_ptr;
								tmp_connection.arg_ref_index = blob_curr - 128;
								tmp_arg_ref_ptr = &(((struct arg_ref_t*)CONST_ADDR(output_handler->handler_args.args_index))[tmp_connection.arg_ref_index]);
								tmp_arg_ref.arg_type = CONST_UI8(tmp_arg_ref_ptr->arg_type);
								tmp_arg_ref.arg_size = CONST_UI8(tmp_arg_ref_ptr->arg_size);
								tmp_arg_ref.arg_offset = CONST_UI8(tmp_arg_ref_ptr->arg_offset);
								tmp_connection.curr_arg = ((unsigned char*)tmp_connection.args) + tmp_arg_ref.arg_offset;
								if(tmp_arg_ref.arg_type == arg_str)
									(*((unsigned char*)tmp_connection.curr_arg + tmp_arg_ref.arg_size - 1)) = tmp_arg_ref.arg_size - 1;
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
	#ifndef DISABLE_ARGS
				if(tmp_connection.arg_ref_index != 128) {
					if(tmp_char == '&') {
						tmp_connection.arg_ref_index = 128;
						blob = (const unsigned char *)CONST_ADDR(output_handler->handler_args.args_tree);
						continue;
					} else if(tmp_char == ' ') {
						break;
					} else {
						switch(tmp_arg_ref.arg_type) {
							case arg_str: {
								unsigned char *tmp_size_ptr = ((unsigned char*)tmp_connection.curr_arg + tmp_arg_ref.arg_size - 1);
								if(*tmp_size_ptr) {
									*((unsigned char*)tmp_connection.curr_arg + (tmp_arg_ref.arg_size - *tmp_size_ptr - 1)) = tmp_char;
									(*tmp_size_ptr)--;
								}
								break;
							}
							case arg_ui8:
								*((unsigned char*)tmp_connection.curr_arg) *= 10;
								*((unsigned char*)tmp_connection.curr_arg) += tmp_char - '0';
								break;
							case arg_ui16:
								*((uint16_t*)tmp_connection.curr_arg) *= 10;
								*((uint16_t*)tmp_connection.curr_arg) += tmp_char - '0';
								break;
						}
					}
				} else
	#endif
				{
					do {
						unsigned char offsetInf = 0;
						unsigned char offsetEq = 0;
						unsigned char blob_next;
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
									output_handler = &http_404_handler;
									break;
								}
							}
							break;
						} else if (tmp_char < blob_curr) {
							if (blob_next < 32 && blob_next & 1) {
								blob += offsetInf;
							} else {
								output_handler = &http_404_handler;
								break;
							}
						} else {
							if (blob_next < 32 && blob_next & 4) {
								unsigned char offsetSup = offsetEq + ((blob_next & 3)?CONST_READ_UI8(blob+(offsetInf-1)):0);
								blob += offsetSup;
							} else {
								output_handler = &http_404_handler;
								break;
							}
						}
					} while(1);
				}
			}

			if(!output_handler) {
				tmp_connection.blob = blob;
			} else {
				if(tmp_connection.parsing_state != parsing_cmd) {
					tmp_connection.output_handler = output_handler;
					UI32(tmp_connection.next_outseqno) = UI32(current_inack);
					if(output_handler->handler_type == type_file) {
						UI32(tmp_connection.final_outseqno) = UI32(tmp_connection.next_outseqno) + CONST_UI32(GET_FILE(output_handler).length);
					} else {
						UI32(tmp_connection.final_outseqno) = UI32(tmp_connection.next_outseqno) - 1;
					}
	#ifndef DISABLE_COMET
					tmp_connection.comet_send_ack = CONST_UI8(output_handler->handler_comet) ? 1 : 0;
					tmp_connection.comet_passive = 0;
					tmp_connection.comet_streaming = 0;
	#endif
				}
				tmp_connection.parsing_state = parsing_out;
				tmp_connection.blob = blob_http_rqt;
	#ifndef DISABLE_ARGS
				tmp_connection.arg_ref_index = 128;
	#endif
			}
		}
	}

	printf("\nFinished getting TCP Segment\n\n");
	/* drop remaining TCP data */
	//printf("I parsed %d TCP payload\n",x);
	while(x++ < segment_length)
		DEV_GETC(tmp_char);
	//printf("%p %d %d %d\n",tmp_connection.output_handler,tmp_connection.tcp_state, segment_length,tcp_established);
	/* acknowledge received and processed TCP data if no there is no current output_handler */
	if(!tmp_connection.output_handler && tmp_connection.tcp_state == tcp_established && segment_length) {
		printf("Sending VOID ACK\n");
		tmp_connection.output_handler = &ref_ack;
	}

	/* check TCP checksum using the partially precalculated pseudo header checksum */
	checksum_end();
	//printf("Current calculated checksum is %04x\n",UI16(current_checksum));
	if(UI16(current_checksum) == TCP_CHK_CONSTANT_PART) {
		
		if(defer_clean_service) { /* free in-flight segment information for acknowledged segments */
			clean_service(tmp_connection.generator_service, current_inack);
			if(defer_free_handler) { /* free handler and generator service is the service is completely acknowledged */
				cr_clean(&tmp_connection.generator_service->coroutine);
				mem_free(tmp_connection.generator_service, sizeof(struct generator_service_t));
				tmp_connection.generator_service = NULL;
#ifndef DISABLE_ARGS
				mem_free(defer_free_args, defer_free_args_size);
#endif
			}
		}
		
		if(!connection && tmp_connection.tcp_state == tcp_syn_rcvd) {
			/* allocate a new connection */
			connection = mem_alloc(sizeof(struct http_connection)); /* test NULL: done */
			if(connection != NULL) {
				/* insert the new connection */
				if(all_connections == NULL) {
					tmp_connection.next = connection;
					tmp_connection.prev = connection;
					all_connections = connection;
				} else {
					tmp_connection.prev = all_connections->prev;
					tmp_connection.prev->next = connection;
					tmp_connection.next = all_connections;
					all_connections->prev = connection;
				}
			}
		}

		if(!connection) {
			/* no valid connection has been found for this packet, send a reset */
			UI32(tmp_connection.next_outseqno) = UI32(current_inack);
			UI32(rst_connection.ip_addr) = UI32(tmp_connection.ip_addr);
			UI16(rst_connection.port) = UI16(tmp_connection.port);
			UI32(rst_connection.current_inseqno) = UI32(tmp_connection.current_inseqno);
			UI32(rst_connection.next_outseqno) = UI32(tmp_connection.next_outseqno);
		} else {
			if(tmp_connection.tcp_state == tcp_listen) {
				free_connection(connection);
			} else {
				/* update the current connection */
				*connection = tmp_connection;
			}
		}
	} else {
		//todo erase this else
		printf("TCP Checksum not good\n");
	}

	return 1;
}
