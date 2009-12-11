/*
 * rand.h
 *
 *  Created on: Aug 10, 2009
 *      Author: alex
 */

#ifndef RAND_H_
#define RAND_H_

#include "types.h"

void rand_next(uint32_t*);
void init_rand(uint32_t);

/* structure for random value */
union int_char{
    uint32_t lfsr_int[8];
    uint8_t lfsr_char[32];

};



#endif /* RAND_H_ */
