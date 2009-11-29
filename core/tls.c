
#include "tls.h"
#include "memory.h"
#include "input.h"
#include "checksum.h"
#include "record.h"
#include "rand.h"

/* current value of server random (for each connection a next sequence random is generated) */
union int_char server_random;




/* fetch & parse TLS client hello */
uint8_t tls_get_client_hello(struct tls_connection *tls){

	int16_t x = 0;
	int16_t i = 0, j = 0;
	uint16_t length;
	uint8_t tmp;
	
	if( !(length = read_header(TLS_CONTENT_TYPE_HANDSHAKE)) ){
		return HNDSK_ERR;
	}

	uint8_t *record_buffer = mem_alloc(length);


	/* -------- Handshake Record Parsing ------- */

	/* get handshake type , handshake length , TLS Max Version and client random*/
	DEV_GETN( record_buffer, 38 );
	if(record_buffer[0] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Bad Handshake Type. Expected Client Hello (0x01) \n");
#endif
		return HNDSK_ERR;
	}

	/* save the client hello to the end of the buffer */
	/* copy_bytes((record_buffer + 6),record_buffer, BUFFER_SIZE - RANDOM_SIZE, 32 ); */

	x+=38;

#ifdef DEBUG_DEEP
	DEBUG_MSG("INFO:tls_get_client_hello: Client Random Bytes : ");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( (record_buffer + 6)[i],"%02x");
	DEBUG_MSG("\n");
#endif	
	
	/* get session id length */
	DEV_GETC(record_buffer[x]); x++;
	/* TODO if session support is added then we must get additional 32 bytes here to save the session ID*/

	/* cipher suites length */
	DEV_GETC(record_buffer[x]);  x++;
	DEV_GETC(record_buffer[x]);  x++; /* cipher len never more than 255 */
	
	
#ifdef DEBUG
	DEBUG_MSG("INFO:tls_get_client_hello: Number of cipher suites proposed is ");
	DEBUG_VAR(record_buffer[x - 1]/2,"%d \n");
#endif

	/* get cipher suite bytes */
	DEV_GETN( (record_buffer + x), record_buffer[x - 1]);

#ifdef DEBUG_DEEP
	{

		DEBUG_MSG("INFO:tls_get_client_hello: Cipher suites : \n");
		for( i = x, j= 0 ; j < record_buffer[x - 1] ; j+=2, i+=2){
			DEBUG_VAR(record_buffer[i],"0x%02x");
			DEBUG_VAR(record_buffer[i + 1],"%02x | ");
		}
		DEBUG_MSG("\n");
	}
#endif

	/* check if client supports our only cipher suite */
	for( i = x, j = 0 ; j < record_buffer[x - 1]; j+=2, i+=2)
		/* found ciphersuite TLS_ECDH_ECDSA_WITH_RC4_SHA */
		if(record_buffer[i] == 0xc0 && record_buffer[i + 1] == 0x02) break;

	if(j == record_buffer[x - 1]){

#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Compatible cipher suite not found! \n");
#endif
		return HNDSK_ERR;
	}

	x+= record_buffer[x - 1];

	/* get compression length */
	DEV_GETC(record_buffer[x]);  x++;

	/*  get compression methods and check for null */
	DEV_GETN((record_buffer + x), record_buffer [x - 1]);
	for(i = x, j = 0; j < record_buffer [x - 1] ; j++, i++){
		if(record_buffer[i] == TLS_COMPRESSION_NULL) break;
	}

	if(j == record_buffer [x - 1]){
		 /* this is almost impossible to reach because every TLS SHOULD support null */
#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Client does not support compression method null\n");
#endif
		return HNDSK_ERR;
	}

	/* get extensions lenght */
	DEV_GETC(record_buffer[x]);  x++;
	DEV_GETC(record_buffer[x]);  x++; /* assume ext len never more than 255 */
	tmp = record_buffer[x - 1];

	/* parse extensions */
	i = 0;
	while(i < tmp){

		uint8_t j;

		DEV_GETC(record_buffer[x]); /* get extension type */
		DEV_GETC(record_buffer[x+1]);
		i+=2;
		/* check if is elliptic curves extension */
		if(record_buffer[x] == 0x00 && record_buffer[x + 1] == TLS_EXT_EC){

			x+=2;
			DEV_GETC(record_buffer[x]); /* get extension len */
			DEV_GETC(record_buffer[x+1]);
			i+=record_buffer[x + 1] + 2;
			x+=2;
			/* get all extensions*/
			DEV_GETN( (record_buffer + x), record_buffer[x - 1])

			/* check if ECC 256 is supported */
			for(j = 0; j < record_buffer[x - 1] ; j+=2)
				if(record_buffer[x + j] == 0x00 && record_buffer[x + j + 1] == TLS_EXTENSION_SECP256R1) break;



			if(j == record_buffer[x - 1]) {
#ifdef DEBUG
				DEBUG_MSG("FATAL:tls_get_client_hello: Unsupported elliptic curve extensions \n");
#endif
				return HNDSK_ERR;
			}

			x+= record_buffer [ x - 1 ] ;

			continue;


		} /* ecc extension */

		/* check if is point format extension */
		if(record_buffer[x] == 0x00 && record_buffer[x + 1] == TLS_EXT_POINT_FORMATS){

			x+=2;
			DEV_GETC(record_buffer[x]); /* get extension len */
			DEV_GETC(record_buffer[x + 1]);
			i+= record_buffer[x + 1] + 2;
			x+=2;

			/* get all point formats */
			DEV_GETN( (record_buffer + x), record_buffer[x - 1])
			for(j = 0; j < record_buffer[x - 1] ; j++)
				if(record_buffer[ x + j ] == TLS_COMPRESSION_NULL) break;

			if(j == record_buffer[x - 1]) {
#ifdef DEBUG
				DEBUG_MSG("FATAL:tls_get_client_hello: Unsupported point formats\n");
#endif
				return HNDSK_ERR;
			}


			x+= record_buffer [ x - 1 ] ;

			continue;
		} /* point format extension */

		x+=2;
		DEV_GETC(record_buffer[x]);     /* get extension type */
		DEV_GETC(record_buffer[x + 1]); /* get extension len */
		i+= record_buffer[x+1] + 2;
		x+=2;

		/* drop uninteresting extensions */
		DEV_GETN( (record_buffer + x), record_buffer[x - 1])

		x+= record_buffer[x - 1];

	}

	/* allocate memory for handshake hashes */
	tls->client_md5 = mem_alloc(sizeof(struct md5_context));
	tls->client_sha1 = mem_alloc(sizeof(struct sha1_context));

	/* update finished hash contexts */
	md5_update(tls->client_md5, record_buffer, x);
	sha1_update(tls->client_sha1, record_buffer, x);

	/* free the buffer used in this phase */
	mem_free(record_buffer,length);

	/* if everything was ok we set the connection parameters */
	/* TODO revise this */
	tls->ccs_recv = 0;
	tls->ccs_sent = 0;
	tls->encode_seq_no.long_int = 0;
	tls->decode_seq_no.long_int = 0;


	return HNDSK_OK;

}


