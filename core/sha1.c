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

#include "sha1.h"


static void sha1_transform(struct sha1_context *ctx) {

  uint32_t W[80];
  uint32_t A, B, C, D, E;
  uint8_t* p = ctx->buffer;
  uint32_t *state = ctx->state;
  uint8_t t;


  for(t = 0; t < 16; ++t) {
    uint32_t tmp =  *p++ << 24;
    tmp |= *p++ << 16;
    tmp |= *p++ << 8;
    tmp |= *p++;
    W[t] = tmp;
  }

  for(; t < 80; t++) {
    W[t] = ROL(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
  }

  A = state[0];
  B = state[1];
  C = state[2];
  D = state[3];
  E = state[4];

  for(t = 0; t < 80; t++) {
    uint32_t tmp = ROL(5,A) + E + W[t];

    if (t < 20)
      tmp += (D^(B&(C^D))) + 0x5A827999;
    else if ( t < 40)
      tmp += (B^C^D) + 0x6ED9EBA1;
    else if ( t < 60)
      tmp += ((B&C)|(D&(B|C))) + 0x8F1BBCDC;
    else
      tmp += (B^C^D) + 0xCA62C1D6;

    E = D;
    D = C;
    C = ROL(30,B);
    B = A;
    A = tmp;
  }

  state[0] += A;
  state[1] += B;
  state[2] += C;
  state[3] += D;
  state[4] += E;

}



/*
* SHA1 block update operation. Continues an SHA1 message-digest
* operation, processing another message block, and updating internal
* context.
*/
void sha1_update(struct sha1_context *ctx, const uint8_t* data, uint16_t len){

	uint64_t i = ctx->count & 63ULL;
	uint8_t *p = ctx->buffer;
	ctx->count += len;


	while (len--) {
		p[i++] = *data++;
		if (i == 64) {
		  sha1_transform(ctx);
		  i = 0;
		}
	}

}

void sha1_digest(struct sha1_context *ctx){

	uint8_t *p = ctx->buffer;
	uint32_t *state = ctx->state;
	uint64_t cnt = ctx->count * 8;
	uint8_t i;

	sha1_update(ctx, (uint8_t*)"\x80", 1);

	while ((ctx->count & 63) != 56) {
		sha1_update(ctx,(uint8_t*)"\0", 1);
	}

	for (i = 0; i < 8; ++i) {
		uint8_t tmp = cnt >> ((7 - i) * 8);
		sha1_update(ctx,&tmp, 1);
	}

	for (i = 0; i < 5; i++) {
		uint32_t tmp = state[i];
		*p++ = tmp >> 24;
		*p++ = tmp >> 16;
		*p++ = tmp >> 8;
		*p++ = tmp >> 0;
	}

	/* copy_bytes(buffer,r,0,SHA1_SIZE); */
}



/* SHA1 initialization. Begins an SHA1 operation, writing a new context */
void sha1_init(struct sha1_context *ctx){

	ctx->count = 0ULL;

	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xefcdab89;
	ctx->state[2] = 0x98badcfe;
	ctx->state[3] = 0x10325476;
	ctx->state[4] = 0xC3D2E1F0;

	/* for (i = 0; i < 64 ; i++) ctx->buffer[i] = 0; */

}

void sha1_clone(struct sha1_context *src, struct sha1_context *dst){

	uint8_t i ;
	for(i = 0; i < 64 ; i++){
		dst->buffer[i] = src->buffer[i];
	}

	for(i = 0; i < 5 ; i++){
		dst->state[i] = src->state[i];
	}

	dst->count = src->count;

}


