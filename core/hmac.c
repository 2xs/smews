
#include "hmac.h"

static uint8_t ipad[HMAC_BLOCKSIZE];
static uint8_t opad[HMAC_BLOCKSIZE];

/* local contexts for hash computation */
struct md5_context md5;
struct sha1_context sha1;


/* function needed to update HMAC bytewise */
void hmac_update(uint8_t c){

	sha1_update(&sha1,&c,1);

}

void hmac_finish(uint8_t alg){

	if(alg == MD5){

		md5_digest(&md5);
		copy_bytes(md5.buffer,ipad,0,MD5_KEYSIZE);
		md5_init(&md5);
		md5_update(&md5,opad,HMAC_BLOCKSIZE);
		md5_update(&md5,ipad,MD5_KEYSIZE);
		md5_digest(&md5);

	} else {

		sha1_digest(&sha1);
		copy_bytes(sha1.buffer,ipad,0,SHA1_KEYSIZE);
		sha1_init(&sha1);
		sha1_update(&sha1,opad,HMAC_BLOCKSIZE);
		sha1_update(&sha1,ipad,SHA1_KEYSIZE);
		sha1_digest(&sha1);

	}

}

void hmac_preamble(struct tls_connection* tls, uint16_t record_size, uint8_t operation){

	/* HMAC first 13 bytes necessary for later record MAC calculation
	 * 6.2.3.1 RFC 2246 */
	/* record_size is payload data (no MAC) */
	uint8_t i;
	uint8_t *seqno = (operation == DECODE ? tls->decode_seq_no.bytes : tls->encode_seq_no.bytes );
	for( i = 0 ; i < 8 ; i++)
		hmac_update(seqno[7-i]);

	hmac_update(TLS_CONTENT_TYPE_APPLICATION_DATA);
	hmac_update(TLS_SUPPORTED_MAJOR);
	hmac_update(TLS_SUPPORTED_MINOR);
	hmac_update( (record_size ) >> 8);
	hmac_update( (uint8_t)(record_size));

}

void hmac_init(uint8_t alg,uint8_t *key, uint8_t key_len){

	uint8_t i;
	for(i = 0; i < key_len ; i++){
		ipad[i] = key[i] ^ 0x36;
		opad[i] = key[i] ^ 0x5c;
	}

	for(i = key_len; i < HMAC_BLOCKSIZE ; i++){
		ipad[i] = 0x36;
		opad[i] = 0x5c;
	}

	if(alg == MD5){
		md5_init(&md5);
		md5_update(&md5,ipad,HMAC_BLOCKSIZE);
	} else {

		sha1_init(&sha1);
		sha1_update(&sha1,ipad,HMAC_BLOCKSIZE);
	}

}



/* wrapper function used by prf */
void hmac(uint8_t alg,uint8_t *key, uint8_t key_len,uint8_t *data, uint8_t data_len,uint8_t *result){

	hmac_init(alg,key,key_len);

	if(alg == MD5){
		md5_update(&md5,data,data_len);

	}else {
		sha1_update(&sha1,data,data_len);
	}

	hmac_finish(alg);

	if(alg == MD5){
		copy_bytes(md5.buffer,result,0,MD5_KEYSIZE);

	}else {
		copy_bytes(sha1.buffer,result,0,SHA1_KEYSIZE);
	}

}


