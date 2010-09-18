

#include "tls.h"
#include "prf.h"
#include "rc4.h"
#include "record.h"
#include "rand.h"
#include "../ecc/ecc.h"


static CONST_VAR(uint8_t,part1_srv_hello[6]) = { TLS_HANDSHAKE_TYPE_SERVER_HELLO, 0, 0, TLS_HELLO_RECORD_LEN - 4, TLS_SUPPORTED_MAJOR, TLS_SUPPORTED_MINOR };
static CONST_VAR(uint8_t,part2_srv_hello[4]) = { 0, TLS1_ECDH_ECDSA_WITH_RC4_128_SHA >> 8, (uint8_t)TLS1_ECDH_ECDSA_WITH_RC4_128_SHA, TLS_COMPRESSION_NULL};
static CONST_VAR(uint8_t,part3_srv_ext[8]) = { 0, TLS_EXT_LEN, 0, TLS_EXT_POINT_FORMATS, 0, 0x02, 0x01, TLS_COMPRESSION_NULL };
static CONST_VAR(uint8_t,srv_hello_done[4]) = { TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE, 0, 0, 0 };

/* labels for PRF seed */
static CONST_VAR(uint8_t,ms_label[]) /*13 bytes*/ = { 0x6d,0x61,0x73,0x74,0x65,0x72,0x20,0x73,0x65,0x63,0x72,0x65,0x74 };
static CONST_VAR(uint8_t,ke_label[]) /*13 bytes*/ = { 0x6b,0x65,0x79,0x20,0x65,0x78,0x70,0x61,0x6e,0x73,0x69,0x6f,0x6e };
static CONST_VAR(uint8_t,f_label[2][15]) = { {0x73,0x65,0x72,0x76,0x65,0x72,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64}, {0x63,0x6c,0x69,0x65,0x6e,0x74,0x20,0x66,0x69,0x6e,0x69,0x73,0x68,0x65,0x64} };

static union int_char server_random;

uint8_t record_buffer[BUFFER_SIZE]; /* common buffer for input/output */


struct tls_context tls;


/* hash contexts for computation of handshake hashes required in finish message */
static struct md5_context client_md5;
static struct sha1_context client_sha1;


void compute_finished(struct md5_context*, struct sha1_context*, uint8_t , uint8_t *);



/*TODO revise functions for stack optimization maybe convert to macro */
void /*TODO inline*/ copy_bytes(const uint8_t* src, uint8_t* dest,uint16_t start,uint16_t len){

	int16_t i;

	for(i = 0 ; i < len ; i++)
		dest[start++] = src[i];

}



