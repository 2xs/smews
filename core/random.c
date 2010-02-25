

#include "random.h"

/* taps: 32 31 29 1; characteristic polynomial: x^32 + x^31 + x^29 + x + 1 */
#define MASK 0xd0000001u

/* current value of server random (for each connection a next sequence random is generated) */
union int_char server_random;

void rand_next(uint32_t* lfsr){

    uint8_t i;
    for(i = 0 ; i < 8 ; i++)
        lfsr[i] = (server_random.lfsr_int[i] >> 1) ^ (-(server_random.lfsr_int[i] & 1u) & MASK);


#ifdef DEBUG
	DEBUG_MSG("INFO:tls_send_hello_cert_done: Server Generated Random :");
	for( i = 0; i < 32 ; i++)
		DEBUG_VAR( lfsr.lfsr_char[i],"%02x");
	DEBUG_MSG("\n");
#endif

}


void init_rand(uint32_t seed){

	uint8_t i;
    for(i = 0; i < 8 ; i++)
        server_random.lfsr_int[i] = seed + i;


}

