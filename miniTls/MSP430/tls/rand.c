

#include "rand.h"

/* taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
#define MASK 0xd0000001u

void rand_next(uint32_t* lfsr){

    uint8_t i;
    for(i = 0 ; i < 8 ; i++)
        lfsr[i] = (lfsr[i] >> 1) ^ (-(lfsr[i] & 1u) & MASK);

}


void init_rand(uint32_t * lfsr,uint32_t seed){

	uint8_t i;
    for(i = 0; i < 8 ; i++)
        lfsr[i] = seed + i;


}

