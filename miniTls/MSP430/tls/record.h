/*
 * record.h
 *
 *  Created on: Jul 30, 2009
 *      Author: alex
 */

#ifndef RECORD_H_
#define RECORD_H_

#include "target.h"

#include "prf.h"
#include "rc4.h"
#include "tls.h"

void write_header(uint8_t, uint16_t);
uint8_t decode_record(uint8_t,uint8_t *, uint16_t );
void write_record(uint8_t ,uint8_t* , uint16_t );
uint16_t read_header(const uint8_t );



#endif /* RECORD_H_ */