/* send in one record 3 handshake messages : hello,certificate,hello done */
uint8_t tls_send_hello_cert_done(struct tls_connection *tls){

	uint16_t i;
	uint8_t *ptr;


	/*write_header(TLS_CONTENT_TYPE_HANDSHAKE,TLS_HELLO_CERT_DONE_LEN);*/


	init_rand(server_random.lfsr_int, 0xABCDEF12); /* TODO move random init somewhere else */
	rand_next(server_random.lfsr_int);


	for(i = 0; i < 32 ; i++){
		s_hello_cert_done[12+i] = server_random.lfsr_char[i];
	}

#ifdef DEBUG
	DEBUG_MSG("INFO:tls_send_hello_cert_done: Server Generated Random :");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( server_random.lfsr_char[i],"%02x");
	DEBUG_MSG("\n");
#endif

	/* skipping record header */
	ptr = s_hello_cert_done + 5;

	/* updating digest with this message */
	md5_update(tls->client_md5, ptr, TLS_HELLO_CERT_DONE_LEN);
	sha1_update(tls->client_sha1, ptr, TLS_HELLO_CERT_DONE_LEN);

	for(i = 0; i < TLS_HELLO_CERT_DONE_LEN ; i++){
		DEV_PUT(s_hello_cert_done[i]);
	}

#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_send_server_hello: Server Hello, Certificate, Done sent");
#endif

	/* DEV_OUTPUT_DONE;*/

	return HNDSK_OK;


}
