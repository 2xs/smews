
#include "tls.h"
#include "memory.h"
#include "input.h"
#include "checksum.h"
#include "record.h"
#include "rand.h"
#include "prf.h"
#include "ecc.h"



/* labels for PRF seed */
static CONST_VAR(uint8_t,ms_label[]) /*13 bytes*/ = { 0x6d,0x61,0x73,0x74,0x65,0x72,0x20,0x73,0x65,0x63,0x72,0x65,0x74 };
static CONST_VAR(uint8_t,ke_label[]) /*13 bytes*/ = { 0x6b,0x65,0x79,0x20,0x65,0x78,0x70,0x61,0x6e,0x73,0x69,0x6f,0x6e };
static CONST_VAR(uint8_t,f_label[2][15]) = { {0x73,0x65,0x72,0x76,0x65,0x72,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64},
											 {0x63,0x6c,0x69,0x65,0x6e,0x74,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64} };


/*TODO revise functions for stack optimization maybe convert to macro */
void copy_bytes(const uint8_t* src, uint8_t* dest,uint16_t start,uint16_t len){

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


	/* -------- Handshake Record Parsing ------- */

	/* get handshake type , handshake length , TLS Max Version and client random*/
	DEV_GETN( record_buffer, 38 );
	if(record_buffer[0] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Bad Handshake Type. Expected Client Hello (0x01) \n");
#endif
		return HNDSK_ERR;
	}

	/* save the client random to in the tls connection */
	tls->client_random = mem_alloc(32);
	copy_bytes((record_buffer + 6),tls->client_random, 0, 32 );


	x+=38;

#ifdef DEBUG_DEEP
	DEBUG_MSG("INFO:tls_get_client_hello: Client Random Bytes : ");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( tls->client_random[i],"%02x");
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
	x+= record_buffer[x - 1];

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
	md5_init(tls->client_md5);
	sha1_init(tls->client_sha1);

	/* update finished hash contexts */
	md5_update(tls->client_md5, record_buffer, x);
	sha1_update(tls->client_sha1, record_buffer, x);

#ifdef DEBUG
	PRINT_ARRAY(record_buffer,length,"=========== Client Hello Data ============\n")
#endif
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

	/*write_header(TLS_CONTENT_TYPE_HANDSHAKE,TLS_HELLO_CERT_DONE_LEN);*/


	/* updating digest with this message (the whole message skipping record header) */
	md5_update(tls->client_md5, s_hello_cert_done + 5, TLS_HELLO_CERT_DONE_LEN - TLS_RECORD_HEADER_LEN);
	sha1_update(tls->client_sha1, s_hello_cert_done + 5, TLS_HELLO_CERT_DONE_LEN - TLS_RECORD_HEADER_LEN);

	for(i = 0; i < TLS_HELLO_CERT_DONE_LEN; i++){
		DEV_PUT(s_hello_cert_done[i]);
	}

#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_send_server_hello: Server Hello, Certificate, Done sent\n");
#endif

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


	/* where keyblock will be saved temporary */
	//uint8_t *key_block = record_buffer + SEED_LEN;

	/* set pointers in the buffer for the client random and pms */
	//uint8_t *client_random = record_buffer + BUFFER_SIZE - RANDOM_SIZE;


	if( read_header(TLS_CONTENT_TYPE_HANDSHAKE) != TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN){
		return HNDSK_ERR;
	}

	//record_buffer = mem_alloc(TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN + 7); /* 7 is to cover the seed length*/


	/* the record len should always be 70 for this key exchange */
	DEV_GETN( record_buffer, TLS1_ECDH_ECDSA_WITH_RC4_128_SHA_KEXCH_LEN); /* fetch full message */

	if(record_buffer[0] != 16) {
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_keyexch: Bad Handshake Type. Expected Client Key Exchange(0x10)\n");
#endif
		return HNDSK_ERR;
	}

	/* this message is always 66 bytes long for ECC-256 ECDH */
	if(record_buffer[3] != 66){
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_keyexch: Unsupported Point format or point of unusual size");
#endif
		return HNDSK_ERR;
	}

	md5_update(tls->client_md5, record_buffer,70);
	sha1_update(tls->client_sha1, record_buffer,70);


#ifdef DEBUG
	{
	uint8_t j;
	DEBUG_MSG("\nINFO:tls_get_client_keyexch: Client Public Key (X Coordinate) :");
	for(j = 0; j < 32; j++)
		DEBUG_VAR(record_buffer[6 + j],"%02x");
	DEBUG_MSG("\nINFO:tls_get_client_keyexch: Client Public Key (Y Coordinate) :");
	for(j = 0; j < 32; j++)
		DEBUG_VAR(record_buffer[6 + 32 + j],"%02x");
	DEBUG_MSG("\n");
	}
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

	/* change same byte_seed with new label and inverting randomness :) */
	copy_bytes(ke_label,record_buffer,0,PRF_LABEL_SIZE);
	copy_bytes(tls->server_random.lfsr_char,record_buffer,13,32); 		 	 /* copy the server random */
	copy_bytes(tls->client_random,record_buffer,45,32); 		 			 /* copy the client random */


	/* generate KEY_MATERIAL_LEN key material for connection keys*/
	prf(record_buffer,SEED_LEN,tls->master_secret,MS_LEN,key_block,KEY_MATERIAL_LEN);
	//PRINT_ARRAY(key_block,KEY_MATERIAL_LEN,"keyblock ; ");

	/* save session keys in tls connection */
	copy_bytes(key_block,tls->client_mac,0,MAC_KEYSIZE);
	copy_bytes(key_block + MAC_KEYSIZE,tls->server_mac,0,MAC_KEYSIZE);
	copy_bytes(key_block + (MAC_KEYSIZE<<1),tls->client_key,0,CIPHER_KEYSIZE);
	copy_bytes(key_block + (MAC_KEYSIZE<<1) + (CIPHER_KEYSIZE),tls->server_key,0,CIPHER_KEYSIZE);

	/* initialization of encode/decode ciphers */
	rc4_init(key_block + 2*MAC_KEYSIZE, MODE_DECRYPT);
	rc4_init(key_block + 2*MAC_KEYSIZE + CIPHER_KEYSIZE, MODE_ENCRYPT);

#ifdef DEBUG
	DEBUG_MSG("\n************************ SESSION KEYS BEGIN *****************************************\n");
	PRINT_ARRAY(tls->master_secret,MS_LEN,"Master Secret :");
	PRINT_ARRAY(tls->client_mac,MAC_KEYSIZE,"Client MAC Secret :");
	PRINT_ARRAY(tls->server_mac,MAC_KEYSIZE,"Server MAC Secret :");
	PRINT_ARRAY(tls->client_key,CIPHER_KEYSIZE,"Client Key Secret :");
	PRINT_ARRAY(tls->server_key,CIPHER_KEYSIZE,"Server Key Secret :");
	DEBUG_MSG("************************** SESSION KEYS END *******************************************\n");
#endif

	//TODO free client_random and server_random memory; don't need them anymore

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

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_get_change_cipher: Damaged Change Cipher Spec message\n");
#endif
		return HNDSK_ERR;
	}

	tls->ccs_recv = 1;

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_get_change_cipher: Change Cipher Spec Message received\n");
#endif

	return HNDSK_OK;

}

