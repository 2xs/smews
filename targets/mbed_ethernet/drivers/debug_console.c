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
/*
  Author: Michael Hauspie <michael.hauspie@univ-lille1.fr>
  Created: 2011-09-29
  Time-stamp: <2012-02-21 16:53:12 (hauspie)>
*/


#ifdef KERNEL_CONSOLE

#include "kernel_console.h"

#include "target.h"

#include <rflpc17xx/rflpc17xx.h>

#include "out_buffers.h"
#include "mbed_debug.h"

#define DECLARE_MBED_CONSOLE_COMMAND(command, shortcut, help) \
	static const char *command_##command = #command; \
	static const char *shortcut_##command = shortcut; \
	static const char *help_##command = help; \
	void mbed_console_##command(const char *args);

#define ADD_HANDLER(command) printf("Adding %s %d\r\n", #command, kernel_console_add_handler(command_##command, shortcut_##command, help_##command, mbed_console_##command))

DECLARE_MBED_CONSOLE_COMMAND(tx_state, "ts", "show the state of transmit buffers");
DECLARE_MBED_CONSOLE_COMMAND(rx_state, "rs", "show the state of reception buffers");
DECLARE_MBED_CONSOLE_COMMAND(eth_state, "es", "show the state of ethernet device");
DECLARE_MBED_CONSOLE_COMMAND(stack_dump, "sd", "show the state of the stack");

void mbed_console_init()
{
	ADD_HANDLER(tx_state);
	ADD_HANDLER(rx_state);
	ADD_HANDLER(eth_state);
	ADD_HANDLER(stack_dump);
}

extern void mbed_eth_dump_tx_buffer_status();

void mbed_console_tx_state(const char *args)
{
    mbed_eth_dump_tx_buffer_status();
}

/* extern variable, needed for rx state */
extern const uint8_t * volatile current_rx_frame;
extern volatile uint32_t current_rx_frame_size;
extern volatile uint32_t current_rx_frame_idx;

void mbed_console_rx_state(const char *args)
{
   KERNEL_CONSOLE_PRINT("Rx frame ptr: %p\r\n", current_rx_frame);
   KERNEL_CONSOLE_PRINT("Rx frame idx: %d\r\n", current_rx_frame_idx);
   KERNEL_CONSOLE_PRINT("Rx frame size: %d\r\n", current_rx_frame_size);
   if (current_rx_frame)
   {
      MBED_DUMP_BYTES(current_rx_frame, current_rx_frame_size);
   }
   KERNEL_CONSOLE_PRINT("Dumping rx descriptors\r\n");
   rflpc_eth_descriptor_t *d = (rflpc_eth_descriptor_t*)LPC_EMAC->RxDescriptor;
   rflpc_eth_rx_status_t *s = (rflpc_eth_rx_status_t*)LPC_EMAC->RxStatus;
   int i;
   for (i = 0 ; i <= LPC_EMAC->RxDescriptorNumber ; ++i)
   {
      KERNEL_CONSOLE_PRINT("Descriptor %d: packet: %p, status: %0x\r\n", i, d[i].packet, s[i].status_info);
      if (d[i].packet)
         MBED_DUMP_BYTES(d[i].packet, rflpc_eth_get_packet_size(s[i].status_info));
   }
}
void mbed_console_eth_state(const char *args)
{
    rflpc_eth_dump_internals();
}

void mbed_console_stack_dump(const char *args)
{
   uint32_t *mstack =(uint32_t*) __get_MSP();
   int i;
   /* As this function is called from an interrupt handler, we will look for 0xFFFFFFF9 to find the exception frame.
    * This value is the value of the LR which is pushed in the interrupt handler. The frame exception frame is just after.
    * It includes
    * R0-R3,R12
    * Return Address
    * PSR
    * LR
    * ...
    */
   for (i = 0 ; i < 32 ; ++i)
   {
      if (mstack[i] == 0xFFFFFFF9)
      {
         ++i;
         KERNEL_CONSOLE_PRINT("Current PC: %p\r\n", mstack[i+5]);
         /* Dump from here */
         int j;
         for (j = 0 ; j < 128 ; ++j)
         {
            if (j % 16 == 0)
               KERNEL_CONSOLE_PRINT("\n\r%p: ", ((uint8_t*)(mstack+i))  + j);
            KERNEL_CONSOLE_PRINT("%02x ", ((uint8_t*)(mstack+i))[j]);
         }
         KERNEL_CONSOLE_PRINT("\n\r");
         break;
      }
   }
}
#endif
