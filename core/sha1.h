#ifndef SHA1_H_
#define SHA1_H_

#include "types.h"

#define SHA1_KEYSIZE 20

#define SHA1 1

struct sha1_context {

	uint32_t state[5];
	uint64_t count;
	uint8_t buffer[64];

}; /* 92 bytes */

/* macro needed for both MD5 & SHA1 */
#define ROL(bits, value) (((value) << (bits)) | ((value) >> (32 - (bits))))


void sha1_init(struct sha1_context*);
void sha1_digest(struct sha1_context*);
void sha1_update(struct sha1_context*,const uint8_t*,uint16_t);
void sha1_clone(struct sha1_context*,struct sha1_context*);

#endif /* SHA1_H_ */
