#ifndef __DEV_H__
#define __DEV_H__

#include "avrlibtypes.h"

#define MAX_PACKET	12
//#define MAX_PACKET	36

void dev_prepare_output(int size);
void dev_put(unsigned char byte);
void dev_output_done(void);
char dev_get(void);
void dev_init(void);
unsigned char data_available(void);

void hardware_init(void);

typedef struct packet_s {
	volatile u16 packetPtr;
	volatile u16 nextPtr;
	volatile u16 size;
	volatile u08 mac_src[6];
} packet_t;

#endif