uint8_t tls_send_change_cipher(struct tls_connection *tls){



	//write_header(TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC,1);
	uint8_t i;

	for(i = 0; i < TLS_CHANGE_CIPHER_SPEC_LEN; i++){
		DEV_PUT(tls_ccs_msg[i]);
	}


	tls->ccs_sent = 1;

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_send_change_cipher: Change Cipher Spec Message Sent \n");
#endif

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


#ifdef DEBUG_DEEP
	DEBUG_MSG("\n******************** FINISHED CLIENT MESSAGE BEGIN *****************************\n");
#endif

	if( read_header(TLS_CONTENT_TYPE_HANDSHAKE) != TLS_FINISHED_MSG_LEN ){
		return HNDSK_ERR;
	}


	DEV_GETN( start_buffer, TLS_FINISHED_MSG_LEN);

	/* if CCS was received that means the read state changed */
	/* this is the first message encoded with the csuite just negotiated */
	if(tls->ccs_recv == 1){

		if(!decode_record(tls,TLS_CONTENT_TYPE_HANDSHAKE,record_buffer,TLS_FINISHED_MSG_LEN)){
			return HNDSK_ERR;
		}

		if( start_buffer[0] != TLS_HANDSHAKE_TYPE_FINISHED){
#ifdef DEBUG
			DEBUG_MSG("\nFATAL:tls_get_finished: Bad Record Data Type");
#endif
			return HNDSK_ERR;
		}

	} else {
		/* if we didn't received CCS it's fatal */
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_finished: Finished Message Received before Change Cipher Spec Message");
		return HNDSK_ERR;
#endif

	}

	/* cloning hash contexts to continue calculation after */
	md5_clone(tls->client_md5,&temp_md5);
	sha1_clone(tls->client_sha1,&temp_sha1);

	/* compute finished message for the client */
	compute_finished(tls, tls->client_md5,tls->client_sha1,CLIENT,expected_finished);

	/* restore contexts */
	md5_clone(&temp_md5,tls->client_md5);
	sha1_clone(&temp_sha1,tls->client_sha1);

	{
		uint8_t i;
		for(i = 0 ; i < 12 ; i++)
			if(expected_finished[i] != start_buffer[4 + i]) {
#ifdef DEBUG
			DEBUG_MSG("\nFATAL:tls_get_finished: Client Finished Message does not match local calculated\n");
			PRINT_ARRAY(expected_finished,12,"\t\t Local Calculated Finish Message:");
			PRINT_ARRAY( (start_buffer + 4) ,12,"\t\t Client Calculated Finish Message:");
#endif
			return HNDSK_ERR;
			}
	}


	/* last update of hash message */
	md5_update(tls->client_md5,start_buffer,16);
	sha1_update(tls->client_sha1,start_buffer,16);

#ifdef DEBUG_DEEP
	DEBUG_MSG("******************** FINISHED CLIENT MESSAGE END *****************************");
#endif

	return HNDSK_OK;


}

