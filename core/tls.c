
#include "tls.h"
#include "memory.h"
#include "input.h"
#include "checksum.h"
#include "record.h"
#include "random.h"
#include "prf.h"
#include "ecc.h"



/* labels for PRF seed */
static CONST_VAR(uint8_t,ms_label[]) /*13 bytes*/ = { 0x6d,0x61,0x73,0x74,0x65,0x72,0x20,0x73,0x65,0x63,0x72,0x65,0x74 };
static CONST_VAR(uint8_t,ke_label[]) /*13 bytes*/ = { 0x6b,0x65,0x79,0x20,0x65,0x78,0x70,0x61,0x6e,0x73,0x69,0x6f,0x6e };
static CONST_VAR(uint8_t,f_label[2][15]) = { {0x73,0x65,0x72,0x76,0x65,0x72,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64},
											 {0x63,0x6c,0x69,0x65,0x6e,0x74,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64} };


/*TODO revise functions for stack optimization maybe convert to macro */
void inline copy_bytes(const uint8_t* src, uint8_t* dest,uint16_t start,uint16_t len){

	int16_t i;

	for(i = 0 ; i < len ; i++)
		dest[start++] = src[i];

}

/* fetch & parse TLS client hello */
uint8_t tls_get_client_hello(struct tls_connection *tls){

	int16_t x = 0;
	int16_t i = 0, j = 0;
	uint16_t length;
	uint8_t tmp;
	
	if( !(length = read_header(TLS_CONTENT_TYPE_HANDSHAKE)) ){
		return HNDSK_ERR;
	}

	// TODO discutabil daca sa-l aloc dinamic sau static
	uint8_t *record_buffer = mem_alloc(length);


	/* Handshake Record Parsing */

	/* handshake type, handshake length, TLS Max Version and client random */
	DEV_GETN( record_buffer, 38 );
	if(record_buffer[0] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_client_hello: Bad Handshake Type. Expected Client Hello (0x01)");
#endif
		return HNDSK_ERR;
	}

	/* save the client random to in the TLS connection */
	tls->client_random = mem_alloc(32);
	copy_bytes((record_buffer + 6),tls->client_random, 0, 32 );


	x+=38;

#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY(tls->client_random,32,"INFO:tls_get_client_hello: Client Random: ");
#endif	
	
	/* get session id length */
	DEV_GETC(record_buffer[x]); x++;
	/* TODO if session support is added then we must get additional 32 bytes here to save the session ID*/

	/* cipher suites length */
	DEV_GETC(record_buffer[x]);  x++;
	DEV_GETC(record_buffer[x]);  x++; /* cipher len never more than 255 */
	
	
#ifdef DEBUG_TLS_DEEP
	DEBUG_VAR(record_buffer[x - 1]/2,"%d","INFO:tls_get_client_hello: Number of cipher suites proposed is: ");
#endif

	/* get cipher suite bytes */
	DEV_GETN( (record_buffer + x), record_buffer[x - 1]);

#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY((record_buffer + x),record_buffer[x - 1],"INFO:tls_get_client_hello: Cipher suites: ");
#endif

	/* check if client supports our only cipher suite */
	for( i = x, j = 0 ; j < record_buffer[x - 1]; j+=2, i+=2)
		/* found ciphersuite TLS_ECDH_ECDSA_WITH_RC4_SHA */
		if(record_buffer[i] == 0xc0 && record_buffer[i + 1] == 0x02) break;

	if(j == record_buffer[x - 1]){

#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_client_hello: Compatible cipher suite not found!");
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
#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_client_hello: Client does not support compression method null\n");
#endif
		return HNDSK_ERR;
	}
	x+= record_buffer[x - 1];

	/* get extensions length */
	DEV_GETC(record_buffer[x]);  x++;
	DEV_GETC(record_buffer[x]);  x++; /* assumming ext len never more than 255 */
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
#ifdef DEBUG_TLS
				DEBUG_MSG("FATAL:tls_get_client_hello: Unsupported elliptic curve extensions");
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
#ifdef DEBUG_TLS
				DEBUG_MSG("FATAL:tls_get_client_hello: Unsupported point formats");
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
	md5_init(tls->client_md5);
	sha1_init(tls->client_sha1);

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

#ifdef DEBUG_TLS
	DEBUG_MSG("\n >>>>>>>>>>> CLIENT HELLO END >>>>>>>>>>>\n");
#endif

	return HNDSK_OK;

}


