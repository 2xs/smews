
#ifndef RECORD_H_
#define RECORD_H_

#include "types.h"

uint16_t read_header(const uint8_t );
void write_header(uint8_t type, uint16_t len);


#endif /* RECORD_H_ */