void build_finished(struct tls_connection *tls,uint8_t *finished){


	uint8_t *startbuffer = (finished + START_BUFFER);

	/* fill in handshake header */
	startbuffer[0] = TLS_HANDSHAKE_TYPE_FINISHED;
	startbuffer[1] = 0;
	startbuffer[2] = 0;
	startbuffer[3] = 12; /* size of finished message */

	/*computing finished directly in the record_buffer */
	compute_finished(tls, tls->client_md5, tls->client_sha1, SERVER, startbuffer + 4);

}


uint8_t tls_send_finished(struct tls_connection *tls, uint8_t *record_buffer){

	//uint8_t record_buffer[TLS_FINISHED_MSG_LEN + START_BUFFER];
	uint8_t *startbuffer = (record_buffer + START_BUFFER);

/*	 fill in handshake header
	startbuffer[0] = TLS_HANDSHAKE_TYPE_FINISHED;
	startbuffer[1] = 0;
	startbuffer[2] = 0;
	startbuffer[3] = 12;  size of finished message

	computing finished directly in the record_buffer
	compute_finished(tls, tls->client_md5, tls->client_sha1, SERVER, startbuffer + 4);*/


#ifdef DEBUG_DEEP
	DEBUG_MSG("******************** FINISHED SERVER MESSAGE BEGIN *****************************\n");
#endif

#ifdef DEBUG_DEEP
	PRINT_ARRAY( (startbuffer + 4), 12,"INFO:tls_send_finished: Calculated SERVER Finished Message :");
#endif

	//DEV_PREPARE_OUTPUT;
	write_record(tls, TLS_CONTENT_TYPE_HANDSHAKE, record_buffer, 16);
	//DEV_OUTPUT_DONE;

#ifdef DEBUG
	DEBUG_MSG("INFO:tls_send_finished: Finished message sent");
#endif

#ifdef DEBUG_DEEP
	DEBUG_MSG("\n******************** FINISHED SERVER MESSAGE END *****************************\n");
#endif

	return HNDSK_OK;

}




