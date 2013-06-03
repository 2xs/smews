/*
* Copyright or © or Copr. 2008, Simon Duquennoy
* Copyright or © or Copr. 2013, Thomas Vantroys
* 
* Author e-mail: thomas.vantroys@univ-lille1.fr
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

#ifndef __TARGET_H__
#define __TARGET_H__

#include <stdint.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "serial_line.h"

extern volatile uint32_t global_timer;

/* Drivers interface */

#define HARDWARE_INIT hardware_init()
#define HARDWARE_STOP
#define TIME_MILLIS global_timer
#define DEV_GET(c) {(c) = dev_get();}
#define DEV_PUT(c) dev_put(c)
#define DEV_PREPARE_OUTPUT(length) serial_line_write(SLIP_END);
#define DEV_OUTPUT_DONE serial_line_write(SLIP_END);
#define DEV_DATA_TO_READ (serial_line.readPtr != NULL)

/* Smews states */

#define SMEWS_WAITING
#define SMEWS_RECEIVING
#define SMEWS_SENDING
#define SMEWS_ENDING 

/* Const and persistent access macros */

#define CONST_VOID_P_VAR PGM_VOID_P
#define CONST_VAR(type,name) type const name PROGMEM
#define PERSISTENT_VAR(type,name) type name
#define CONST_WRITE_NBYTES(dst,src,len) strncpy(dst,src,len)

#define CONST_READ_UI8(x) pgm_read_byte_near(x)
#define CONST_READ_UI16(x) pgm_read_word_near(x)
#define CONST_READ_UI32(x) pgm_read_dword_near(x)
#define CONST_READ_ADDR(x) ((void*)pgm_read_word_near(x))

#define CONST_UI8(x) CONST_READ_UI8(&(x))
#define CONST_UI16(x) CONST_READ_UI16(&(x))
#define CONST_UI32(x) CONST_READ_UI32(&(x))
#define CONST_ADDR(x) CONST_READ_ADDR(&(x))

/* Endianness */

#define ENDIANNESS LITTLE_ENDIAN

/* Context switching */
#define BACKUP_CTX(sp) \
		{\
			uint16_t temp = SP;\
			sp[0] = (void *)temp;\
		}

#define RESTORE_CTX(sp) \
		{\
			uint16_t temp = (uint16_t)sp[0];\
			SP = temp; \
		}
 
#define PUSHREGS \
	({ \
	 asm volatile ( \
		 "push r0 \n\t" \
		 "push r1 \n\t" \
		 "push r2 \n\t" \
		 "push r3 \n\t" \
		 "push r4 \n\t" \
		 "push r5 \n\t" \
		 "push r6 \n\t" \
		 "push r7 \n\t" \
		 "push r8 \n\t" \
		 "push r9 \n\t" \
		 "push r10 \n\t" \
		 "push r11 \n\t" \
		 "push r12 \n\t" \
		 "push r13 \n\t" \
		 "push r14 \n\t" \
		 "push r15 \n\t" \
		 "push r16 \n\t" \
		 "push r17 \n\t" \
		 "push r18 \n\t" \
		 "push r19 \n\t" \
		 "push r20 \n\t" \
		 "push r21 \n\t" \
		 "push r22 \n\t" \
		 "push r23 \n\t" \
		 "push r24 \n\t" \
		 "push r25 \n\t" \
		 "push r26 \n\t" \
		 "push r27 \n\t" \
		 "push r28 \n\t" \
		 "push r29 \n\t" \
		 "push r30 \n\t" \
		 "push r31 \n\t" \
		);\
	 })

#define POPREGS \
	({ \
	 asm volatile ( \
		 "pop r31 \n\t" \
		 "pop r30 \n\t" \
		 "pop r29 \n\t" \
		 "pop r28 \n\t" \
		 "pop r27 \n\t" \
		 "pop r26 \n\t" \
		 "pop r25 \n\t" \
		 "pop r24 \n\t" \
		 "pop r23 \n\t" \
		 "pop r22 \n\t" \
		 "pop r21 \n\t" \
		 "pop r20 \n\t" \
		 "pop r19 \n\t" \
		 "pop r18 \n\t" \
		 "pop r17 \n\t" \
		 "pop r16 \n\t" \
		 "pop r15 \n\t" \
		 "pop r14 \n\t" \
		 "pop r13 \n\t" \
		 "pop r12 \n\t" \
		 "pop r11 \n\t" \
		 "pop r10 \n\t" \
		 "pop r9 \n\t" \
		 "pop r8 \n\t" \
		 "pop r7 \n\t" \
		 "pop r6 \n\t" \
		 "pop r5 \n\t" \
		 "pop r4 \n\t" \
		 "pop r3 \n\t" \
		 "pop r2 \n\t" \
		 "pop r1 \n\t" \
		 "pop r0 \n\t" \
		); \
	})


/* Smews configuration */
#define OUTPUT_BUFFER_SIZE 256
#define ALLOC_SIZE 768
#define STACK_SIZE 256

/* For automatic test purpose */
#define TEST_ARRAY_SIZE	128

#endif /* __TARGET_H__ */
