

#ifndef CYGWIN_TARGET_TYPES_H
#define CYGWIN_TARGET_TYPES_H

#define DEV_MTU 1500

#include <stdint.h>

extern void hardware_init(void);
extern unsigned char dev_get(void);
extern void dev_put(unsigned char byte);
extern char dev_prepare_output(uint16_t len);
extern unsigned char dev_data_to_read(void);

#define HARDWARE_INIT hardware_init()
#define HARDWARE_STOP
#define TIME_MILLIS 0
#define DEV_GET(c) {(c) = dev_get();}
#define DEV_PUT(c) dev_put(c)
#define DEV_PREPARE_OUTPUT(length) if(!dev_prepare_output(length)) return
#define DEV_OUTPUT_DONE
#define DEV_DATA_TO_READ dev_data_to_read()
 

#define SMEWS_WAITING
#define SMEWS_SENDING
#define SMEWS_RECEIVING
#define SMEWS_ENDING

/* Progmem macros */

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

#define CONST_WRITE_NBYTES(dst,src,len) memcpy(dst,src,len)

extern int strcmp(const char *s1, const char *s2);
#define STRCMP_P(s,sconst) strcmp(s,sconst)

/* Endianness */

#define ENDIANNESS LITTLE_ENDIAN

/* Smews configuration */

#define MAX_TIMERS 8
#define MAX_CONNECTIONS 32
#define OUTPUT_BUFFER_SIZE 128
#define ARGS_BUFFER_SIZE 128

#endif /* AVR_TARGET_TYPES_H */
