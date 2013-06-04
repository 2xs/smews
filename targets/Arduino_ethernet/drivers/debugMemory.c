#include <stdint.h>

#include <avr/io.h>
#include <util/delay.h>

#include "serial.h"

//#include "debug_arduino.h"

//#define HEAP_SIZE 	256
//#define STACK_SIZE	128

#define DEBUG_ARDUINO

//	uint32_t heap[HEAP_SIZE/4+1];
//	uint8_t stack[STACK_SIZE];

#ifdef DEBUG_ARDUINO

#define FREE_MARK	0x40

extern uint16_t __bss_start;             // highest stack address
extern uint16_t __bss_end;               // lowest stack address
extern uint16_t __stack;                 // highest stack address

void init_debug_serial(void)
{
	uint8_t baudrate = 103;
	/* Set baud rate */
	UBRR0H = (unsigned char)(baudrate>>8);
	UBRR0L = (unsigned char)baudrate;

	/* Enable transmitter */
	UCSR0B = (1<<TXEN0);

	/* Set frame format */
	UCSR0C = 0x06;
}

void send_debug(uint8_t c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}


void init_tab8(uint8_t *ptr, uint16_t size, uint8_t magic_number)
{
	uint16_t i;
	for(i=0; i<size; i++)
		*ptr++ = magic_number;
}

void init_tab16(uint16_t *ptr, uint16_t size, uint16_t magic_number)
{
	uint16_t i;
	for(i=0; i<size; i++)
		*ptr++ = magic_number;
}

void init_tab32(uint32_t *ptr, uint16_t size, uint32_t magic_number)
{
	uint16_t i;
	for(i=0; i<size; i++)
		*ptr++ = magic_number;
}

uint16_t stack_free(void)    // unused stack since last call
{
	uint8_t flag = 1;
	uint16_t i, free = 0;
	uint8_t *mp = &__bss_end;

    for( i = SP - (uint16_t)&__bss_end + 1; i; i--)
	{
		if( *mp != FREE_MARK )
			flag = 0;
		free += flag;
		*mp++ = FREE_MARK;
	}
	return free;
}


void dump_stack(void)
{
	uint16_t i;
	uint8_t j = 0;
	uint8_t *ptr;
	ptr = &__stack;
//	ptr = 0x08FF;
	//for(i=__stack; i>__bss_start; i--)
/*
	send('\n');
	send('\n');
	send(((uint16_t)&__stack)>>8);
	send(((uint16_t)&__stack)&0x00FF);
	send('\n');
	send(((uint16_t)&__bss_start)>>8);
	send(((uint16_t)&__bss_start)&0x00ff);
	send('\n');
	send(((uint16_t)&__bss_end)>>8);
	send(((uint16_t)&__bss_end)&0x00ff);
*/
	sprintf(stringBuffer,"--MEM-S",SP);
	displayString(stringBuffer);
	for(i=((uint16_t)&__stack); i>(uint16_t)&__bss_start; i--)
	{
		if(j==0)
		{
			send_debug(((uint16_t)ptr)>>8);
			send_debug(((uint16_t)ptr)&0x00ff);
		}
		send_debug(*ptr--);
		j++;
		if(j==16)
		{
			j=0;
		}
	}
	sprintf(stringBuffer,"--MEM-E",SP);
	displayString(stringBuffer);
}

#endif

