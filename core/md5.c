/* Copyright 2009 Alex Negrea (adapted from Google Inc)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ========================================================================

 Optimized for minimal code size.
 */


#include "md5.h"


static void md5_transform(struct md5_context*);

static CONST_VAR(uint8_t,Kr[64]) =
{
  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};



static CONST_VAR(uint32_t,consts[64]) = {
	0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
	0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
	0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
	0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,

	0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
	0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
	0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
	0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,

	0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
	0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
	0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
	0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,

	0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
	0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
	0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
	0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
};



/*
* MD5 basic transformation. Transforms state based on a 64-byte block.
*/
static void md5_transform(struct md5_context *ctx){


	uint8_t* p = ctx->buffer;
	uint32_t *state = ctx->state;
	uint32_t block[16];

	uint32_t A, B, C, D;
	uint8_t i;


	for(i = 0; i < 16; ++i) {
	    uint32_t tmp =  *p++;
	    tmp |= *p++ << 8;
	    tmp |= *p++ << 16;
	    tmp |= *p++ << 24;
	    block[i] = tmp;
	}

	A = state[0];
	B = state[1];
	C = state[2];
	D = state[3];

	for(i = 0; i < 64; i++) {
		uint32_t f, tmp;
		uint32_t g;

		if (i < 16) {
		  f = (D^(B&(C^D)));
		  g = i;
		} else if ( i < 32) {
		  f = (C^(D&(B^C)));
		  g = (5*i + 1) & 15;
		} else if (i < 48) {
		  f = (B^C^D);
		  g = (3*i + 5) & 15;
		} else {
		  f = (C^(B|(~D)));
		  g = (7*i) & 15;
		}

		tmp = D;
		D = C;
		C = B;
		B = B + ROL(Kr[i], (A+f+consts[i]+block[g]));
		A = tmp;
	}


	state[0] += A;
	state[1] += B;
	state[2] += C;
	state[3] += D;



}

/* MD5 block update operation. Continues an MD5 message-digest
* operation, processing another message block, and updating the context. */
void md5_update(struct md5_context *ctx, const uint8_t* data, uint16_t len){

	uint8_t *p = ctx->buffer;
	uint32_t i = ctx->count & 63ULL;
	ctx->count +=len;


	while (len--) {

		p[i++] = *data++;
		if (i == 64) {
		  md5_transform(ctx);
		  i = 0;
		}

	}


}

/*
* MD5 finalization. Ends an MD5 message-digest operation, writing the
* the message digest and zeroizing the context.
*/
void md5_digest(struct md5_context *ctx){

	uint8_t i;

	uint8_t *p = ctx->buffer;
	uint64_t cnt = ctx->count * 8;

    md5_update(ctx, (uint8_t*)"\x80", 1);

	while(( (ctx->count) & 63) != 56) {
		md5_update(ctx, (uint8_t*)"\0", 1);

	}

	for (i = 0; i < 8; ++i) {
		uint8_t tmp = cnt >> (i * 8);
		md5_update(ctx, &tmp, 1);
	}

	for (i = 0; i < 4; i++) {
		uint32_t tmp = ctx->state[i];
		*p++ = tmp;
		*p++ = tmp >> 8;
		*p++ = tmp >> 16;
		*p++ = tmp >> 24;
	}


	/* copy_bytes(ctx->buffer,r,0,MD5_SIZE); */

}


/* MD5 initialization. Begins an MD5 operation, writing a new context */
void md5_init(struct md5_context *ctx){

	ctx->count = 0ULL;

	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;


	/* for (i = 0; i < 64 ; i++) ctx->buffer[i] = 0; */



}

void md5_clone(struct md5_context *src, struct md5_context *dst){

	uint8_t i ;
	for(i = 0; i < 64 ; i++){
		dst->buffer[i] = src->buffer[i];
	}

	for(i = 0; i < 4 ; i++){
		dst->state[i] = src->state[i];
	}

	dst->count = src->count;

}

