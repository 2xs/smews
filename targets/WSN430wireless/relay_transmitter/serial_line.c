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
#include "serial_line.h"
#include "target.h"

#define UART1_PIN_RX 7
#define UART1_PIN_TX 6

#define UART1_BIT_RX (1 << UART1_PIN_RX)
#define UART1_BIT_TX (1 << UART1_PIN_TX)

#define TIMEOUT 5000
#define TIMETOWAIT 500

volatile serial_line_t serial_line;

/*-----------------------------------------------------------------------------------*/
void dev_init(void) {
	//Init of MSP430 Usart1 pins
	P3SEL |= (UART1_BIT_RX | UART1_BIT_TX);

	//Init of USART1 Module
	U1ME  |= UTXE1|URXE1;           //Enable USART1 transmiter and receiver (UART mode)

	U1CTL  = SWRST;                 //reset
	U1CTL  = CHAR;                  //init & release reset

	U1TCTL = SSEL_SMCLK|TXEPT;      //use SMCLK 
	U1RCTL = 0;

	// 115200 @ SMCLK 1MHz
	#define U1BR1_INIT        0
	#define U1BR0_INIT        0x08
	#define U1MCTL_INIT       0x6d

	U1BR1  = U1BR1_INIT;
	U1BR0  = U1BR0_INIT;
	U1MCTL = U1MCTL_INIT;
	
	U1IE  |= URXIE1;

	serial_line.writePtr = serial_line.buffer;
	serial_line.readPtr = NULL;

	return;
}

/*-----------------------------------------------------------------------------------*/
/* Serial line handler */
interrupt (USART1RX_VECTOR) usart1irq( void ) {
	volatile int16_t c;
	uint32_t last_time = global_timer; /* To be deleted */
	LED_ON(LED_GREEN); /* Rx */
	while(global_timer-last_time<TIMETOWAIT); /* To be deleted */
	/* Check status register for receive errors. */
	if(URCTL1 & RXERR) {
		/* Clear error flags by forcing a dummy read. */
		c = RXBUF1;
		return;
	} else {
		c = (int16_t) U1RXBUF;
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
	/* TODO Envoie en radio */
	LED_OFF(LED_GREEN); /* End of Rx */
}

/*-----------------------------------------------------------------------------------*/
static unsigned char read_byte_from_buffer() {
	unsigned char c;
	if(serial_line.writePtr == NULL)
		serial_line.writePtr = serial_line.readPtr;
	c = *serial_line.readPtr++;
	if(serial_line.readPtr == serial_line.buffer + INBUF_SIZE)
		serial_line.readPtr = serial_line.buffer;
	if (serial_line.readPtr == serial_line.writePtr)
		serial_line.readPtr = NULL;
	return c;
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
int32_t serial_line_write(unsigned char value) {
	U1TXBUF = value;
	while ((U1TCTL & TXEPT) != TXEPT);
	return 1;
}

/*-----------------------------------------------------------------------------------*/
void dev_put(unsigned char byte) {
  uint32_t last_time = global_timer;
  LED_ON(LED_GREEN); /* Tx */
  while(global_timer-last_time<TIMETOWAIT);
  serial_line_write(byte);
  LED_OFF(LED_GREEN); /* End of Tx */
}