/* send in one record 3 handshake messages : hello,certificate,hello done */
uint8_t tls_send_hello_cert_done(struct tls_connection *tls){

	uint16_t i;

	/* TODO ? */
	uint8_t *tls_record = mem_alloc(TLS_HELLO_CERT_DONE_LEN - 32);

	CONST_READ_NBYTES(tls_record,s_hello_cert_done,TLS_HELLO_CERT_DONE_LEN - 32);

	/* updating digest with the handshake messages (skipping record header) */
	md5_update(tls->client_md5, tls_record + 5, 6);
	sha1_update(tls->client_sha1, tls_record + 5, 6);

	/* putting server generated random */
	md5_update(tls->client_md5, tls->server_random.lfsr_char, 32);
	sha1_update(tls->client_sha1, tls->server_random.lfsr_char, 32);

	md5_update(tls->client_md5, tls_record + 11, TLS_HELLO_CERT_DONE_LEN - 11 - 32);
	sha1_update(tls->client_sha1, tls_record + 11, TLS_HELLO_CERT_DONE_LEN - 11 - 32);


	for(i = 0; i < 11; i++)
		DEV_PUT(tls_record[i]);

	for(i = 0; i < 32 ; i++)
		DEV_PUT(tls->server_random.lfsr_char[i]);

	for(i = 11; i < TLS_HELLO_CERT_DONE_LEN - 32; i++)
		DEV_PUT(tls_record[i]);


	/* change TLS state to the next valid state */
	tls->tls_state = key_exchange;

	return HNDSK_OK;


}

/* this function will parse a key exchange in an ECDH client key exchange algorithm with 256 bits key size */
uint8_t tls_get_client_keyexch(struct tls_connection *tls){

	uint8_t i = 0,j = 0;
	uint8_t tmp_char;

	/* 77 = 70 (keyexchange message) + 7 (to accomodate seed length that will reuse the existing buffer) */
	uint8_t record_buffer[TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN + 7];

	/* TODO, this can be optimized to share stack between key_block and pms_secret because they are in mutual exclusion */
	uint8_t pms_secret[PMS_LEN];
	uint8_t key_block[KEY_MATERIAL_LEN];


	if( read_header(TLS_CONTENT_TYPE_HANDSHAKE) != TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN){
		return HNDSK_ERR;
	}


	/* the record len should always be 70 for this key exchange */
	DEV_GETN( record_buffer, TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN); /* fetch full message */

	if(record_buffer[0] != 16) {
#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_client_keyexch: Bad Handshake Type. Expected Client Key Exchange(0x10)");
#endif
		return HNDSK_ERR;
	}

	/* this message is always 66 bytes long for ECC-256 ECDH */
	if(record_buffer[3] != 66){
#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_client_keyexch: Unsupported Point format or point of unusual size");
#endif
		return HNDSK_ERR;
	}

	md5_update(tls->client_md5, record_buffer,70);
	sha1_update(tls->client_sha1, record_buffer,70);


#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY((record_buffer + 6),32,"INFO:tls_get_client_keyexch: Client Public Key (X Coordinate): ");
	PRINT_ARRAY((record_buffer + 6 + 32),32,"INFO:tls_get_client_keyexch: Client Public Key (Y Coordinate): ");
#endif

	/* converting to little endian in place */
	for(j = 0; j < 32; j++ ){

		tmp_char = record_buffer[6 + j];
		record_buffer[6 + j] = record_buffer[6 + 63 - j];
		record_buffer[6 + 63 - j] = tmp_char;
	}


	/* calculating premaster secret */
	gfp_point_mult((uint32_t*)ec_priv_key_256,(uint32_t*)(record_buffer + 6 + 32),(uint32_t*)(record_buffer + 6),(uint32_t*)pms_secret);


	/* creating byte_seed(label + crand + srand) for Master Generation  */
	/* TODO revise this */
	copy_bytes(ms_label,record_buffer,0,PRF_LABEL_SIZE); 			 		 /* copy the salt "master secret" */
	copy_bytes(tls->client_random,record_buffer,PRF_LABEL_SIZE,32); 		 /* copy the client random */
	copy_bytes(tls->server_random.lfsr_char,record_buffer,45,32); 		 	 /* copy the server random */

	/* changing PMS endianess because PRF needs it in big-endian */
	for(i = 0, j = PMS_LEN - 1 ; i < PMS_LEN/2 ; i++ , j--){
			tmp_char = pms_secret[j];
			pms_secret[j] = pms_secret[i];
			pms_secret[i] = tmp_char;
	}


	/* call PRF to generate master secret */
	prf(record_buffer,SEED_LEN,pms_secret,PMS_LEN,tls->master_secret,MS_LEN);

	/* reconstruct byte seed */
	copy_bytes(ke_label,record_buffer,0,PRF_LABEL_SIZE);
	copy_bytes(tls->server_random.lfsr_char,record_buffer,13,32); 		 	 /* copy the server random */
	copy_bytes(tls->client_random,record_buffer,45,32); 		 			 /* copy the client random */


	/* generate KEY_MATERIAL_LEN key material for connection keys*/
	prf(record_buffer,SEED_LEN,tls->master_secret,MS_LEN,key_block,KEY_MATERIAL_LEN);

	/* save session keys in tls connection */
	copy_bytes(key_block,tls->client_mac,0,MAC_KEYSIZE);
	copy_bytes(key_block + MAC_KEYSIZE,tls->server_mac,0,MAC_KEYSIZE);

	/* symmetric ciper keys are not saved for RC4 TODO (?)*/
	//copy_bytes(key_block + (MAC_KEYSIZE<<1),tls->client_key,0,CIPHER_KEYSIZE);
	//copy_bytes(key_block + (MAC_KEYSIZE<<1) + (CIPHER_KEYSIZE),tls->server_key,0,CIPHER_KEYSIZE);

	/* initialization of encode/decode ciphers */
	rc4_init(key_block + 2*MAC_KEYSIZE, MODE_DECRYPT);
	rc4_init(key_block + 2*MAC_KEYSIZE + CIPHER_KEYSIZE, MODE_ENCRYPT);

#ifdef DEBUG_TLS_DEEP
	DEBUG_MSG("************************ SESSION KEYS BEGIN *****************************************");
	PRINT_ARRAY(tls->master_secret,MS_LEN,"Master Secret :");
	PRINT_ARRAY(tls->client_mac,MAC_KEYSIZE,"Client MAC Secret :");
	PRINT_ARRAY(tls->server_mac,MAC_KEYSIZE,"Server MAC Secret :");
	PRINT_ARRAY( (key_block + 2*MAC_KEYSIZE),CIPHER_KEYSIZE,"Client Key Secret :");
	PRINT_ARRAY( (key_block + 2*MAC_KEYSIZE + CIPHER_KEYSIZE),CIPHER_KEYSIZE,"Server Key Secret :");
	DEBUG_MSG("************************** SESSION KEYS END *******************************************\n");
#endif

	//TODO free client_random and server_random memory; don't need them anymore
	//mem_free(tls->client_random, 32);

#ifdef DEBUG_TLS
	DEBUG_MSG("\n >>>>>>>>>>> CLIENT KEY EXCHANGE END >>>>>>>>>>>\n");
#endif

	return HNDSK_OK;

}

