/*
 * Prime Field Arithmetic implementation for NIST-P256
 * Majority of Algorithms are referenced in Guide to Elliptic Curve Cryptography
 * or Handbook of Applied Cryptography
 */

#include "ecc.h"

/* Prime Modulus already transformed in LSB to MSB */
const uint32_t modulus[] = {
	0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0,0,0,
	0x00000001,0xFFFFFFFF
};

/* constants one and zero */
const uint32_t one[GFP_ARRAY_LENGHT32] = {1,0,0,0,0,0,0,0};
const uint32_t zero[GFP_ARRAY_LENGHT32] = {0,0,0,0,0,0,0,0};



/* Routine to multiply two big integers in little endian format
 * r is twice the size of inputs and must be accomodated
 * Alg 2.9(p.31) Guide to ECC */
void gfp_mult(uint32_t *x,uint32_t *y, uint32_t *r){

	uint32_t c;
	uint64_t uv;
	uint8_t i,j;


	for(i = 0; i < 2*GFP_ARRAY_LENGHT32; i++)
		r[i] = 0;

	for(i = 0; i < GFP_ARRAY_LENGHT32 ; i++){
		c = 0;
		for(j = 0; j < GFP_ARRAY_LENGHT32  ; j++){

			uv = (uint64_t)r[i+j] + (uint64_t)x[j]*(uint64_t)y[i] + c;

			/* getting the LSB -> v */
			r[i+j] = (uint32_t)uv;
			/* getting u */
			c = uv >> 32;
		}
		/* getting u again */
		r[i+GFP_ARRAY_LENGHT32] = uv >> 32;

	}

}

/* calculates (x*y mod p) by first multiplying and then reducing */
void gfp_mod_mult(uint32_t *x,uint32_t *y, uint32_t *r){

	uint32_t r64[2*GFP_ARRAY_LENGHT32];

	gfp_mult(x,y,r64);
	gfp_reduce(r64,r);

}

// /* Routine for squaring a big integer in little endian format
//  * r is twice the size of input and must be accomodated
//  * Alg 14.16 (p.597) Handbook of Applied Cryptography */
// void gfp_square(uint32_t *x,uint32_t *r){
// /* TODO squaring does not work for the moment */
// 	uint32_t c;
// 	uint64_t uv;
// 	uint64_t toadd;
// 	uint8_t i,j;
// 
// 	char carry; /* one bit carry TODO more efficient ? one bit variable*/
// 
// 	/* store the maxvalue of a 64 bits integer to compare to it later*/
// 	/*uint64_t maxlong;
// 	*((uint32_t*)&maxlong) = 0xffffffff;
// 	*(((uint32_t*)&maxlong)+1) = 0xffffffff;*/
// 
// 	/* initialization of result */
// 	for(i = 0; i < 2*GFP_ARRAY_LENGHT32; i++)
// 		r[i] = 0;
// 
// 	for(i = 0; i < GFP_ARRAY_LENGHT32 ; i++){
// 
// 		uv = (uint64_t)x[i]*x[i];
// 
// 		/* check if we have an over 64bit overflow and set carry */
// 		carry = 0xffffffffffffffffLL - r[2*i] < uv;
// 		uv += r[2*i];
// 
// 		r[2*i] = (uint32_t)uv; /* get v (LSB) */
// 		c = (uint32_t)(uv >> 32); /* get u */
// 
// 		for(j = i+1; j < GFP_ARRAY_LENGHT32; j++ ){
// 
// 			uv = (uint64_t)x[j]*x[i];
// 
// 			toadd = uv + r[i+j] + c + ((uint64_t)carry<<32);
// 			/* check if what were going to add will cause an 64 bit overflow to uv */
// 			carry = ( 0xffffffffffffffffLL - toadd ) < uv;
// 
// 			uv += toadd;
// 
// 			r[i+j] = (uint32_t)uv;
// 			c = (uint32_t)(uv >> 32);
// 		}
// 
// 		r[i+GFP_ARRAY_LENGHT32] = (uint32_t)(uv >> 32);
// 
// 	}
// 
// 
// }

