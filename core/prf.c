/*
 * prf.c
 *
 *  Created on: Jul 21, 2009
 *  	Author: alex
 */

#include "prf.h"
#include "memory.h"

//static void p_hash(uint8_t, uint8_t*, uint8_t,uint8_t*, uint8_t,  uint8_t *, uint8_t);

extern void inline copy_bytes(const uint8_t*,uint8_t*,uint16_t,uint16_t);


//static uint8_t seed_len;


/* Implementation of p_hash function defined in RFC 2246 section 5
 * secret 	- secret that should be used for hmac
 * alg 		- hashing algorithm to use
 * len 		- how many bytes to generate
 */
static void p_hash(uint8_t alg, uint8_t* secret, uint8_t secret_len, uint8_t* result, uint8_t len, uint8_t *byte_seed, uint8_t seed_len){

	uint8_t r_index = 0;
	uint8_t i;

	uint8_t alg_size = (alg == MD5 ? MD5_KEYSIZE : SHA1_KEYSIZE);
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
	uint8_t *p_md5 = mem_alloc(len);
	uint8_t *p_sha1 = mem_alloc(len);
	//uint8_t p_md5[len];
	//uint8_t p_sha1[len];

	//seed_len = slen;

	secret_len >>= 1;

	/* the secret is split in half and fed to p_hash function for generating len bytes*/

	p_hash(MD5,secret,secret_len,p_md5,len,seed,slen);

	p_hash(SHA1,(secret + secret_len),secret_len,p_sha1,len,seed,slen);

	for(i = 0 ; i < len ; i ++){

		result[i] = p_md5[i] ^ p_sha1[i];
	}

	mem_free(p_md5,len);
	mem_free(p_sha1,len);


}
