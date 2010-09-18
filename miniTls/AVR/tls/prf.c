/*
 * prf.c
 *
 *  Created on: Jul 21, 2009
 *  	Author: alex
 */

#include "prf.h"

static void p_hash(uint8_t, uint8_t*, uint8_t,uint8_t*, uint8_t,  uint8_t *);

extern void /*TODO add inline */copy_bytes(const uint8_t*,uint8_t*,uint16_t,uint16_t);


static uint8_t seed_len;

 static struct md5_context md5;
 static struct sha1_context sha1;


/*
* It is the implementation of the hashing function defined in RFC 2104
* alg 		- Hashing algorithm type (MD5 or SHA)
* secret 	- Secret used in combination for generating hash
* seed 		- Seed
* result 	- Array containing the hash of alg_size size
*/
void hmac(uint8_t alg, uint8_t *secret, uint8_t secret_len, uint8_t *seed, uint16_t seed_len, uint8_t *r){

	/* local contexts for hash computation */

	uint8_t i ;
	/* blocksize is 64 for both algo */
	uint8_t ipad[HMAC_BLOCKSIZE];
	uint8_t opad[HMAC_BLOCKSIZE];
	

	for(i = 0; i < HMAC_BLOCKSIZE ; i++){
		ipad[i] = 0x36;
		opad[i] = 0x5c;
	}

	/* here rfc says we should check if secret len is greater than blocksize, but it's never our case */

	/* ipad = K XOR ipad */
	for(i = 0; i < secret_len ; i++)
		ipad[i] = (secret[i] ^ ipad[i]);

	/* calculating hash for ipad and seed */
	if(alg==MD5){
		md5_init(&md5);
		md5_update(&md5,ipad,HMAC_BLOCKSIZE);
		md5_update(&md5,seed,seed_len);
		md5_digest(&md5);

		copy_bytes(md5.buffer,ipad,0,MD5_SIZE); /* TODO revise */

	} else {
		sha1_init(&sha1);
		sha1_update(&sha1,ipad,HMAC_BLOCKSIZE);
		sha1_update(&sha1,seed,seed_len);
		sha1_digest(&sha1);

		copy_bytes(sha1.buffer,ipad,0,SHA1_SIZE); /* TODO revise*/

	}

	/* opad = K XOR opad */
	for(i = 0; i < secret_len ; i++)
		opad[i] = (secret[i] ^ opad[i]);

	/* hash for opad and update it with last hash */
	if(alg == MD5){
		md5_init(&md5);
		md5_update(&md5, opad,HMAC_BLOCKSIZE);
		md5_update(&md5, ipad,MD5_SIZE);

		/* get final hash */
		md5_digest(&md5);


	} else {

		sha1_init(&sha1);
		sha1_update(&sha1,opad,HMAC_BLOCKSIZE);
		sha1_update(&sha1,ipad,SHA1_SIZE);

		/* get final hash */
		sha1_digest(&sha1);

	}

	if(alg == MD5) copy_bytes(md5.buffer,r,0,MD5_SIZE);
	if(alg == SHA1) copy_bytes(sha1.buffer,r,0,SHA1_SIZE);


}


/* Implementation of p_hash function defined in RFC 2246 section 5
 * secret 	- secret that should be used for hmac
 * alg 		- hashing algorithm to use
 * len 		- how many bytes to generate
 */
static void p_hash(uint8_t alg, uint8_t* secret, uint8_t secret_len, uint8_t* result, uint8_t len, uint8_t *byte_seed){

	uint8_t r_index = 0;
	uint8_t i;

	uint8_t alg_size = (alg == MD5 ? MD5_SIZE : SHA1_SIZE);
	uint8_t iterations = (len % alg_size == 0) ? len/alg_size : len/alg_size + 1 ;
	/* TODO not c90 compliant ,have VLA */
	uint8_t temp[seed_len+alg_size];
	uint8_t a_i[alg_size]; /* size will be changed to MD5_SIZE */

	/* calculating A(i) = HMAC_hash(secret, A(i-1)) */
	for( i = 0 ; i < iterations - 1 ; i++){

		if(i == 0){
			/* a(0) */
			hmac(alg,secret,secret_len,byte_seed,seed_len,a_i);
		} else {
			hmac(alg,secret,secret_len,a_i,alg_size,a_i);
		}

		/* constructing A(i) + seed into a temp array to be hmac-ed again and added to the result */
		copy_bytes(a_i,temp,0,alg_size);
		copy_bytes(byte_seed,temp,alg_size,seed_len);
		hmac(alg,secret,secret_len,temp,seed_len+alg_size, result + r_index);

		r_index += alg_size; /* update index in result array */


	}

	if(i == 0){
		/* a(0) */
		hmac(alg,secret,secret_len,byte_seed,seed_len,a_i);
	} else {
		/* last iteration just fill up to max len of result */
		hmac(alg,secret,secret_len,a_i,alg_size,a_i);
	}
	copy_bytes(a_i,temp,0,alg_size);
	copy_bytes(byte_seed,temp,alg_size,seed_len);
	hmac(alg,secret,secret_len,temp,seed_len+alg_size, result + r_index);


}

/* This is the implementation of the Pseudo-Random Function required by TLS
 * when generating Master Secret and connection keys(RFC 2246 Section 5)
 * seed,slen			- seed and seed len
 * secret,secret_len 	- secret key and secret key len
*  result,len			- pointer for result and how many bytes to generate
 */
void prf(uint8_t *seed, uint8_t slen, uint8_t* secret, uint8_t secret_len, uint8_t *result, uint8_t len){

	uint8_t i;

	/* results of p_hash function */
	uint8_t *p_md5 = seed + slen + secret_len;
	uint8_t *p_sha1 = seed + slen + secret_len + len;

	seed_len = slen;

	secret_len >>= 1;

	/* the secret is split in half and fed to p_hash function for generating len bytes*/

	p_hash(MD5,secret,secret_len,p_md5,len,seed);

	p_hash(SHA1,(secret + secret_len),secret_len,p_sha1,len,seed);

	for(i = 0 ; i < len ; i ++){

		result[i] = p_md5[i] ^ p_sha1[i];
	}


}
