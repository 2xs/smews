#ifndef MD5_H_
#define MD5_H_

#include "types.h"

#define MD5_KEYSIZE 16

#define MD5 2

struct md5_context {

	uint32_t state[4];
	uint8_t buffer[64];
	uint64_t count;

}; /* 88 bytes */

/* macro needed for both MD5 & SHA1 */
#define ROL(bits, value) (((value) << (bits)) | ((value) >> (32 - (bits))))

void md5_init(struct md5_context*);
void md5_digest(struct md5_context*);
void md5_update(struct md5_context*,const uint8_t*,uint16_t);
void md5_clone(struct md5_context*, struct md5_context*);

#endif /* MD5_H_ */
