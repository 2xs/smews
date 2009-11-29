
#include "record.h"
#include "types.h"
#include "tls.h"
#include "input.h"
#include "output.h"


/* reads a TLS record header and returns size of record data */
uint16_t read_header(const uint8_t type){

	uint8_t tmp[2];

	/* get record type */
	DEV_GETC(tmp[0]);

	if(tmp[0] != type) {

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:read_header: Unexpected record type");
#endif
		return HNDSK_ERR;
	}

	/* get TLS Version */
	DEV_GETC16(tmp);

	if(tmp[1] != TLS_SUPPORTED_MAJOR && tmp[0] != TLS_SUPPORTED_MINOR){

#ifdef DEBUG
		DEBUG_MSG("\nFATAL:read_header: Unsupported or damaged TLS Version");
#endif
		return HNDSK_ERR;
	}

	DEV_GETC16(tmp);

	return UI16(tmp);

}

/* fills output buffer with record header information */
void write_header(uint8_t type, uint16_t len){

	/* set handshake message type */
	DEV_PUT(type);

	/* advertise TLS version */
	DEV_PUT(TLS_SUPPORTED_MAJOR);
	DEV_PUT(TLS_SUPPORTED_MINOR);

	/* this is the record lenght */
	DEV_PUT16_VAL(len);

}
