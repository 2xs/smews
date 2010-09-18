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


#ifndef __TARGET_H__
#define __TARGET_H__

/* Types definition */
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned short uint16_t;
typedef short int16_t;
typedef unsigned char uint8_t;
typedef char int8_t;

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

#define CONST_VOID_P_VAR const void *
#define CONST_VAR(type,name) type const name
#define PERSISTENT_VAR(type,name) type name

#define CONST_READ_UI8(x) ((uint8_t)*((uint8_t*)(x)))
#define CONST_READ_UI16(x) ((uint16_t)*((uint16_t*)(x)))
#define CONST_READ_UI32(x) ((uint32_t)*((uint32_t*)(x)))
#define CONST_READ_ADDR(x) ((void*)*((void**)(x)))

#define CONST_UI8(x) CONST_READ_UI8(&(x))
#define CONST_UI16(x) CONST_READ_UI8(&(x))
#define CONST_UI32(x) CONST_READ_UI32(&(x))
#define CONST_ADDR(x) CONST_READ_ADDR(&(x))

/* Endianness */

#define ENDIANNESS LITTLE_ENDIAN

/* Context switching */
/* To be done */

#define BACKUP_CTX(sp)
		
#define RESTORE_CTX(sp)

#define PUSHREGS

#define POPREGS

/* Smews configuration */

#define OUTPUT_BUFFER_SIZE 256
#define ALLOC_SIZE 2048
#define STACK_SIZE 64

#endif /* __TARGET_H__ */
