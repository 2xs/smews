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

#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

/* Network drivers */

extern void dev_init(void);
extern uint32_t get_time(void);
extern int16_t dev_get();
extern unsigned char dev_data_to_read();
extern void dev_prepare_output();
extern void dev_output_done(void);
extern void wait_input(void);

#define INBUF_SIZE 2048
#define OUTBUF_SIZE 1516

extern unsigned char in_buffer[INBUF_SIZE];
extern unsigned char out_buffer[OUTBUF_SIZE];

extern int tun_fd;
extern int in_curr;
extern int out_curr;
extern int in_nbytes;
extern struct timeval tv;
extern fd_set fdset;

/* Drivers interface */

#define DEV_MTU 1500
#define HARDWARE_INIT dev_init()
#define HARDWARE_STOP

#define TIME_MILLIS get_time()
#define DEV_GET(c) {(c) = dev_get();}
#define DEV_PUT(c) {out_buffer[out_curr++]=(c); printf("%c",c);}
#define DEV_PUTN(ptr,n) {memcpy(out_buffer + out_curr,ptr,n); out_curr+=n;}
#define DEV_PUTN_CONST(ptr,n) {memcpy(out_buffer + out_curr,ptr,n); out_curr+=n;}
#define DEV_PREPARE_OUTPUT(length) dev_prepare_output()
#define DEV_OUTPUT_DONE dev_output_done()
#define DEV_DATA_TO_READ dev_data_to_read()
#define DEV_WAIT {wait_input();}

/* Smews states */

#define SMEWS_WAITING
#define SMEWS_RECEIVING
#define SMEWS_SENDING
#define SMEWS_ENDING

/* Const and persistent access macros */

#define CONST_VOID_P_VAR const void *
#define CONST_VAR(type,name) type const name
#define PERSISTENT_VAR(type,name) type name

#define CONST_READ_UI8(x) (*((uint8_t*)(x)))
#define CONST_READ_UI16(x) (*((uint16_t*)(x)))
#define CONST_READ_UI32(x) (*((uint32_t*)(x)))
#define CONST_READ_ADDR(x) (*((void**)(x)))

#define CONST_UI8(x) ((uint8_t)(x))
#define CONST_UI16(x) ((uint16_t)(x))
#define CONST_UI32(x) ((uint32_t)(x))
#define CONST_ADDR(x) ((void*)(x))

#define CONST_WRITE_NBYTES(dst,src,len) memcpy(dst,src,len)

/* Endianness */

#define ENDIANNESS LITTLE_ENDIAN

/* Context switching */

#define USE_FRAME_POINTER

#ifdef __x86_64__

#define BACKUP_CTX(sp) \
	asm ("mov %%rsp, %0" : "=r"((sp)[0])); \
	asm ("mov %%rbp, %0" : "=r"((sp)[1])); \
		
#define RESTORE_CTX(sp) \
	asm ("mov %0, %%rsp" :: "r"((sp)[0])); \
	asm ("mov %0, %%rbp" :: "r"((sp)[1])); \


#define PUSHREGS asm( \
	"push	%rbx\n" \
	"push	%r12\n" \
	"push	%r13\n" \
	"push	%r14\n" \
	"push	%r15\n" \
); \

#define POPREGS asm( \
	"pop	%r15\n" \
	"pop	%r14\n" \
	"pop	%r13\n" \
	"pop	%r12\n" \
	"pop	%rbx\n" \
); \

#else

#define BACKUP_CTX(sp) \
	asm ("movl %%esp, %0" : "=r"((sp)[0])); \
	asm ("movl %%ebp, %0" : "=r"((sp)[1])); \
		
#define RESTORE_CTX(sp) \
	asm ("movl %0, %%esp" :: "r"((sp)[0])); \
	asm ("movl %0, %%ebp" :: "r"((sp)[1])); \


#define PUSHREGS asm( \
	"pushl	%edi\n" \
	"pushl	%esi\n" \
	"pushl	%ebx\n" \
); \

#define POPREGS asm( \
	"popl	%ebx\n" \
	"popl	%esi\n" \
	"popl	%edi\n" \
); \

#endif

/* Smews configuration */

#define OUTPUT_BUFFER_SIZE 1500
#define STACK_SIZE 4*1024
#define ALLOC_SIZE 32*1024

#endif /* __TARGET_H__ */
