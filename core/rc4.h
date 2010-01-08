
#ifndef RC4_H_
#define RC4_H_

#include "types.h"

#define RC4_KEYSIZE 16

#define MODE_ENCRYPT 1
#define MODE_DECRYPT 2

struct rc4_context{
	uint8_t state[256];
	uint8_t i,j;
};

//void rc4_crypt(uint8_t*, uint8_t, uint16_t, uint8_t);
void rc4_crypt(uint8_t*, uint8_t);
void rc4_init(const uint8_t* , uint8_t);


#endif /* RC4_H_ */
