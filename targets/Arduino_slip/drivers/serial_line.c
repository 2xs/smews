/*
* Copyright or © or Copr. 2008, Simon Duquennoy
* 
* Author e-mail: simon.duquennoy@lifl.fr
* 
* This software is a computer program whose purpose is to design an
* efficient Web server for very-constrained embedded system.
* 
* This software is governed by the CeCILL license under French law and
* abiding by the rules of distribution of free software.  You can  use, 
* modify and/ or redistribute the software under the terms of the CeCILL
* license as circulated by CEA, CNRS and INRIA at the following URL
* "http://www.cecill.info". 
* 
* As a counterpart to the access to the source code and  rights to copy,
* modify and redistribute granted by the license, users are provided only
* with a limited warranty  and the software's author,  the holder of the
* economic rights,  and the successive licensors  have only  limited
* liability. 
* 
* In this respect, the user's attention is drawn to the risks associated
* with loading,  using,  modifying and/or developing or reproducing the
* software by the user in light of its specific status of free software,
* that may mean  that it is complicated to manipulate,  and  that  also
* therefore means  that it is reserved for developers  and  experienced
* professionals having in-depth computer knowledge. Users are therefore
* encouraged to load and test the software's suitability as regards their
* requirements in conditions enabling the security of their systems and/or 
* data to be ensured and,  more generally, to use and operate it in the 
* same conditions as regards security.
* 
* The fact that you are presently reading this means that you have had
* knowledge of the CeCILL license and that you accept its terms.
*/

#include "types.h"
#include <avr/interrupt.h>

//#define F_CPU	16000000 // Arduino atmel328p
//#define BAUD 38400 // Arduino
#define BAUD 57600 // Arduino
//#define BAUD_PRESCALE (((F_CPU/16)/BAUD)-1) // For asynchronous normal 
#define BAUD_PRESCALE (((F_CPU/8)/BAUD)-1) // For asynchronous double speed

#define TIMEOUT 50

volatile serial_line_t serial_line;
enum slip_state_e{slip_in,slip_escaped};
static enum slip_state_e slip_state;

/*-----------------------------------------------------------------------------------*/
void dev_init(void) {
	UBRR0H = (unsigned char)(BAUD_PRESCALE>>8);
	UBRR0L = (unsigned char)BAUD_PRESCALE;
//	UCSR0A &= ~(1 << U2X0); // Arduino Normal asynchronous
	UCSR0A |= (1 << U2X0); // Arduino Double speed asynchronous

	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Turn on the transmission and reception circuitry
	UCSR0C = (1 <<UCSZ00) | (1 <<UCSZ01);

	serial_line.writePtr = serial_line.buffer;
	serial_line.readPtr = NULL;
	slip_state = slip_in;

	return;
}

/*-----------------------------------------------------------------------------------*/
ISR(USART_RX_vect) { // Arduino
	cli();
	unsigned char c = UDR0; // Arduino

	/* SLIP management */
	switch(c) {
		case SLIP_END:
			goto uart_irq_end;
		case SLIP_ESC:
			slip_state = slip_escaped;
			goto uart_irq_end;
		case SLIP_ESC_END:
			if(slip_state == slip_escaped) {
				slip_state = slip_in;
				c = SLIP_END;
			}
			break;
		case SLIP_ESC_ESC:
			if(slip_state == slip_escaped) {
				slip_state = slip_in;
				c = SLIP_ESC;
			}
			break;
		default:
			break;
	}

	if(serial_line.writePtr != NULL) {
		if(serial_line.readPtr == NULL)
			serial_line.readPtr = serial_line.writePtr;
		*serial_line.writePtr++ = c;
		if(serial_line.writePtr == serial_line.buffer + INBUF_SIZE)
			serial_line.writePtr = serial_line.buffer;
		if(serial_line.writePtr == serial_line.readPtr)
			serial_line.writePtr = NULL;
	}
uart_irq_end:
	/* Reenable this interrupt */
	sei();
}

/*-----------------------------------------------------------------------------------*/
static unsigned char read_byte_from_buffer(void) {
	int16_t c;
	cli();

	if(serial_line.writePtr == NULL)
		serial_line.writePtr = serial_line.readPtr;
	c = *serial_line.readPtr++;
	if(serial_line.readPtr == serial_line.buffer + INBUF_SIZE)
		serial_line.readPtr = serial_line.buffer;
	if (serial_line.readPtr == serial_line.writePtr)
		serial_line.readPtr = NULL;

	sei();
	return  c;
}

/*-----------------------------------------------------------------------------------*/
int16_t dev_get(void) {
	uint32_t last_time = global_timer;
	while(serial_line.readPtr == NULL && global_timer-last_time<TIMEOUT);
	if(serial_line.readPtr == NULL) {
		return -1;
	} else {
		return read_byte_from_buffer();
	}
}

/*-----------------------------------------------------------------------------------*/
void serial_line_write(unsigned char value) {
	/* Wait for empty transmit buffer */
	while(!((UCSR0A) & (1<<UDRE0)));
	/* Put data into buffer, sends the data */
	UDR0 = value;
}

/*-----------------------------------------------------------------------------------*/
void dev_put(unsigned char byte) {
	if(byte==SLIP_END){
		serial_line_write(SLIP_ESC);
		serial_line_write(SLIP_ESC_END);
	} else if(byte==SLIP_ESC){
		serial_line_write(SLIP_ESC);
		serial_line_write(SLIP_ESC_ESC);
	} else {
		serial_line_write(byte);
	}
}
