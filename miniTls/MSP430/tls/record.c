/* This is an implementation of TLS record layer encoding/decoding */

#include "record.h"

extern struct tls_context tls;
static void compute_mac(uint8_t, uint8_t *, uint16_t , uint8_t, uint8_t*);


/* constructs a record and send it to TCP for transmission */
/* type - record type (CCS, ALRT, HNDSHK or APP)
 * record_buffer - record body
 * len - length of the record body
 */
void write_record(uint8_t type,uint8_t* record_buffer, uint16_t len){

	uint16_t i;
	uint8_t *startbuffer = record_buffer + START_BUFFER;

	write_header(type,len + MAC_KEYSIZE);


	/* computing MAC for plaintext and adding it to the end of record*/
	compute_mac(type, record_buffer, len, ENCODE, (startbuffer + len) );


#ifdef DEBUG_DEEP
	PRINT_ARRAY( (startbuffer),len + 20,"Record data padded with MAC ready to be encrypted :");
#endif
	/* cipher in place the plaintext together with MAC */
	rc4_crypt(startbuffer, 0, len + 20, MODE_ENCRYPT);

#ifdef DEBUG_DEEP
	PRINT_ARRAY( (startbuffer) ,len + 20,"Record Data after encryption :");
#endif


#ifdef DEBUG_DEEP
	PRINT_ARRAY( (startbuffer) ,len + 20,"Record Ready to be Send :");
#endif

	/* we encoded one more record, increment seq number */
	tls.encode_seq_no.long_int++;


	/* send to TCP */
	for( i = 0 ; i < len + 20; i++)
		DEV_PUT(startbuffer[i]);



}

/* parse a TLS Record Header; return number of bytes of record data */
/* type can be HANDSHAKE, APP or CCS . TODO : add ALRT */
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
	DEV_PUT16(len);

}

/* decodes a record body by decrypting, recalculating and comparing MAC */
uint8_t decode_record(uint8_t type,uint8_t *record_data, uint16_t len){

	uint16_t i;
	uint8_t *startbuffer = record_data + START_BUFFER;
	uint16_t data_len;

	/* expected MAC after recalculation */
	uint8_t exp_mac[MAC_KEYSIZE];

	/* decrypt record_data obtaining payload + MAC */
	rc4_crypt(startbuffer,0,len,MODE_DECRYPT);

#ifdef DEBUG_DEEP
	PRINT_ARRAY( startbuffer ,len,"Record data after Decryption :");
#endif
	/* actual payload length */
	data_len = len - MAC_KEYSIZE;

	/* compute MAC on payload */

	compute_mac(type, record_data, data_len, DECODE, exp_mac);

	for( i = 0 ; i < MAC_KEYSIZE ; i++)

		if(exp_mac[i] != startbuffer[i+data_len]){

#ifdef DEBUG
			DEBUG_MSG("\nBad Record MAC when reading record!");
#endif

#ifdef DEBUG_DEEP

			PRINT_ARRAY(exp_mac,MAC_KEYSIZE,"Expected :");
			PRINT_ARRAY( (startbuffer + data_len ),MAC_KEYSIZE,"\t\t\tFound :");

#endif

			return HNDSK_ERR;

		}

	tls.decode_seq_no.long_int++;
	return HNDSK_OK;

}


/* computes MAC of a byte-array containing plain text */
static void compute_mac(uint8_t type, uint8_t *buff, uint16_t len, uint8_t operation, uint8_t* r){


	int16_t i;
	uint8_t *seqno = (operation == DECODE ? tls.decode_seq_no.bytes : tls.encode_seq_no.bytes );
	uint8_t *mac_key = (operation == DECODE ? tls.client_mac : tls.server_mac );

	/* fill in the first 12 positions of buffer for MAC computation, up to START_BUFFER */
	for( i = 0 ; i < 8 ; i++)
		buff[i] = seqno[7-i];

	buff[8] = type;
	buff[9] = TLS_SUPPORTED_MAJOR;
	buff[10] = TLS_SUPPORTED_MINOR;
	buff[11] = len >> 8;
	buff[12] = (uint8_t)len;

	/* put the fragment */


	hmac(SHA1,mac_key,MAC_KEYSIZE,buff,len + 13,r);

#ifdef DEBUG_DEEP
	PRINT_ARRAY(r,MAC_KEYSIZE,"Computed MAC of fragment :");
#endif



}
