/*
 * hash_common.h
 * Common variables for md5 and sha1
 *  Created on: Jul 19, 2009
 *      Author: alex
 */

#ifndef PRF_H_
#define PRF_H_

#include "hmac.h"

/* SEED for key exchange phase */
#define SEED_LEN 77


/* pseudorandom function that calculates master secret */
void prf(uint8_t*, uint8_t , uint8_t *, uint8_t,uint8_t*,uint8_t);

#endif /* RC4_H_ */
