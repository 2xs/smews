
#ifndef RECORD_H_
#define RECORD_H_

#include "types.h"
#include "prf.h"
#include "tls.h"


uint16_t read_header(const uint8_t );
void write_header(uint8_t type, uint16_t len);
uint8_t decode_record(struct tls_connection *tls,uint8_t type,uint8_t *record_data, uint16_t len);
void write_record(struct tls_connection *tls, uint8_t type, uint8_t* record_buffer, uint16_t len);

#endif /* RECORD_H_ */
