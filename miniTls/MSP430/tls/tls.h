
#ifndef TLS_H_
#define TLS_H_

#include "md5.h"
#include "sha1.h"


void /*TODO add inline */copy_bytes(const uint8_t*,uint8_t*,uint16_t,uint16_t);

void tls_send_app_data(uint16_t);
uint8_t tls_get_app_data();
uint8_t tls_handshake();


/* general return values */
#define HNDSK_ERR 0
#define HNDSK_OK 1


/* usual TLS headers & constants */

/* TLS version */
#define TLS_SUPPORTED_MAJOR 0x03
#define TLS_SUPPORTED_MINOR 0x01

/* lengths for Hello,Certificate and HelloDone handshake records */
#define TLS_HELLO_RECORD_LEN 50
#define TLS_CERT_RECORD_LEN 482
#define TLS_HDONE_RECORD_LEN 4

/* record length is always 5*/
#define TLS_RECORD_HEADER_LEN 5


/* for finished message calculation */
#define SERVER 0
#define CLIENT 1

/* ciphersuites of interest  */

/* supported by Firefox */
#define TLS1_ECDH_ECDSA_WITH_RC4_128_SHA 0xc002
#define TLS1_ECDHE_ECDSA_WITH_RC4_128_SHA 0xc007
#define TLS1_ECDH_ECDSA_WITH_AES_128_CBC_SHA 0xc004

/* supported by IE, Chrome, Firefox */
#define TLS1_ECDHE_ECDSA_WITH_AES_128_CBC_SHA 0xc009

/* extension common values */
#define TLS_EXTENSION_SECP256R1 0x17
#define TLS_EXT_POINT_FORMATS 0x0b
#define TLS_EXT_EC 0x0a
#define TLS_EXT_LEN 0x06
/* Compression Method - no compression*/
#define TLS_COMPRESSION_NULL 0x00

/* server certificate and private key */
extern const uint8_t server_cert[];
extern const uint8_t ec_priv_key_256[];

#define SECRET_LEN 48 /* master secret len */
#define PMS_LEN 32	  /* pre-master secret len */

/* label size for MS and Session Keys computation */
#define PRF_LABEL_SIZE 13

/* these are current sizes for the compiled ciphersuite */
#define CIPHER_KEYSIZE 	16 		/* RC4 */
#define MAC_KEYSIZE 	20    	/* SHA1 */
#define KEY_MATERIAL_LEN 72		/* Material Len for Session Keys */

#define ENCODE 1
#define DECODE 2

/* maximum allocated buffer size for global buffer*/
#define BUFFER_SIZE 1024

#define RANDOM_SIZE 32
#define FINISHED_MSG_LEN 16
#define FIN_SEED_LEN 51

/* maximum sizes for input and output bufers */
#define MAX_OUTPUT BUFFER_SIZE - MAC_KEYSIZE - START_BUFFER /* application layer must limit output to this value */
#define MAX_INPUT BUFFER_SIZE - START_BUFFER /* tcp must limit to this size */

/* position in global buffer where the actual data should start in receive or send*/
#define START_BUFFER 13


/* ContentType */
enum {
        TLS_CONTENT_TYPE_CHANGE_CIPHER_SPEC = 20,
        TLS_CONTENT_TYPE_ALERT = 21,
        TLS_CONTENT_TYPE_HANDSHAKE = 22,
        TLS_CONTENT_TYPE_APPLICATION_DATA = 23
};

/* HandshakeType */
enum {
        TLS_HANDSHAKE_TYPE_HELLO_REQUEST = 0,
        TLS_HANDSHAKE_TYPE_CLIENT_HELLO = 1,
        TLS_HANDSHAKE_TYPE_SERVER_HELLO = 2,
        TLS_HANDSHAKE_TYPE_CERTIFICATE = 11,
        TLS_HANDSHAKE_TYPE_SERVER_KEY_EXCHANGE = 12,
        TLS_HANDSHAKE_TYPE_CERTIFICATE_REQUEST = 13,
        TLS_HANDSHAKE_TYPE_SERVER_HELLO_DONE = 14,
        TLS_HANDSHAKE_TYPE_CERTIFICATE_VERIFY = 15,
        TLS_HANDSHAKE_TYPE_CLIENT_KEY_EXCHANGE = 16,
        TLS_HANDSHAKE_TYPE_FINISHED = 20
};


/* needed for sequence number manipulation */
union long_byte {
	uint8_t bytes[8];
	uint64_t long_int;
};

struct tls_context{

	/* The sequence
	   number ensures that attempts to delete or reorder messages will be
	   detected.  Since sequence numbers are 64 bits long, they should never
	   overflow. */
	union long_byte encode_seq_no;
	union long_byte decode_seq_no;

	uint8_t client_mac[MAC_KEYSIZE]; /* client write mac secret */
	uint8_t server_mac[MAC_KEYSIZE]; /* server write mac secret */
	uint8_t client_key[CIPHER_KEYSIZE]; /* client write key for symmetric alg */
	uint8_t server_key[CIPHER_KEYSIZE]; /* server write key for symmetric alg */

	/* uint8_t ccs_sent:1;  flag indicating if we have sent CCS message */
	uint8_t ccs_recv:1; /* flag indicating if we have recv CCS message */

};


/* structure for random value */
union int_char{
    uint32_t lfsr_int[8];
    uint8_t lfsr_char[32];

};

#endif /* TLS_H_ */



