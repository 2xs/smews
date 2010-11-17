/*
* Copyright or © or Copr. 2010, Simon Duquennoy, Thomas Soete
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
#include "SimpleLib/serial.h"
#include "slip.h"

#define TIMEOUT 50

volatile serial_line_t serial_line;
enum slip_state_e{slip_in,slip_escaped};
static enum slip_state_e slip_state;

/*-----------------------------------------------------------------------------------*/
void slip_init(void) {
	SERIAL_INIT();
	SERIAL_SETBAUD(115200);
	SERIAL_ENABLE_INTERRUPT(SERIAL_INT_RX);

	serial_line.writePtr = serial_line.buffer;
	serial_line.readPtr = NULL;
	slip_state = slip_in;
}

/*-----------------------------------------------------------------------------------*/
SERIAL_INTERRUPT_HANDLER(void) {
	// Check if interrupt is pending
	if(!SERIAL_CHECK_INTERRUPT())
		return;

	// While some data to read
	while (SERIAL_DATA_TO_READ()) {
		char c = SERIAL_GETCHAR();

		/* SLIP management */
		switch (c) {
			case SLIP_END:
				return;
			case SLIP_ESC:
				slip_state = slip_escaped;
				return;
			case SLIP_ESC_END:
				if (slip_state == slip_escaped) {
					slip_state = slip_in;
					c = SLIP_END;
				}
				break;
			case SLIP_ESC_ESC:
				if (slip_state == slip_escaped) {
					slip_state = slip_in;
					c = SLIP_ESC;
				}
				break;
			default:
				break;
		}

		/* Buffer management */
		if (serial_line.readPtr == NULL)
			serial_line.readPtr = serial_line.writePtr;
		*serial_line.writePtr++ = c;
		if (serial_line.writePtr == serial_line.buffer + INBUF_SIZE)
			serial_line.writePtr = serial_line.buffer;
		if (serial_line.writePtr == serial_line.readPtr)
			serial_line.writePtr = NULL;
	}
}


/*-----------------------------------------------------------------------------------*/
int16_t dev_get(void) {
	uint32_t last_time = TIME_MILLIS;
	while (serial_line.readPtr == NULL && (TIME_MILLIS-last_time)<TIMEOUT);
	if (serial_line.readPtr == NULL) {
		return -1;
	} else {
		unsigned char c;
		if (serial_line.writePtr == NULL)
			serial_line.writePtr = serial_line.readPtr;
		c = *serial_line.readPtr++;
		if (serial_line.readPtr == serial_line.buffer + INBUF_SIZE)
			serial_line.readPtr = serial_line.buffer;
		if (serial_line.readPtr == serial_line.writePtr)
			serial_line.readPtr = NULL;
		return c;
	}
}

/*-----------------------------------------------------------------------------------*/
void dev_put(unsigned char byte) {
	if(byte==SLIP_END){
		SERIAL_PUTCHAR(SLIP_ESC);
		SERIAL_PUTCHAR(SLIP_ESC_END);
	} else if(byte==SLIP_ESC){
		SERIAL_PUTCHAR(SLIP_ESC);
		SERIAL_PUTCHAR(SLIP_ESC_ESC);
	} else {
		SERIAL_PUTCHAR(byte);
	}
}

/* for debug purposes. allows to use printf. */
// /*-----------------------------------------------------------------------------------*/
// void putchar(unsigned char byte) {
//         static char in_frame = 0;
//         if(!in_frame) {
// 	      serial_line_write(SLIP_END);
// 	      in_frame = 1;
//         }
//         if(byte == '\n') {
// 	      serial_line_write(SLIP_END);
// 	      in_frame = 0;
//         } else {
// 	      dev_put(byte);
//         }
//         
// }
