
#include "tls.h"
#include "memory.h"
#include "input.h"
#include "checksum.h"
#include "record.h"

/* current value of server random (for each connection a next sequence random is generated) */
union int_char server_random;
extern CONST_VAR(uint8_t, server_cert[]);

static CONST_VAR(uint8_t,part1_srv_hello[6]) = { TLS_HANDSHAKE_TYPE_SERVER_HELLO, 0, 0, TLS_HELLO_RECORD_LEN - 4, TLS_SUPPORTED_MAJOR, TLS_SUPPORTED_MINOR };
static CONST_VAR(uint8_t,part2_srv_hello[4]) = { 0, TLS1_ECDH_ECDSA_WITH_RC4_128_SHA >> 8, (uint8_t)TLS1_ECDH_ECDSA_WITH_RC4_128_SHA, TLS_COMPRESSION_NULL};
static CONST_VAR(uint8_t,part3_srv_ext[8]) = { 0, TLS_EXT_LEN, 0, TLS_EXT_POINT_FORMATS, 0, 0x02, 0x01, TLS_COMPRESSION_NULL };
static CONST_VAR(uint8_t,srv_hello_done[4]) = { TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE, 0, 0, 0 };



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
#endif			return HNDSK_ERR;
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

	/* prepare output buffer - target dependent */
	 /* DEV_PREPARE_OUTPUT; */

	write_header(TLS_CONTENT_TYPE_HANDSHAKE,TLS_HELLO_RECORD_LEN + TLS_CERT_RECORD_LEN + TLS_HDONE_RECORD_LEN );

	/* --- beginning record filling ---*/

	for( i = 0 ; i < 6; i++)
		DEV_PUT(part1_srv_hello[i]);

	/* update handshake hash */
	md5_update(tls->client_md5, part1_srv_hello, 6);
	sha1_update(tls->client_sha1, part1_srv_hello, 6);

	init_rand(server_random.lfsr_int, 0xABCDEF12); /* TODO move random init somewhere else */
	rand_next(server_random.lfsr_int);

	for(i = 0; i < 32 ; i++){
		DEV_PUT(server_random.lfsr_char[i]);
	}

#ifdef DEBUG
	DEBUG_MSG("INFO:tls_send_hello_cert_done: Server Generated Random :");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( server_random.lfsr_char[i],"%02x");
	DEBUG_MSG("\n");
#endif

	/* updating digest with server random */
	md5_update(tls->client_md5, server_random.lfsr_char, 32);
	sha1_update(tls->client_sha1, server_random.lfsr_char, 32);

	/* session id length, ciphersuite, comp method */
	for(i = 0; i < 4 ; i++){
		DEV_PUT(part2_srv_hello[i]);
	}

	/* update handshake hash */
	md5_update(tls->client_md5, part2_srv_hello, 4);
	sha1_update(tls->client_sha1, part2_srv_hello, 4);


	/* Section 5.2 RFC 4492
	 * If no
	 * Supported Point Formats Extension is received with the ServerHello,this is equivalent
	 * to an extension allowing only the uncompressed point format.
	 */
	for(i = 0; i < 8 ; i++){
		DEV_PUT(part3_srv_ext[i]);
	}

	/* update handshake hash */
	md5_update(tls->client_md5, part3_srv_ext, 8);
	sha1_update(tls->client_sha1, part3_srv_ext, 8);


	for(i = 0 ; i < TLS_CERT_RECORD_LEN ; i++)
			DEV_PUT(server_cert[i]);

	md5_update(tls->client_md5, server_cert, TLS_CERT_RECORD_LEN);
	sha1_update(tls->client_sha1, server_cert,TLS_CERT_RECORD_LEN);

	/* write record body */
	for(i = 0; i < 4; i++)
		DEV_PUT(srv_hello_done[i]);

	/* update final handshake */
	md5_update(tls->client_md5,srv_hello_done,4);
	sha1_update(tls->client_sha1,srv_hello_done,4);


#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_send_server_hello: Server Hello, Certificate, Done sent");
#endif

	/* DEV_OUTPUT_DONE;*/

	return HNDSK_OK;


}
