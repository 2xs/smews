
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

#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:read_header: Unexpected record type or not TLS v1.0 compatible browser");
#endif
		return HNDSK_ERR;
	}

	/* get TLS Version */
	DEV_GETC(tmp[S0]);
	DEV_GETC(tmp[S1]);

	if(tmp[1] != TLS_SUPPORTED_MAJOR && tmp[0] != TLS_SUPPORTED_MINOR){

#ifdef DEBUG_TLS
		DEBUG_MSG("FATAL:read_header: Unsupported or damaged TLS Version");
#endif
		return HNDSK_ERR;
	}

	/* get record length */
	DEV_GETC(tmp[S0]);
	DEV_GETC(tmp[S1]);

#ifdef DEBUG_TLS_DEEP
	DEBUG_VAR(UI16(tmp),"%d","Received a TLS record with size (not containing TLS record header): ");
#endif

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


/* computes MAC of a byte-array containing plain text */
static void compute_mac(struct tls_connection *tls, uint8_t type, uint8_t *buff, uint16_t len, uint8_t operation, uint8_t* r){


	int16_t i;
	uint8_t *seqno = (operation == DECODE ? tls->decode_seq_no.bytes : tls->encode_seq_no.bytes );
	uint8_t *mac_key = (operation == DECODE ? tls->client_mac : tls->server_mac );


	/* fill in the first 12 positions of buffer for MAC computation, up to START_BUFFER */
/*	for( i = 0 ; i < 8 ; i++)
		buff[i] = (seqno[7-i]);

	buff[8] = type;
	buff[9] = TLS_SUPPORTED_MAJOR;
	buff[10] = TLS_SUPPORTED_MINOR;
	buff[11] = len >> 8;
	buff[12] = (uint8_t)len;*/
	//todo refactoring here

	hmac_init(SHA1,mac_key,SHA1_KEYSIZE);
	/* fill in the first 12 positions of buffer for MAC computation, up to START_BUFFER */
	for( i = 0 ; i < 8 ; i++)
		hmac_update(seqno[7-i]);

	hmac_update(type);
	hmac_update(TLS_SUPPORTED_MAJOR);
	hmac_update(TLS_SUPPORTED_MINOR);
	hmac_update(len >> 8);
	hmac_update((uint8_t)len);
	//tls->record_size = len;
	//hmac_preamble(tls, operation);

	for (i = 13 ; i < len + 13; i++)
		hmac_update(buff[i]);
	hmac_finish(SHA1);
	copy_bytes(sha1.buffer,r,0,20);
	//hmac(SHA1,mac_key,MAC_KEYSIZE,buff,len + 13,r);

#ifdef DEBUG_DEEP
	PRINT_ARRAY(r,MAC_KEYSIZE,"Computed MAC of fragment :");
#endif



}

/* len - data length without MAC */
void write_record(struct tls_connection *tls, uint8_t type, uint8_t* record_buffer, uint16_t len){

	uint8_t i;
	uint8_t *startbuffer = record_buffer + START_BUFFER;


	/* computing MAC for plaintext and adding it to the end of record*/
	compute_mac(tls, type, record_buffer, len, ENCODE, (startbuffer + len) );


#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY( (startbuffer),len + 20,"Record data padded with MAC ready to be encrypted :");
#endif

	/* cipher in place the plaintext together with MAC */
	for(i = 0; i < len + 20 ; i++)
		rc4_crypt(&startbuffer[i],MODE_ENCRYPT);
	//rc4_crypt(startbuffer, 0, len + 20, MODE_ENCRYPT);

#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY( (startbuffer) ,len + 20,"Record Data after encryption :");
#endif


#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY( (startbuffer) ,len + 20,"Record Ready to be Send :");
#endif

	/* we encoded one more record, increment seq number */
	tls->encode_seq_no.long_int++;


}


/* decodes a record body by decrypting and MAC verification */
uint8_t decode_record(struct tls_connection *tls,uint8_t type,uint8_t *record_data, uint16_t len){

	uint16_t i;
	uint8_t *startbuffer = record_data + START_BUFFER;

	/* expected MAC after recalculation */
	uint8_t exp_mac[MAC_KEYSIZE];

#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY(startbuffer, len, "Record data before decryption :");
#endif

	/* decrypt record_data in place obtaining [payload + MAC] */
	for(i = 0; i < len ; i++)
			rc4_crypt(&startbuffer[i],MODE_DECRYPT);
	//rc4_crypt(startbuffer,0,len,MODE_DECRYPT);

#ifdef DEBUG_TLS_DEEP
	PRINT_ARRAY(startbuffer, len, "Record data after decryption :");
#endif


	/* compute MAC on tls payload of 16 octets (finished msg) */
	compute_mac(tls, type, record_data, 16, DECODE, exp_mac);

	for( i = 0 ; i < MAC_KEYSIZE ; i++)

		if(exp_mac[i] != startbuffer[i+16]){

#ifdef DEBUG_TLS
			DEBUG_MSG("\nBad Record MAC when reading record!");
#endif

#ifdef DEBUG_TLS_DEEP

			PRINT_ARRAY(exp_mac,MAC_KEYSIZE,"Expected :");
			PRINT_ARRAY( (startbuffer + 16 ),MAC_KEYSIZE,"Found :");

#endif

			return HNDSK_ERR;

		}

	tls->decode_seq_no.long_int++;
	return HNDSK_OK;

}