/* get client change cipher spec which is exactly 6 bytes */
uint8_t tls_get_change_cipher(struct tls_connection *tls){

	uint8_t tmp_char;

	if( read_header(TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC) != 1){
			return HNDSK_ERR;
	}

	DEV_GETC(tmp_char);

	if(tmp_char != 1){

#ifdef DEBUG_TLS
		DEBUG_MSG("INFO:tls_get_change_cipher: Damaged Change Cipher Spec message");
#endif
		return HNDSK_ERR;
	}

	tls->ccs_recv = 1;

	return HNDSK_OK;

}

uint8_t tls_send_change_cipher(struct tls_connection *tls){


	uint8_t i;

	for(i = 0; i < TLS_CHANGE_CIPHER_SPEC_LEN; i++){
		DEV_PUT(tls_ccs_msg[i]);
	}


	tls->ccs_sent = 1;

	return HNDSK_OK;

}


static void compute_finished(struct tls_connection *tls, struct md5_context* md5, struct sha1_context *sha1,uint8_t role, uint8_t *r){

	/* seed for finished message is : "client finished" || "server finished" | MD5 Hash | SHA1 Hash */
	uint8_t byte_seed[51];

	md5_digest(md5);
	sha1_digest(sha1);

	/* creating seed */

	copy_bytes(f_label[role],byte_seed,0,15);
	copy_bytes(md5->buffer,byte_seed,15,MD5_KEYSIZE);
	copy_bytes(sha1->buffer,byte_seed,31,SHA1_KEYSIZE);

	/* generate finished message payload*/
	prf(byte_seed,51,tls->master_secret,MS_LEN,r,12);


}

