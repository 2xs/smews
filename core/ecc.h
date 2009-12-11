/*
 * struct.h
 *
 *  Created on: Jun 24, 2009
 *      Author: alex
 */

#ifndef ECC_H_
#define ECC_H_

#include "types.h"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif


#define  CARRYOUT(W)  (uint32_t)((W)>>32) /* get the MSB of a 64 bit number */
#define  ACCUM(W)     (uint32_t)(W)		  /* get the LSB of a 64 bit number */

#define MP_DIGIT(x,n) x[n]
#define MP_DIGIT_MAX 0xffffffff


/* This Print Functions Prints a big integer in big or little endian format */
/*
#define  PRINT_BIG(x,msg) { \
	printf("\n%s",msg); \
	for(i = 0; i < GFP_ARRAY_LENGHT32 ; i++) printf("%08x",x[i]);	}
*/
#define  PRINT_BIG(x,msg) { \
	uint8_t i; \
	printf("\n%s",msg); \
	for(i = GFP_ARRAY_LENGHT32-1; i !=255 ; i--) printf("%08x",x[i]); \
	printf("\n"); }

/*
#define  PRINT_DBIG(x,msg) { \
	printf("\n%s",msg); \
	for(i = 0; i < 2*GFP_ARRAY_LENGHT32 ; i++) printf("%08x",x[i]);	}
*/

#define  PRINT_DBIG(x,msg) { \
	printf("\n%s",msg); \
	for(i = 2*GFP_ARRAY_LENGHT32 -1; i !=255 ; i--) printf("%08x",x[i]);\
	printf("\n");}


#define MP_ADD_CARRY(a1, a2, s, cin, cout)   \
{ uint64_t w; \
  w = ((uint64_t)(cin)) + (a1) + (a2); \
  s = ACCUM(w); \
  cout = CARRYOUT(w); }

#define MP_SUB_BORROW(a1, a2, s, bin, bout)   \
{ uint64_t w; \
  w = ((uint64_t)(a1)) - (a2) - (bin); \
  s = ACCUM(w); \
  bout = (w >> 32) & 1; }

#define PRINT(x) print_hex(x,GFP_ARRAY_LENGHT32,"x = :")

/* prime field size in bits */
#define GFP_PRIME_FIELD 256
/* word length for the architecture usually 8/16/32 bits */
#define GFP_WORD_LENGHT 32
/* Length of array to store integers */
#define GFP_ARRAY_LENGHT32 GFP_PRIME_FIELD/GFP_WORD_LENGHT
/* Length of array to store octets */
#define GFP_ARRAY_LENGHT8 GFP_PRIME_FIELD/8


/* prime field arithmetic */
void gfp_mod_sub(uint32_t*,uint32_t*,uint32_t*);
uint8_t gfp_sub(uint32_t*,const uint32_t*,uint32_t*);
void gfp_mod_add(uint32_t*,uint32_t*,uint32_t*);
uint8_t gfp_add(uint32_t*,const uint32_t*,uint32_t*);
void gfp_mult(uint32_t*,uint32_t*,uint32_t*);
void gfp_mod_mult(uint32_t*,uint32_t*,uint32_t*);
void gfp_square(uint32_t*,uint32_t*);
void gfp_divide2(uint32_t*);
void gfp_fast_reduction_256(uint32_t*,uint32_t*);
void gfp_reduce(uint32_t*,uint32_t*);
void gfp_mod_inv(uint32_t*);
void gfp_shr(uint32_t *x);

/* helper functions for prime-field arithmetic */
int8_t gfp_cmp(const uint32_t*, const uint32_t*);
void gfp_zeroize(uint32_t*);
void gfp_copy(const uint32_t*,uint32_t*);

uint8_t is_odd(uint32_t*);
uint8_t test_bit(uint32_t*,int16_t);
void bytes_to_big(const uint8_t*,uint32_t*);

/* curve arithmetic routines */

void gfp_point_mult(uint32_t*,uint32_t*,uint32_t*,uint32_t*);


#endif