/* TODO have to optimize this function , is huge */
/* Fast reduction algorithm for special NIST primes
 * Algorithm 2.29(p.46) Guide to ECC
 * r = x mod p */
void gfp_reduce(uint32_t *x,uint32_t *r){


	int8_t scarry = 0; /* global carry signed*/

	uint32_t s1[GFP_ARRAY_LENGHT32] = { x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7] };

	uint32_t s2[GFP_ARRAY_LENGHT32] = { 0, 0, 0 ,x[11],x[12],x[13],x[14],x[15]};

	uint32_t s3[GFP_ARRAY_LENGHT32] = { 0, 0, 0 ,x[12],x[13],x[14],x[15],0 };

	uint32_t s4[GFP_ARRAY_LENGHT32] = { x[8],x[9],x[10],0,0,0,x[14],x[15] };

	uint32_t s5[GFP_ARRAY_LENGHT32] = { x[9],x[10],x[11],x[13],x[14],x[15],x[13],x[8] };

	uint32_t s6[GFP_ARRAY_LENGHT32] = { x[11],x[12],x[13],0,0,0,x[8],x[10] };

	uint32_t s7[GFP_ARRAY_LENGHT32] = { x[12],x[13],x[14],x[15],0,0,x[9],x[11] };

	uint32_t s8[GFP_ARRAY_LENGHT32] = { x[13],x[14],x[15],x[8],x[9],x[10],0,x[12] };

	uint32_t s9[GFP_ARRAY_LENGHT32] = { x[14],x[15],0,x[9],x[10],x[11],0,x[13] };

//  	__asm__ (
//  	"toto"
//  	);

	/* Result is (s1 + 2s2 + 2s3 + s4 + s5 − s6 − s7 − s8 − s9 mod p256 ). */


	scarry = gfp_add(s1,s2,r);

	scarry += gfp_add(r,s2,r);

	scarry += gfp_add(r,s3,r);

	scarry += gfp_add(r,s3,r);

	scarry += gfp_add(r,s4,r);

	scarry += gfp_add(r,s5,r);

	scarry -= gfp_sub(r,s6,r);

	scarry -= gfp_sub(r,s7,r);

	scarry -= gfp_sub(r,s8,r);

	scarry -= gfp_sub(r,s9,r);

	while(scarry>0){
		uint32_t r8_d = scarry;
		uint32_t overflow[GFP_ARRAY_LENGHT32] = { r8_d,0,0,-r8_d,0xffffffff,0xffffffff,-(r8_d + 1),(r8_d-1)};
		scarry = gfp_add(r,overflow,r);

	}

	while(scarry<0){

		uint32_t r8_d = -scarry;
		uint32_t overflow[GFP_ARRAY_LENGHT32] = { r8_d,0,0,-r8_d,0xffffffff,0xffffffff,-(r8_d + 1),(r8_d-1)};
		scarry = -gfp_sub(r,overflow,r);

	}

	/* MODULO p256 operation  : r = r - p until r < p */
	while(gfp_cmp(r,modulus) != -1){
		gfp_sub(r,modulus,r);

	}


}



/* magnitude comparison of two big integers in Little Endian format(LSB to MSB)
 * x > y => 1
 * x = y => 0
 * x < y => -1
 */
int8_t gfp_cmp(const uint32_t *x, const uint32_t *y){

	uint8_t i;
	/*TODO improve comparison with zero or one*/


	/* starting from MSB */
	for(i = GFP_ARRAY_LENGHT32 - 1; i!=255; i--)
	{ /* check digit by digit from MSB to LSB */

		 if (x[i] > y[i]) return 1;
		 if (x[i] < y[i]) return -1;

	}
	return 0;

}

/* used to zeroize a big integer */
void gfp_zeroize(uint32_t* x){

	uint8_t i ;
	/* TODO a faster zeroize maybe ? inline even */
	for(i = 0; i < GFP_ARRAY_LENGHT32; i++)
		x[i] = 0;

}

/* copy values from source to dest */
void gfp_copy(const uint32_t *source, uint32_t *dest){
	/* TODO make copy of 1 faster */
	unsigned char i;
	for(i = 0; i < 8 ; i++)
		dest[i] = source[i];


}