uint8_t tls_get_finished(struct tls_connection *tls){


	uint8_t expected_finished[12];
	uint8_t record_buffer[TLS_FINISHED_MSG_LEN + START_BUFFER];
	uint8_t *start_buffer = (record_buffer + START_BUFFER);

	/* used for saving hash contexts */
	struct md5_context temp_md5;
	struct sha1_context temp_sha1;


	if( read_header(TLS_CONTENT_TYPE_HANDSHAKE) != TLS_FINISHED_MSG_LEN ){
		return HNDSK_ERR;
	}


	DEV_GETN( start_buffer, TLS_FINISHED_MSG_LEN);

	/* if CCS was received that means the read state changed */
	/* this is the first message encoded with the cipher suite just negotiated */
	if(tls->ccs_recv == 1){

		if(!decode_record(tls, TLS_CONTENT_TYPE_HANDSHAKE, record_buffer, TLS_FINISHED_MSG_LEN)){
			return HNDSK_ERR;
		}

		if( start_buffer[0] != TLS_HANDSHAKE_TYPE_FINISHED){
#ifdef DEBUG_TLS
			DEBUG_MSG("FATAL:tls_get_finished: Bad Record Data Type");
#endif
			return HNDSK_ERR;
		}

	} else {
		/* if we didn't received CCS  */
#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:tls_get_finished: Finished Message Received before Change Cipher Spec Message");
		return HNDSK_ERR;
#endif

	}

	/* cloning hash contexts to continue calculation after */
	md5_clone(tls->client_md5,&temp_md5);
	sha1_clone(tls->client_sha1,&temp_sha1);

	/* compute finished message for the client */
	compute_finished(tls, tls->client_md5, tls->client_sha1, CLIENT, expected_finished);

	/* restore contexts */
	md5_clone(&temp_md5,tls->client_md5);
	sha1_clone(&temp_sha1,tls->client_sha1);

	{
		uint8_t i;
		for(i = 0 ; i < 12 ; i++)
			if(expected_finished[i] != start_buffer[4 + i]) {
#ifdef DEBUG_TLS_DEEP
			DEBUG_MSG("FATAL:tls_get_finished: Client Finished Message does not match local computation: ");
			PRINT_ARRAY(expected_finished, 12, "Local Calculated Finish Message: ");
			PRINT_ARRAY( (start_buffer + 4), 12, "Client Calculated Finish Message: ");
#endif
			return HNDSK_ERR;
			}
	}


	/* last update of hash message */
	md5_update(tls->client_md5,start_buffer,16);
	sha1_update(tls->client_sha1,start_buffer,16);

#ifdef DEBUG_TLS
	DEBUG_MSG("\n >>>>>>>>>>> CLIENT FINISHED MESSAGE END >>>>>>>>>>>\n");
#endif

	return HNDSK_OK;


}


uint8_t build_finished(struct tls_connection *tls, uint8_t *record_buffer){


	uint8_t *startbuffer = (record_buffer + START_BUFFER);

	/* fill in handshake header */
	startbuffer[0] = TLS_HANDSHAKE_TYPE_FINISHED;
	startbuffer[1] = 0;
	startbuffer[2] = 0;
	startbuffer[3] = 12;  //size of finished message

	//computing finished directly in the record_buffer
	compute_finished(tls, tls->client_md5, tls->client_sha1, SERVER, startbuffer + 4);


#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY( (startbuffer + 4), 12,"INFO: Calculated SERVER Finished Message : ");
#endif

	write_record(tls, TLS_CONTENT_TYPE_HANDSHAKE, record_buffer, 16);

	return HNDSK_OK;

}

uint8_t tls_send_finished(uint8_t *record_buffer){

	uint8_t i;
	write_header(TLS_CONTENT_TYPE_HANDSHAKE, TLS_FINISHED_MSG_LEN);

	for(i = 0; i < TLS_FINISHED_MSG_LEN ; i++)
		DEV_PUT(record_buffer[i]);

#ifdef DEBUG_TLS
	DEBUG_MSG("\n >>>>>>>>>>> SERVER FINISHED MESSAGE END >>>>>>>>>>>\n");
#endif
	return HNDSK_OK;

}


void dump_tls_conn(struct tls_connection *tls){

	printf("================= DUMPING TLS CONNECTION =================");
	PRINT_ARRAY(tls->client_mac,20,"Client MAC ");
	PRINT_ARRAY(tls->server_mac,20,"Server MAC ");
	//PRINT_ARRAY(tls->client_md5->buffer,64,"client md5 buffer ");
	//PRINT_ARRAY(tls->client_sha1->buffer,64,"client sha1 buffer ");
	PRINT_ARRAY(tls->decode_seq_no.bytes,8,"decode seq ");
	PRINT_ARRAY(tls->encode_seq_no.bytes,8,"encode seq ");
	DEBUG_VAR(tls->record_size,"%d","record size ");
	printf("ccs sent %c\n", tls->ccs_sent ? '1' : '0');
	printf("ccs recv %c\n", tls->ccs_recv ? '1' : '0');
	PRINT_ARRAY(tls->client_random,32,"client random ");
	PRINT_ARRAY(tls->server_random.lfsr_char,32,"server random ");
	printf("================= DUMPING TLS CONNECTION =================");


}

