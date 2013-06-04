#ifndef __DEV_H__
#define __DEV_H__


#define MAX_PACKET	12
//#define MAX_PACKET	36

void dev_prepare_output(uint16_t size);
void dev_put(unsigned char byte);
void dev_output_done(void);
char dev_get(void);
void dev_init(void);
unsigned char data_available(void);

void hardware_init(void);

typedef struct packet_s {
	volatile uint16_t packetPtr;
	volatile uint16_t nextPtr;
	volatile uint16_t size;
	volatile uint8_t mac_src[6];
} packet_t;

#endif