/*  checks parity of a big integer
 *  returns 1 if odd, 0 if even */
uint8_t is_odd(uint32_t *a) {
	return ((a[0] & 0x00000001) != 0);
}

/* checks if the bit on position 'bit' is set in 'a' */
uint8_t test_bit(uint32_t *a, int16_t bit) {
	return ((a[bit/GFP_WORD_LENGHT] & (0x00000001 << (bit%GFP_WORD_LENGHT))) != 0);
}

/* routine that calculates x = x/2 in place */
void gfp_divide2(uint32_t *x){

	uint8_t carry = 0;

	 /* if 'x' is odd, we have to add the prime to make it even */
	if(is_odd(x)){
		carry = gfp_add(x,modulus,x);
		gfp_shr(x);
		x[GFP_ARRAY_LENGHT32 - 1] |= (carry << (GFP_WORD_LENGHT - 1));
	} else {
		gfp_shr(x);
	}



}

/* routine that shifts right x by 1 */
void gfp_shr(uint32_t *x){

	int16_t i = GFP_ARRAY_LENGHT32 ;
	uint32_t tmp;
	uint32_t res = 0;

	while(i--){
		tmp = x[i];
		x[i] = res | (tmp >> 1);
		res = (tmp << 31) & 0xffffffff;
	}

}

/* Routine that calculates a^-1 mod p using Binary algorithm for inversion in Fp
 * Algorithm 2.22(p.41) Guide to ECC */
void gfp_mod_inv(uint32_t *a){


	uint32_t v[GFP_ARRAY_LENGHT32];
	uint32_t x1[GFP_ARRAY_LENGHT32] = {1,0,0,0,0,0,0,0};
	uint32_t x2[GFP_ARRAY_LENGHT32] = {0,0,0,0,0,0,0,0};


	gfp_copy(modulus,v);		/* v <- p */

	while( (gfp_cmp(a,one) != 0) && (gfp_cmp(v,one) != 0) ){

		while(!is_odd(a)){

			gfp_shr(a);
			gfp_divide2(x1);

		}

		while(!is_odd(v)){

			gfp_shr(v);
			gfp_divide2(x2);

		}

		if(gfp_cmp(a,v) >=0){
			gfp_sub(a,v,a);

			gfp_mod_sub(x1,x2,x1);

		} else {
			gfp_sub(v,a,v);
			gfp_mod_sub(x2,x1,x2);

		}

	}

	if(gfp_cmp(a,one) == 0){
		gfp_copy(x1,a);
	} else {
		gfp_copy(x2,a);
	}

}



/*void complement(uint32_t *x){

	uint8_t i;

	for(i = 0; i < GFP_ARRAY_LENGHT32 ; i++)
		x[i] = 0xffffffff - x[i];

	gfp_add(x,one,x);

}*/

/* routine that converts from BIG-ENDIAN (MSB to LSB) byte array
 * to the right endianness of target machine */
void bytes_to_big(const uint8_t *bytes, uint32_t *big)
{

	uint8_t i,j;
#ifdef DEBUG_ECC
	PRINT_BYTES(bytes,"Byte array before mapping (uint8_t[]):");
#endif

	/* convert to LE order (LSB(0) to MSB(x))*/
#ifdef LITTLE_ENDIAN

		for(i = 0,j = GFP_ARRAY_LENGHT32 - 1; i < GFP_ARRAY_LENGHT8; i+=4,j--){

			big[j] = bytes[i+3];
			big[j] |= bytes[i+2] << 8;
			big[j] |= bytes[i+1] << 16;
			big[j] |= bytes[i] << 24;

		}


#else  /* integers are already BE order, so just reverse the array to have it LSB to MSB */
	   for(i = 0,j = GFP_ARRAY_LENGHT32 - 1; i < GFP_ARRAY_LENGHT8; i+=4,j--)
		   big[j] = *(uint32_t*)((&bytes[i]));
#endif

#ifdef DEBUG_ECC
	PRINT_BIG(big,"Big integer after conversion (uint32_t[]): ");
#endif


}
