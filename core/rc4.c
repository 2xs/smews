/*
 * rc4.c
 *
 *  Created on: Jul 23, 2009
 *      Author: alex
 */




#include "rc4.h"

static struct rc4_context rc4_encrypt;
static struct rc4_context rc4_decrypt;


void rc4_init(const uint8_t* key, uint8_t mode) {

	uint16_t i = 0, j = 0;
	uint8_t *S;

	struct rc4_context *ctx;


	if(mode == MODE_ENCRYPT){
		ctx = &rc4_encrypt;
	}
	else{
		ctx = &rc4_decrypt;
	}

	S = ctx->state;

	for (i = 0; i < 256; ++i) {
		S[i] = i;
	}


	for (i = 0; i < 256; ++i) {
		uint8_t tmp;
		j = (j + S[i] + key[i % RC4_KEYSIZE]) & 255;
		tmp = S[i];
		S[i] = S[j];
		S[j] = tmp;
	}

	ctx->i = 0;
	ctx->j = 0;


}

/* old rc4_crypt working on strings */
/*void rc4_crypt(uint8_t *in, uint8_t in_off, uint16_t in_len, uint8_t mode) {

	uint8_t tmp;
	uint32_t t;
	uint8_t kt;
	uint16_t i;
	uint8_t *S;
	uint8_t ii,jj;

	struct rc4_context *ctx;

	if(mode == MODE_ENCRYPT){
		ctx = &rc4_encrypt;
	}
	else{
		ctx = &rc4_decrypt;
	}

	S = ctx->state;

	ii = ctx->i;
	jj = ctx->j;

	for (i = 0; i < in_len; i++) {

		ii = (ii + 1) & 255;
		tmp = S[ii];
		jj = (jj + (tmp & 255) ) & 255;
		S[ii] = S[jj];
		S[jj] = tmp;
		t = (S[ii] + tmp) & 255;
		kt = S[t];
		in[i] = in[in_off + i] ^ kt;

	}


	ctx->i = ii;
	ctx->j = jj;

}*/

void rc4_crypt(uint8_t *in, uint8_t mode) {

	uint8_t tmp;
	uint32_t t;
	uint8_t kt;
	uint8_t *S;
	uint8_t ii,jj;

	struct rc4_context *ctx;

	if(mode == MODE_ENCRYPT){
		ctx = &rc4_encrypt;
	}
	else{
		ctx = &rc4_decrypt;
	}

	S = ctx->state;

	ii = ctx->i;
	jj = ctx->j;


	ii = (ii + 1) & 255;
	tmp = S[ii];
	jj = (jj + (tmp & 255) ) & 255;
	S[ii] = S[jj];
	S[jj] = tmp;
	t = (S[ii] + tmp) & 255;
	kt = S[t];
	*in = *in ^ kt;


	ctx->i = ii;
	ctx->j = jj;

}




/*

int main() {

	//NU UITA SA SCHIMB KEY SIZE DCA FACI TESTE PE KEY DE LUNGIME DIFERITA DE 16
    unsigned char key[6] = "Secret";
    unsigned char plain[14] = "Attack at dawn";

	int y;
	rc4_init(key, MODE_ENCRYPT);
	rc4_init(key, MODE_DECRYPT);
	rc4_crypt(plain,0,14,MODE_ENCRYPT);
	for (y = 0; y < 14; y++)
		printf("%c", plain[y]);
		printf("\n\n");
	rc4_crypt(plain,0,14,MODE_DECRYPT);
	for (y = 0; y < 14; y++)
		printf("%c", plain[y]);
		printf("\n");


    return 0;


}
*/



