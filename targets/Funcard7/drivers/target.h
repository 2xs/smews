/*
* Copyright or © or Copr. 2008, Geoffroy Cogniaux
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

#include <avr/io.h>
#include "at_config.h"
#include "sw.h"
#define NULL ((void*)0)
#define EEMEM __attribute__((section(".eeprom")))

extern void hardware_init(void);
extern unsigned char dev_get(void);
extern void dev_put(unsigned char byte);
extern char dev_prepare_output(uint16_t len);
extern unsigned char dev_data_to_read(void);
extern void init_read();

/* Drivers interface */

#define HARDWARE_INIT hardware_init()
#define HARDWARE_STOP
#define TIME_MILLIS 0
#define DEV_GET(c) {(c) = dev_get();}
#define DEV_PUT(c) dev_put(c)
#define DEV_PREPARE_OUTPUT(length) if(!dev_prepare_output(length)) return;
#define DEV_OUTPUT_DONE 
#define DEV_DATA_TO_READ dev_data_to_read()
#define DEV_WAIT init_read()

/* Smews states */

#define SMEWS_WAITING
#define SMEWS_SENDING
#define SMEWS_RECEIVING
#define SMEWS_ENDING

/* Const and persistent access macros */

#define CONST_VOID_P_VAR const void*
#define CONST_VAR(type,name) type const name EEMEM
#define PERSISTENT_VAR(type,name) CONST_VAR(type,name)

extern uint8_t fxeread(uint8_t* addr);
extern uint32_t eeprom_read_dword (const void *addr);
extern uint16_t eeprom_read_word  (const void *addr);

#define CONST_READ_UI8(x)  ((uint8_t)fxeread((uint8_t*)(x)))
#define CONST_READ_UI16(x) eeprom_read_word ((x))
#define CONST_READ_UI32(x) eeprom_read_dword((x))
#define CONST_READ_ADDR(x) ((void*)(eeprom_read_word((x))))

#define CONST_UI8(x)  CONST_READ_UI8(&(x))
#define CONST_UI16(x) CONST_READ_UI16(&(x))
#define CONST_UI32(x) CONST_READ_UI32(&(x))
#define CONST_ADDR(x) CONST_READ_ADDR(&(x))

extern void eeprom_write_nbytes(void *dst, const unsigned char *src, uint16_t len);
#define CONST_WRITE_NBYTES(dst,src,len) eeprom_write_nbytes(dst,src,len)

/* Endianness */
#define ENDIANNESS LITTLE_ENDIAN

/* Context switching */
/* To be done */

#define BACKUP_CTX(sp)
		
#define RESTORE_CTX(sp)

#define PUSHREGS

#define POPREGS

/* Smews configuration */

#define OUTPUT_BUFFER_SIZE 32
#define STACK_SIZE 32
#define ALLOC_SIZE 128

#endif /* __TARGET_H__ */