/* fetch client hello and negotiate session */
static uint8_t tls_get_client_hello(){

	int16_t x = 0;
	int16_t i = 0;
	uint8_t length = 0;

	if( !read_header(TLS_CONTENT_TYPE_HANDSHAKE) ){
		return HNDSK_ERR;
	}

	/* -------- Handshake Record Parsing ------- */

	/* get handshake type , handshake length , TLS Max Version and client hello*/
	DEV_GETN( (record_buffer + x) ,38);
	if(record_buffer[0] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
#ifdef DEBUG
		DEBUG_MSG("FATAL:tls_get_client_hello: Bad Handshake Type. Expected Client Hello (0x01)");
#endif
		return HNDSK_ERR;
	}

	/* save the client hello to the end of the buffer */
	copy_bytes((record_buffer + 6),record_buffer, BUFFER_SIZE - RANDOM_SIZE, 32 );

	x+=38;

#ifdef DEBUG_DEEP
	DEBUG_MSG("INFO:tls_get_client_hello: Client Random Bytes : ");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( (record_buffer + BUFFER_SIZE - 32)[i],"%02x");
#endif

	/* get session id length & cipher suite length  */
	/* TODO if session support is added then we must get additional 32 bytes here*/

	DEV_GETC(record_buffer[x++]);
	DEV_GETC(record_buffer[x++]);
	DEV_GETC(record_buffer[x++]); /* cipher len never more than 255 */

	length = record_buffer[x - 1];
#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_get_client_hello: Number of cipher suites proposed is ");
	DEBUG_VAR(length/2,"%d ");
#endif

	/* get cipher suite bytes */
	DEV_GETN( (record_buffer + x), length);

#ifdef DEBUG_DEEP
	DEBUG_MSG("\nINFO:tls_get_client_hello: Cipher suites : \n ");
	for( i = x ; i < length ; i+=2){
		DEBUG_VAR(record_buffer[i],"0x%02x");
		DEBUG_VAR(record_buffer[i + 1],"%02x | ");

	}
#endif

	/* check if client supports our only cipher suite */
	for( i = x ; i < length; i+=2)
		/* found ciphersuite TLS_ECDH_ECDSA_WITH_RC4_SHA */
		if(record_buffer[i] == 0xc0 && record_buffer[i + 1] == 0x02) break;

	if(i == length - 1){

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_hello: Compatible cipher suite not found! \n");
#endif
		return HNDSK_ERR;
	}

	x+= length;

	/* get compression length */
	DEV_GETC(record_buffer[x++]);

	/*  get compression methods and check for null */
	DEV_GETN((record_buffer + x), record_buffer [x - 1]);
	for(i = record_buffer[x - 1]; i > 0 ; i--){
		if(record_buffer[x++] == TLS_COMPRESSION_NULL) break;
	}

	if(i == 0){
		 /* this is almost impossible to reach because every TLS SHOULD support null */
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_hello: Client does not support compression method null");
#endif
		return HNDSK_ERR;
	}

	/* get extensions lenght */
	DEV_GETC(record_buffer[x++]);
	DEV_GETC(record_buffer[x++]); /* assume ext len never more than 255 */
	length = record_buffer[x - 1];

	/* parse extensions */
    i = 0;
	while(i < length){

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

/* send in one record 3 handshake messages : hello,certificate,hello done */
static uint8_t tls_send_hello_cert_done(){

	uint16_t i;

	/* prepare output buffer - target dependent */
	DEV_PREPARE_OUTPUT;

	write_header(TLS_CONTENT_TYPE_HANDSHAKE,TLS_HELLO_RECORD_LEN + TLS_CERT_RECORD_LEN + TLS_HDONE_RECORD_LEN );

	/* --- beginning record filling ---*/

	for( i = 0 ; i < 6; i++)
		DEV_PUT(part1_srv_hello[i]);

	/* update handshake hash */
	md5_update(&client_md5, part1_srv_hello, 6);
	sha1_update(&client_sha1, part1_srv_hello, 6);


	rand_next(server_random.lfsr_int);
	for(i = 0; i < 32 ; i++){
		DEV_PUT(server_random.lfsr_char[i]);
	}

#ifdef DEBUG
	PRINT_ARRAY(server_random.lfsr_char,32,"\nINFO:tls_send_hello_cert_done: Server Generated Random :");
#endif

	/* updating digest with server random */
	md5_update(&client_md5, server_random.lfsr_char, 32);
	sha1_update(&client_sha1, server_random.lfsr_char, 32);



	/* session id length, ciphersuite, comp method*/

	for(i = 0; i < 4 ; i++){
		DEV_PUT(part2_srv_hello[i]);
	}

	/* update handshake hash */
	md5_update(&client_md5, part2_srv_hello, 4);
	sha1_update(&client_sha1, part2_srv_hello, 4);


	/* Section 5.2 RFC 4492
	 * If no
	 * Supported Point Formats Extension is received with the ServerHello,this is equivalent
	 * to an extension allowing only the uncompressed point format.
	 */
	for(i = 0; i < 8 ; i++){
		DEV_PUT(part3_srv_ext[i]);
	}

	/* update handshake hash */
	md5_update(&client_md5, part3_srv_ext, 8);
	sha1_update(&client_sha1, part3_srv_ext, 8);


	for(i = 0 ; i < TLS_CERT_RECORD_LEN ; i++)
			DEV_PUT(server_cert[i]);

	md5_update(&client_md5, server_cert, TLS_CERT_RECORD_LEN);
	sha1_update(&client_sha1, server_cert,TLS_CERT_RECORD_LEN);

	/* write record body */
	for(i = 0; i < 4; i++)
		DEV_PUT(srv_hello_done[i]);

	/* update final handshake */
	md5_update(&client_md5,srv_hello_done,4);
	sha1_update(&client_sha1,srv_hello_done,4);


#ifdef DEBUG
	DEBUG_MSG("\nINFO:tls_send_server_hello: Server Hello, Certificate, Done sent");
#endif

	DEV_OUTPUT_DONE;

	return HNDSK_OK;


}


static uint8_t tls_get_client_keyexch(){

	uint8_t x = 0;
	uint8_t i = 0,j = 0;
	uint8_t t;


	/* seed for prf function when generating master secret and session keys */
	//uint8_t byte_seed[77];

	uint8_t tmp_char;

	/* where keyblock will be saved temporary */
	uint8_t *key_block = record_buffer + SEED_LEN;

	/* set pointers in the buffer for the client random and pms */
	uint8_t *client_random = record_buffer + BUFFER_SIZE - RANDOM_SIZE;
	uint8_t *pms_secret = record_buffer + SEED_LEN;


	if(!read_header(TLS_CONTENT_TYPE_HANDSHAKE)){
		return HNDSK_ERR;
	}


	/* the record len should always be 70 for this key exchange */
	DEV_GETN( (record_buffer + x) , 70); /* fetch full message */

	if(record_buffer[0] != 16) {
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_client_keyexch: Bad Handshake Type. Expected Client Key Exchange(0x10)");
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

	md5_update(&client_md5, record_buffer,70);
	sha1_update(&client_sha1, record_buffer,70);


#ifdef DEBUG
	{
	uint8_t j;
	DEBUG_MSG("\nINFO:tls_get_client_keyexch: Client Public Key (X Coordinate) :");
	for(j = 0; j < 32; j++)
		DEBUG_VAR(record_buffer[6 + j],"%02x");
	DEBUG_MSG("\nINFO:tls_get_client_keyexch: Client Public Key (Y Coordinate) :");
	for(j = 0; j < 32; j++)
		DEBUG_VAR(record_buffer[6 + 32 + j],"%02x");

	}
#endif

	//secret.length = PMS_LEN;


	/* converting to little endian in place */
	for(j = 0; j < 32; j++ ){

		t = record_buffer[6 + j];
		record_buffer[6 + j] = record_buffer[6 + 63 - j];
		record_buffer[6 + 63 - j] = t;
	}

	/* I preserve this for reference. This code did what the above for loop does now. */
	/*bytes_to_big(record_buffer + i,(uint32_t*)(record_buffer + i) );
	bytes_to_big(record_buffer + i + 32,(uint32_t*)(record_buffer + i +32));
	bytes_to_big(ec_priv_key_256,k);*/


	/* calculating premaster secret */
	gfp_point_mult((uint32_t*)ec_priv_key_256,(uint32_t*)(record_buffer + 6 + 32),(uint32_t*)(record_buffer + 6),(uint32_t*)pms_secret);


	//seed_len = SEED_LEN;
	/* creating byte_seed(label + crand + srand) for Master Generation  */
	/* TODO revise this */
	copy_bytes(ms_label,record_buffer,0,PRF_LABEL_SIZE); /* copy the salt "master secret" */
	copy_bytes(client_random,record_buffer,13,32); 		 /* copy the client random */
	copy_bytes(server_random.lfsr_char,record_buffer,45,32); 		 /* copy the server random */

	/* changing PMS endianess because PRF needs it in big-endian */
	for(i = 0, j = PMS_LEN - 1 ; i < PMS_LEN/2 ; i++ , j--){
			tmp_char = pms_secret[j];
			pms_secret[j] = pms_secret[i];
			pms_secret[i] = tmp_char;
	}


	/* call PRF to generate master secret */
	prf(record_buffer,SEED_LEN,pms_secret,PMS_LEN,record_buffer + BUFFER_SIZE - RANDOM_SIZE - SECRET_LEN,SECRET_LEN);

	/* change same byte_seed with new label and inverting randomness :) */
	copy_bytes(ke_label,record_buffer,0,PRF_LABEL_SIZE);
	copy_bytes(server_random.lfsr_char,record_buffer,13,32); 		 /* copy the server random */
	copy_bytes(client_random,record_buffer,45,32); 		 			 /* copy the client random */


	//secret.length = SECRET_LEN;

	/* generate KEY_MATERIAL_LEN key material for connection keys*/
	prf(record_buffer,SEED_LEN,record_buffer + BUFFER_SIZE - RANDOM_SIZE - SECRET_LEN,SECRET_LEN,key_block,KEY_MATERIAL_LEN);
	//PRINT_ARRAY(key_block,KEY_MATERIAL_LEN,"keyblock ; ");

	/* save session keys in tls context */
	copy_bytes(key_block,tls.client_mac,0,MAC_KEYSIZE);
	copy_bytes(key_block + MAC_KEYSIZE,tls.server_mac,0,MAC_KEYSIZE);
	copy_bytes(key_block + (MAC_KEYSIZE<<1),tls.client_key,0,CIPHER_KEYSIZE);
	copy_bytes(key_block + (MAC_KEYSIZE<<1) + (CIPHER_KEYSIZE),tls.server_key,0,CIPHER_KEYSIZE);

	/* initialization of encode/decode ciphers */
	rc4_init(key_block + 2*MAC_KEYSIZE, MODE_DECRYPT);
	rc4_init(key_block + 2*MAC_KEYSIZE + CIPHER_KEYSIZE, MODE_ENCRYPT);

#ifdef DEBUG
	DEBUG_MSG("\n************************ SESSION KEYS BEGIN *****************************************\n");
	PRINT_ARRAY((record_buffer + BUFFER_SIZE - RANDOM_SIZE - SECRET_LEN),SECRET_LEN,"Master Secret :");
	PRINT_ARRAY(tls.client_mac,MAC_KEYSIZE,"Client MAC Secret :");
	PRINT_ARRAY(tls.server_mac,MAC_KEYSIZE,"Server MAC Secret :");
	PRINT_ARRAY(tls.client_key,CIPHER_KEYSIZE,"Client Key Secret :");
	PRINT_ARRAY(tls.server_key,CIPHER_KEYSIZE,"Server Key Secret :");
	DEBUG_MSG("************************** SESSION KEYS END *******************************************");
#endif

	/* get_session_keys(); */

	return HNDSK_OK;

}

/* get client change cipher spec which is exactly 6 bytes */
static uint8_t tls_get_change_cipher(){

	uint8_t tmp_char;

	if(!read_header(TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC)){
			return HNDSK_ERR;
	}

	DEV_GETC(tmp_char);

	if(tmp_char != 1){

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_get_change_cipher: Damaged Change Cipher Spec message");
#endif
		return HNDSK_ERR;
	}

	/* from this point forward client signaled that all its messages will be encoded */
	tls.ccs_recv = 1;

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_get_change_cipher: Change Cipher Spec Message received");
#endif

	return HNDSK_OK;

}

static uint8_t tls_send_change_cipher(){

	DEV_PREPARE_OUTPUT;

	write_header(TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC,1);

	DEV_PUT(0x01);

	DEV_OUTPUT_DONE;

	/*tls.ccs_sent = 1; */

#ifdef DEBUG
		DEBUG_MSG("\nINFO:tls_send_change_cipher: Change Cipher Spec Message Sent \n");
#endif

	return HNDSK_OK;


}

static uint8_t tls_get_finished(){


	uint8_t *expected_finished = record_buffer + START_BUFFER + FINISHED_MSG_LEN + FIN_SEED_LEN;
	uint8_t r_length;
	struct md5_context server_md5;
	struct sha1_context server_sha1;

	uint8_t *startbuffer = record_buffer + START_BUFFER;



#ifdef DEBUG_DEEP
	DEBUG_MSG("\n******************** FINISHED CLIENT MESSAGE BEGIN *****************************\n");
#endif

	if( (r_length = read_header(TLS_CONTENT_TYPE_HANDSHAKE)) == 0 ){
		return HNDSK_ERR;
	}

	/* actual payload size should be 16 for finished message */
	if(r_length - MAC_KEYSIZE != 16) {
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_finished: Bad Finished Message Length.");
#endif
		return HNDSK_ERR;

	}

	DEV_GETN( startbuffer , r_length);

	/* if CCS was received that means the read state changed */
	/* this is the first message encoded with the csuite just negotiated */
	if(tls.ccs_recv){

		if(!decode_record(TLS_CONTENT_TYPE_HANDSHAKE,record_buffer,r_length)){
			return HNDSK_ERR;
		}

		if( startbuffer[0] != TLS_HANDSHAKE_TYPE_FINISHED){
#ifdef DEBUG
			DEBUG_MSG("\nFATAL:tls_get_finished: Bad Record Data Type.");
#endif
		}

	} else {
		/* if we didn't received CCS it's fatal */
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_finished: Finished Received before Change Cipher Spec Message.");
		return HNDSK_ERR;
#endif

	}

	/* cloning hash contexts to continue calculation after */
	md5_clone(&client_md5,&server_md5);
	sha1_clone(&client_sha1,&server_sha1);

	compute_finished(&client_md5,&client_sha1,CLIENT,expected_finished);

	/* restore contexts */
	md5_clone(&server_md5,&client_md5);
	sha1_clone(&server_sha1,&client_sha1);

	{
		uint8_t i;
		for(i = 0 ; i < 12 ; i++)
			if(expected_finished[i] != startbuffer[4+i]) {
#ifdef DEBUG
		DEBUG_MSG("\nFATAL:tls_get_finished: Client Finished Message does not match local \n");
		PRINT_ARRAY(expected_finished,12,"\t\t Local Calculated Finish :");
		PRINT_ARRAY( (startbuffer + 4) ,12,"\t\t Client Calculated Finish :");
#endif
			return HNDSK_ERR;
			}
	}


	/* do the last hash calculation on our finished message */
	md5_update(&client_md5,startbuffer,16);
	sha1_update(&client_sha1,startbuffer,16);

#ifdef DEBUG_DEEP
	DEBUG_MSG("******************** FINISHED CLIENT MESSAGE END *****************************");
#endif

	return HNDSK_OK;


}


static uint8_t tls_send_finished(){

	uint8_t *startbuffer = record_buffer + START_BUFFER;

	/*computing finished in the record_buffer */
	compute_finished(&client_md5,&client_sha1,SERVER,startbuffer + 4);

	/* fill in data layer */
	startbuffer[0] = TLS_HANDSHAKE_TYPE_FINISHED;
	startbuffer[1] = 0;
	startbuffer[2] = 0;
	startbuffer[3] = 12; /* size of finished message */


#ifdef DEBUG_DEEP
	DEBUG_MSG("******************** FINISHED SERVER MESSAGE BEGIN *****************************\n");
#endif

#ifdef DEBUG_DEEP
	PRINT_ARRAY( (startbuffer + 4), 12,"INFO:tls_send_finished: Calculated SERVER Finished Message :");
#endif

	DEV_PREPARE_OUTPUT;
	write_record(TLS_CONTENT_TYPE_HANDSHAKE, record_buffer, 16);
	DEV_OUTPUT_DONE;

#ifdef DEBUG
	DEBUG_MSG("INFO:tls_send_finished: Finished message sent");
#endif

#ifdef DEBUG_DEEP
	DEBUG_MSG("\n******************** FINISHED SERVER MESSAGE END *****************************\n");
#endif

	return HNDSK_OK;

}


void compute_finished(struct md5_context* md5, struct sha1_context *sha1,uint8_t role, uint8_t *r){

	uint8_t *byte_seed = record_buffer + START_BUFFER + 16;
	md5_digest(md5);
	sha1_digest(sha1);



	//uint8_t *m_secret = record_buffer + BUFFER_SIZE - RANDOM_SIZE - PMS_LEN - SECRET_LEN;
	/* creating seed */
	//seed_len = 15 + MD5_SIZE + SHA1_SIZE;
	copy_bytes(f_label[role],byte_seed,0,15);
	copy_bytes(md5->buffer,byte_seed,15,MD5_SIZE);
	copy_bytes(sha1->buffer,byte_seed,31,SHA1_SIZE);

	/* generate finished message */
	prf(byte_seed,51,record_buffer + BUFFER_SIZE - RANDOM_SIZE - SECRET_LEN,SECRET_LEN,r,12);


}


void tls_send_app_data(uint16_t len){

	DEV_PREPARE_OUTPUT;

#ifdef DEBUG

	DEBUG_MSG("\n\n>>>>> Sending Application Data : ");
	PRINT_ARRAY((record_buffer + START_BUFFER),len,"");

#endif
	/* record buffer must be filled by the application layer from START_BUFFER POSITION */
	write_record(TLS_CONTENT_TYPE_APPLICATION_DATA,record_buffer,len);
	DEV_OUTPUT_DONE;


}

uint8_t tls_get_app_data(){

	uint8_t *startbuffer = record_buffer + START_BUFFER;
	uint16_t r_length;

	if( (r_length = read_header(TLS_CONTENT_TYPE_APPLICATION_DATA)) == 0 ){
			return HNDSK_ERR;
	}

	DEV_GETN( (startbuffer), r_length);

#ifdef DEBUG
	DEBUG_MSG("\n\n<<<<< Received Application Data (");
	DEBUG_VAR(r_length," %d bytes ) : ");
	PRINT_ARRAY( (startbuffer),r_length,"");
#endif

	if(!decode_record(TLS_CONTENT_TYPE_APPLICATION_DATA,record_buffer,r_length)){
		return HNDSK_ERR;
	}

#ifdef DEBUG
	DEBUG_MSG("<<<<< Application Data Decoded : ");
	{
		uint16_t i;
		for( i = 0; i < (r_length - MAC_KEYSIZE ); i++)
			DEBUG_VAR(startbuffer[i],"%c");
		DEBUG_MSG("\n");
	}

#endif


	return HNDSK_OK;


}


uint8_t tls_handshake(){



	uint8_t ret;
	/* init global structures for handshake computation */
	md5_init(&client_md5);
	sha1_init(&client_sha1);

	init_rand(server_random.lfsr_int, 0xABCDEF12);

	ret = tls_get_client_hello();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_send_hello_cert_done();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_get_client_keyexch();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_get_change_cipher();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_get_finished();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_send_change_cipher();
	if(ret!= HNDSK_OK) return 0;
	ret = tls_send_finished();
	if(ret!= HNDSK_OK) return 0;


	return HNDSK_OK;

}


