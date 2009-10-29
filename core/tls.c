
#include "tls.h"
#include "checksum.h"


/* hash contexts for computation of handshake hashes required in finish message */
static struct md5_context client_md5;
static struct sha1_context client_sha1;

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

#define DEV_GETN(a,len){ \
	uint16_t i; \
	for(i = 0; i < len; i++) \
		DEV_GET(a[i]); \
}

/* reads a TLS record header and returns size of record data */
static uint16_t read_header(const uint8_t type){

	uint8_t tmp[2];

	/* get record type */
	DEV_GETC(tmp[0]);

	if(tmp[0] != type) {

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:read_header: Unexpected record type");
#endif
		return HNDSK_ERR;
	}

	/* get TLS Version */
	DEV_GETC16(tmp);

	if(tmp[1] != TLS_SUPPORTED_MAJOR && tmp[0] != TLS_SUPPORTED_MINOR){

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:read_header: Unsupported or damaged TLS Version");
#endif
		return HNDSK_ERR;
	}

	DEV_GETC16(tmp);

	return UI16(tmp);

}


/* fetch & parse TLS client hello */
uint8_t tls_get_client_hello(struct tls_connection *tls){

	int16_t x = 0;
	int16_t i = 0;
	uint16_t length;
	uint8_t tmp;
	
	if( (length = read_header(TLS_CONTENT_TYPE_HANDSHAKE)) ){
		return HNDSK_ERR;
	}

	uint8_t *record_buffer = mem_alloc(length);

	/* -------- Handshake Record Parsing ------- */

	/* get handshake type , handshake length , TLS Max Version and client random*/
	DEV_GETN( record_buffer, 38 );
	if(record_buffer[0] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Bad Handshake Type. Expected Client Hello (0x01)");
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
#endif	
	
	/* get session id length */
	DEV_GETC(record_buffer[x++]);
	/* TODO if session support is added then we must get additional 32 bytes here to save the session ID*/

	/* cipher suites length */
	DEV_GETC(record_buffer[x++]);
	DEV_GETC(record_buffer[x++]); /* cipher len never more than 255 */
	
	
#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_get_client_hello: Number of cipher suites proposed is ");
	DEBUG_VAR(record_buffer[x - 1]/2,"%d ");
#endif

	/* get cipher suite bytes */
	DEV_GETN( (record_buffer + x), record_buffer[x - 1]);

#ifdef DEBUG_DEEP
	DEBUG_MSG("\nINFO:tls_get_client_hello: Cipher suites : \n ");
	for( i = x ; i < record_buffer[x - 1] ; i+=2){
		DEBUG_VAR(record_buffer[i],"0x%02x");
		DEBUG_VAR(record_buffer[i + 1],"%02x | ");

	}
#endif

	/* check if client supports our only cipher suite */
	for( i = x ; i < record_buffer[x - 1]; i+=2)
		/* found ciphersuite TLS_ECDH_ECDSA_WITH_RC4_SHA */
		if(record_buffer[i] == 0xc0 && record_buffer[i + 1] == 0x02) break;

	if(i == record_buffer[x - 1] - 1){

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_hello: Compatible cipher suite not found! \n");
#endif
		return HNDSK_ERR;
	}

	x+= record_buffer[x - 1];

	/* get compression length */
	DEV_GETC(record_buffer[x++]);

	/*  get compression methods and check for null */
	DEV_GETN((record_buffer + x), record_buffer [x - 1]);
	for(i = x; i < record_buffer [x - 1] ; i++){
		if(record_buffer[i] == TLS_COMPRESSION_NULL) break;
	}

	if(i == record_buffer [x - 1] - 1){
		 /* this is almost impossible to reach because every TLS SHOULD support null */
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_hello: Client does not support compression method null");
#endif
		return HNDSK_ERR;
	}

	/* get extensions lenght */
	DEV_GETC(record_buffer[x++]);
	DEV_GETC(record_buffer[x++]); /* assume ext len never more than 255 */
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



			if(j == record_buffer[x - 1] - 1) {
#ifdef DEBUG
				DEBUG_MSG("\nFATAL:tls_get_client_hello: Unsupported elliptic curve extensions");
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
				DEBUG_MSG("\nFATAL:tls_get_client_hello: Unsupported point formats\n");
#endif
				return HNDSK_ERR;
			}


			x+= record_buffer [ x - 1 ] ;

			continue;
		} /* point format extension */

		x+=2;
		DEV_GETC(record_buffer[x]); /* get extension len */
		DEV_GETC(record_buffer[x + 1]); /* get extension len */
		i+= record_buffer[x+1] + 2;
		x+=2;

		/* drop uninteresting extensions */
		DEV_GETN( (record_buffer + x), record_buffer[x - 1])

		x+= record_buffer[x - 1];

	}


	/* update finished hash contexts */
	md5_update(&client_md5, record_buffer, x);
	sha1_update(&client_sha1, record_buffer, x);


	/* if everything was ok we set the connection parameters */
	/* TODO revise this */
	tls.ccs_recv = 0;
	/* tls.ccs_sent = 0; */
	tls.encode_seq_no.long_int = 0;
	tls.decode_seq_no.long_int = 0;


	return HNDSK_OK;

}