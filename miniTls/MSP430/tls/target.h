


#ifndef AVR_TARGET_H_
#define AVR_TARGET_H_


#include "stdint.h"
#include <stdio.h>




/* normal tls handshake debug messages*/
//#define DEBUG
/* gets deep tls protocol information such as record encoding/decoding */
/*#define DEBUG_DEEP */
/*debug ECC operations */
/*#define DEBUG_ECC  */

#ifdef DEBUG

#define DEBUG_MSG(x) printf("%s",x)
#define DEBUG_VAR(x,format) printf(format,x)


#define PRINT_ARRAY(x,len,msg) { \
		uint16_t i; \
		printf("%s",msg); \
		for(i = 0 ; i < len ; i++) printf("%02x",x[i]);\
		printf("\n");\
	}
#endif



/* Endianness */
#define ENDIANNESS LITTLE_ENDIAN



/* Wn: weight of the n_th byte of a 32 bits integer */
#if ENDIANNESS == LITTLE_ENDIAN
        #define W0 3
        #define W1 2
        #define W2 1
        #define W3 0
#else
        #define W0 0
        #define W1 1
        #define W2 2
        #define W3 3
#endif
/* Sn: weight of the n_th byte of a 16 bits integer */
#if ENDIANNESS == LITTLE_ENDIAN
        #define S0 1
        #define S1 0
#else
        #define S0 0
        #define S1 1
#endif




/* Cast variable as uint16_t or uint32_t */
#define UI16(x) (*((uint16_t*)(x)))
#define UI32(x) (*((uint32_t*)(x)))


#ifndef DEV_PUT16
	#define DEV_PUT16(c) {DEV_PUT((unsigned char)((c) >> 8)); \
		DEV_PUT((unsigned char)(c));}
#endif

#ifndef DEV_PUT32
	#define DEV_PUT32(c) {DEV_PUT((unsigned char)((c) >> 24)); \
		DEV_PUT((unsigned char)((c) >> 16)); \
		DEV_PUT((unsigned char)((c) >> 8)); \
		DEV_PUT((unsigned char)(c));}
#endif



#define DEV_GETC(c) { int16_t getc; \
		DEV_GET(getc); \
		if(getc == -1) return 1; \
		c = getc;} \

#define DEV_GETC16(c) { int16_t getc; \
		DEV_GET(getc); \
		if(getc == -1) return 1; \
		((unsigned char *)(c))[S0] = getc; \
		DEV_GET(getc); \
		if(getc == -1) return 1; \
		((unsigned char *)(c))[S1] = getc;}

#define DEV_GETC32(c) { int16_t getc; \
		 DEV_GET(getc); \
		 if(getc == -1) return 1; \
		 ((unsigned char *)(c))[W0] = getc; \
		 DEV_GET(getc); \
		 if(getc == -1) return 1; \
		 ((unsigned char *)(c))[W1] = getc; \
		 DEV_GET(getc); \
		 if(getc == -1) return 1; \
		 ((unsigned char *)(c))[W2] = getc; \
		 DEV_GET(getc); \
		 if(getc == -1) return 1; \
		 ((unsigned char *)(c))[W3] = getc; }

#define DEV_GETN(a,len){ \
		uint16_t i; \
		for(i = 0; i < len; i++) \
			DEV_GET(a[i]); }




/* Network drivers - definition in hardware.c */
/*extern void dev_init(void);
extern uint32_t get_time(void);
extern void dev_init(void);*/

extern uint8_t dev_get();
//extern unsigned char dev_data_to_read();
//extern void dev_prepare_output();
//extern void dev_output_done(void);
/*
extern void check(int test,const char *message);
extern void wait_input(void);
extern void connect_socket();
extern unsigned char net_receive();
*/

#define INBUF_SIZE 330
#define OUTBUF_SIZE 600

extern uint8_t in_buffer[INBUF_SIZE];
extern uint8_t out_buffer[OUTBUF_SIZE];


extern int in_curr;
extern int out_curr;

/*
extern int tun_fd;
extern int in_nbytes;
extern struct timeval tv;
*/

#define DEV_MTU 1500
#define HARDWARE_INIT 
#define HARDWARE_STOP

#define TIME_MILLIS get_time()
#define DEV_GET(c) {(c) = dev_get();}
#define DEV_PUT(c) {out_buffer[out_curr++]=(c);}
/*#define DEV_PUTN(ptr,n) {memcpy(out_buffer + out_curr,ptr,n); out_curr+=n;}
#define DEV_PUTN_CONST(ptr,n) {memcpy(out_buffer + out_curr,ptr,n); out_curr+=n;}
*/
#define DEV_PREPARE_OUTPUT //dev_prepare_output()
#define DEV_OUTPUT_DONE //dev_output_done()
/*#define DEV_DATA_TO_READ dev_data_to_read()
#define DEV_WAIT {wait_input();}
#define NEW_APP_DATA net_receive()
*/
/* Progmem macros */

/* this macro's are from x86 target. const now goes to ram */
#define CONST_VOID_P_VAR const void *
#define CONST_VAR(type,name) type const name
#define PERSISTENT_VAR(type,name) type name

#define CONST_READ_UI8(x) ((uint8_t)*((uint8_t*)(x)))
#define CONST_READ_UI16(x) ((uint16_t)*((uint16_t*)(x)))
#define CONST_READ_UI32(x) ((uint32_t)*((uint32_t*)(x)))
#define CONST_READ_ADDR(x) ((void*)*((void**)(x)))

#define CONST_UI8(x) ((uint8_t)(x))
#define CONST_UI16(x) ((uint16_t)(x))
#define CONST_UI32(x) ((uint32_t)(x))
#define CONST_ADDR(x) ((void*)(x))

//#define CONST_WRITE_NBYTES(dst,src,len) memcpy(dst,src,len)


#endif
