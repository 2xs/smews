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

#define TIMEOUT 50

#define REG_IME		(*(volatile unsigned short *) 0x04000208)
#define REG_IE		(*(volatile unsigned short *) 0x04000200)
#define REG_IF		(*(volatile unsigned short *) 0x04000202)

#define IT_ENABLE	0x0001
#define IT_SERIAL	0x0080
#define IT_TIMER0	0x0008
#define IT_KEY		0x1000

// Serial IO Registers
#define REG_SIOCNT        *(volatile unsigned short int *)(0x4000128)  // Serial control
#define REG_SIODATA8      *(volatile unsigned short int *)(0x400012a)  // Serial data
#define REG_RCNT          *(volatile unsigned short int *)(0x4000134)  // General IO

// UART settings
#define SIO_USE_UART      0x3000

// Baud Rate
#define SIO_BAUD_9600     0x0000
#define SIO_BAUD_38400    0x0001
#define SIO_BAUD_57600    0x0002
#define SIO_BAUD_115200   0x0003

#define SIO_CTS           0x0004
#define SIO_PARITY_ODD    0x0008
#define SIO_SEND_DATA     0x0010
#define SIO_RECV_DATA     0x0020
#define SIO_ERROR         0x0040
#define SIO_LENGTH_8      0x0080
#define SIO_USE_FIFO      0x0100
#define SIO_USE_PARITY    0x0200
#define SIO_SEND_ENABLE   0x0400
#define SIO_RECV_ENABLE   0x0800
#define SIO_REQUEST_IRQ   0x4000

#define RCNT_MODE_UART    0x0000

#define BAUD_PRESCALE SIO_BAUD_115200

volatile serial_line_t serial_line;
enum slip_state_e{slip_in,slip_escaped};
static enum slip_state_e slip_state;

/*-----------------------------------------------------------------------------------*/
void dev_init(void) {
	unsigned short GBA_Rate, GBA_Opts;

	GBA_Rate = BAUD_PRESCALE;

	REG_SIODATA8 = 'B';
	REG_RCNT     = 0;
	REG_SIOCNT   = 0;
	
	GBA_Opts = GBA_Rate | SIO_USE_UART;

	GBA_Opts |= SIO_CTS;
 	GBA_Opts |= SIO_LENGTH_8;

	REG_SIOCNT  = GBA_Opts;
	REG_SIOCNT |= SIO_SEND_ENABLE | SIO_RECV_ENABLE;
	REG_SIOCNT |= SIO_RECV_DATA   | SIO_SEND_DATA;

	REG_SIOCNT |= SIO_REQUEST_IRQ;

	serial_line.writePtr = serial_line.buffer;
	serial_line.readPtr = NULL;
	slip_state = slip_in;

	return;
}

/*-----------------------------------------------------------------------------------*/
void _int_serial_com(){
	REG_IE &= ~IT_SERIAL;

	if(REG_SIOCNT & SIO_RECV_DATA)
		return;

	int16_t c = (int16_t) REG_SIODATA8;
	
	
	/* SLIP management */
	switch(c) {
		case SLIP_END:
			return;
		case SLIP_ESC:
			slip_state = slip_escaped;
			return;
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

	/* Reenable this interrupt */
	REG_IE |= IT_SERIAL;
}

/*-----------------------------------------------------------------------------------*/
static inline unsigned char read_byte_from_buffer(void) {
	int16_t c;
	REG_IE &= ~IT_SERIAL;

	if(serial_line.writePtr == NULL)
		serial_line.writePtr = serial_line.readPtr;
	c = *serial_line.readPtr++;
	if(serial_line.readPtr == serial_line.buffer + INBUF_SIZE)
		serial_line.readPtr = serial_line.buffer;
	if (serial_line.readPtr == serial_line.writePtr)
		serial_line.readPtr = NULL;

	REG_IE |= IT_SERIAL;
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
int32_t serial_line_write(unsigned char value) {
	while(REG_SIOCNT & SIO_SEND_DATA);
	REG_SIODATA8 = value;
	REG_SIOCNT |= SIO_RECV_DATA | SIO_SEND_DATA;
	return 1;
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
