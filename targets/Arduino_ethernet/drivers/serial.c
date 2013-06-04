#include <string.h>
#include <stdint.h>
#include <avr/io.h>

#include "memory.h"
#include <stdio.h>
#include "serial.h"

volatile char stringBuffer[80];
//char *stringBuffer=NULL;

void serialInit(void)
{
	uint8_t baudrate = 103;
	/* Set baud rate */
	UBRR0H = (unsigned char)(baudrate>>8);
	UBRR0L = (unsigned char)baudrate;

	/* Enable transmitter */
	UCSR0B = (1<<TXEN0) | (1<<RXEN0);

	/* Set frame format */
	UCSR0C = 0x06;
/*
	stringBuffer = mem_alloc(80);
	if(stringBuffer == NULL)
	{
		char BUFF[40];
		sprintf(BUFF, "OUPS\n");
		displayString(BUFF);
	}
*/	
}

void putchr(unsigned char c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

void displayString(char *string) {
  uint8_t i;
  for (i=0;i<strlen(string);i++) {
    putchr(string[i]);
  }
}

