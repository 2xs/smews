/*
* Copyright or © or Copr. 2011, Michael Hauspie
*
* Author e-mail: michael.hauspie@lifl.fr
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

/* Other c and h target-specific files must be located in the drivers directory */

/* Types definition */
#include <stdint.h>

/* Target specific includes */
#include <rflpc17xx/rflpc17xx.h>

#include "hardware.h"
#include "eth_input.h"
#include "eth_output.h"

/* Smews includes */


/* Drivers interface */

/* Set the maximal MTU for the network interface used */
#define DEV_MTU 1500
/* Initializes the hardware */
#define HARDWARE_INIT {mbed_eth_hardware_init();}
/* Stops the hardware */
#define HARDWARE_STOP
/* Returns the time in milliseconds */
#define TIME_MILLIS mbed_get_time()
/* Return 1 if data can be read */
#define DEV_DATA_TO_READ mbed_eth_byte_available()
/* Reads one byte */
#define DEV_GET(c) {(c) = mbed_eth_get_byte();}
/* Writes one byte */
#define DEV_PUT(c) do { mbed_eth_put_byte((c)); } while (0)
/* Preparation for sending n bytes */
#define DEV_PREPARE_OUTPUT(n) do { mbed_eth_prepare_output((n)); } while(0)
/* End of output */
#define DEV_OUTPUT_DONE do { mbed_eth_output_done(); } while (0)

/* Optionnal Smews macros */

/* Writes n bytes starting from ptr */
#define DEV_PUTN(ptr,n) do {mbed_eth_put_nbytes((ptr), (n)); } while (0)
/* Writes n bytes starting from ptr (for const data) */
#define DEV_PUTN_CONST(ptr,n) DEV_PUTN((ptr),(n))
/* Passive wait for a device input. Must return after a given time */
#define DEV_WAIT

/* Smews states */

/* Called by the kernel when starting to wait for data */
#define SMEWS_WAITING
/* Called by the kernel when starting to receive data */
#define SMEWS_RECEIVING
/* Called by the kernel when starting to send data */
#define SMEWS_SENDING
/* Called by the kernel when ending to send data */
#define SMEWS_ENDING

/* Cons variables macros */

/* Used to declare a reference to a const */
#define CONST_VOID_P_VAR const void *
/* used to declare a variable as const */
#define CONST_VAR(type,name) type const name
#define PERSISTENT_VAR(type,name) type name

/* Gets a byte from address x */
#define CONST_READ_UI8(x) ((uint8_t)*((uint8_t*)(x)))
/* Gets two bytes from address x */
#define CONST_READ_UI16(x) ((uint16_t)*((uint16_t*)(x)))
/* Gets four bytes from address x */
#define CONST_READ_UI32(x) ((uint32_t)*((uint32_t*)(x)))
/* Gets an address from address x */
#define CONST_READ_ADDR(x) ((void*)*((void**)(x)))

/* Get the value of x, where x is declared as a one byte const */
#define CONST_UI8(x) ((uint8_t)(x))
/* Get the value of x, where x is declared as a two bytes const */
#define CONST_UI16(x) ((uint16_t)(x))
/* Get the value of x, where x is declared as a four bytes const */
#define CONST_UI32(x) ((uint32_t)(x))
/* Get the value of x, where x is declared as a const reference */
#define CONST_ADDR(x) ((void*)(x))
/* Writes len bytes from src to dst, where dst targets a persistent variable */
#define CONST_WRITE_NBYTES(dst,src,len)

/* Endianness: define ENDIANNESS as LITTLE_ENDIAN or BIG_ENDIAN */
#ifndef ENDIANNESS
	#define ENDIANNESS LITTLE_ENDIAN
#endif

/* Context switching */

/* save the stack pointer in sp[0] (and possibly a frame pointer in sp[1]) */
#define BACKUP_CTX(sp) do {(sp)[0] = (void*)__get_MSP();} while(0)
/* restore the stack pointer from sp[0] (and possibly a frame pointer from sp[1]) */
#define RESTORE_CTX(sp) do { __set_MSP((uint32_t)(sp)[0]); } while(0)
/* push all registers that must not be modified by any function call */
#define PUSHREGS do { asm("push {r4-r11, lr}"); } while(0)
/* pop all registers that must not be modified by any function call */
#define POPREGS do { asm("pop {r4-r11, lr}"); } while (0)

/* Smews configuration */

/* size of the buffer used to generate dynamic content */
#define OUTPUT_BUFFER_SIZE (DEV_MTU)
/* size of the shared stack used by all dynamic content generators */
#define STACK_SIZE 1024
/* size of the dynamic memory allocator pool */
#define ALLOC_SIZE 4096

/* Ethernet configuration */
/* Number of frame descriptors for reception. For each descriptor,
 * 16 bytes will be needed */
#define RX_DESCRIPTORS 10
/* Size of the reception buffers. One buffer will be needed per reception descriptor
   The buffers will be store in the external SRAM. So 32K of RAM is available ONLY for this buffers
 */
#define RX_BUFFER_SIZE RFLPC_ETH_MAX_FRAME_LENGTH
/* Size of the transmission buffer. */
#define TX_BUFFER_SIZE RFLPC_ETH_MAX_FRAME_LENGTH

#define TX_BUFFER_COUNT 10
/* Number of frame descriptors for transmission. For each descriptor,
 * 12 bytes will be needed */
#define TX_DESCRIPTORS 20
/* Maximum number of fragment that we can use to create one ethernet frame. Must less than the number of descriptos */
#define TX_MAX_FRAGMENTS 10

/* This indicates a threshold from which a DEV_PUTN_CONST will be implemented by a multi-fragment frame using the ethernet controller gather DMA */
#define DMA_THRESHOLD 64

#define LINK_LAYER_ADDRESS_SIZE 6

extern EthAddr local_eth_addr;
#define LINK_LAYER_ADDRESS (local_eth_addr.addr)

#define TARGET_PRINTF printf
#define TARGET_RAND   rand()

#endif /* __TARGET_H__ */
