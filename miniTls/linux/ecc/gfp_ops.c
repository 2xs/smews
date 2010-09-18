
#include "ecc.h"

extern const uint32_t modulus[];

/* Routine to add with carry two big integers which are in Little Endian format LSB to MSB
 * returns carry if there is a 256bit overflow */
uint8_t gfp_add(uint32_t *x,const uint32_t *y,uint32_t *r){

	uint8_t i;
	/* carry to take for the next addition */
	uint8_t carry_next = 0;
	uint8_t carry = 0;

	for(i = 0; i < GFP_ARRAY_LENGHT32 ; i ++){

		/* TODO alternative of not using 64 bit temp var. Cannot check overflow with unsigned values like below */
		if( (0xffffffff - x[i]) < y[i]) carry_next = 1;
		else carry_next = 0;

		r[i] = x[i] + y[i];
		
		if( 0xffffffff - carry < r[i]) carry_next += 1;
		r[i] += carry;
		
		carry = carry_next;
		//MP_ADD_CARRY(x[i],y[i],r[i],carry_next,carry_next);


	}

	return carry_next;

}



/* Performs Modular addition (Alg 2.7(p.31) from Guide to ECC */
void gfp_mod_add(uint32_t *x,uint32_t *y,uint32_t *r){


    /* if carry set OR c>=p, substract p from c */
	if(gfp_add(x,y,r) || gfp_cmp(r,modulus)>=0){

		gfp_sub(r,modulus,r);
	}

}


/* Routine to substract two big integers (x>=y) which are in Little Endian format LSB to MSB
 * For x<y the result must be complemented */
uint8_t gfp_sub(uint32_t *x, const uint32_t *y,uint32_t *r){

	uint8_t borrow = 0;
	uint8_t borrow_next = 0;
	uint8_t i;

	for(i = 0; i < GFP_ARRAY_LENGHT32  ; i++){

		if( x[i] < y[i] ) borrow_next = 1;
		else borrow_next = 0;

		r[i] = x[i] - y[i];
		
		if(r[i]<borrow) borrow_next +=1;
		r[i] -= borrow;

	      

		borrow = borrow_next;
		//MP_SUB_BORROW(x[i],y[i],r[i],borrow,borrow);

	}

	return borrow;

}

/* Performs Modular Substraction (Alg 2.8(p.31) from Guide to ECC */
void gfp_mod_sub(uint32_t *x,uint32_t *y,uint32_t *r){


	if(gfp_sub(x,y,r) == 1){
		/* modular operation c = c-p */
		gfp_add(r,modulus,r);

	}

}