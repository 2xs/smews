/*
 * hmac.h
 *
 *  Created on: Jan 6, 2010
 *      Author: root
 */

#ifndef HMAC_H_
#define HMAC_H_

/* common blocksize for both hashing algo */
#define HMAC_BLOCKSIZE 64


void hmac_init(uint8_t,uint8_t*,uint8_t);
void hmac_update(uint8_t);
void hmac_finish(uint8_t);
void hmac(uint8_t,uint8_t*,uint8_t, uint8_t *, uint8_t, uint8_t *);

#endif /* HMAC_H_ */
