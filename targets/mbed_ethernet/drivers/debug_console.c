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


#ifdef MBED_USE_CONSOLE


#include "target.h"

#include <rflpc17xx/rflpc17xx.h>

#include "out_buffers.h"
#include "mbed_debug.h"
#include "arp_cache.h"
#include "connections.h"

#define CONSOLE_BUFFER_SIZE 64


int get_free_mem();


typedef struct
{
    const char *command;
    const char *shortcut;
    void (*handler)(char *args);
    const char *help_message;
} console_command_t;


void mbed_console_help(char *args);
void mbed_console_tx_state(char *args);
void mbed_console_rx_state(char *args);
void mbed_console_eth_state(char *args);
void mbed_console_mem_state(char *args);
void mbed_console_arp_state(char *args);
void mbed_console_stack_dump(char *args);
void mbed_console_connections_state(char *args);

void mbed_console_parse_command();


#define CONSOLE_COMMAND(command, shortcut, help) {#command, shortcut, mbed_console_##command, help}

console_command_t _console_commands[] = {
    CONSOLE_COMMAND(help,"h", "Display this help message"),
    CONSOLE_COMMAND(tx_state, "ts", "Dump the state of TX buffers"),
    CONSOLE_COMMAND(rx_state, "rs", "Dump the state of the RX buffers"),
    CONSOLE_COMMAND(eth_state, "es", "Dump the state of ethernet device"),
    CONSOLE_COMMAND(mem_state, "ms", "Dump the state of memory"),
    CONSOLE_COMMAND(arp_state, "as", "Dump the state of arp resolve table"),
    CONSOLE_COMMAND(stack_dump, "sd", "Dump the stack"),
    CONSOLE_COMMAND(connections_state, "cs", "Show the connections state"),
};

static int _console_command_count = sizeof(_console_commands) / sizeof(_console_commands[0]);

static char _console_buffer[CONSOLE_BUFFER_SIZE];
static int _console_buffer_idx;

void mbed_console_connections_state(char *args)
{
	int cpt = 0;
	FOR_EACH_CONN(conn, {
		printf("Connection: %d\r\n", cpt++);
		if (!conn->output_handler || conn->output_handler->handler_type != type_general_ip_handler)
		{
			printf("\tport: %d\r\n", UI16(conn->protocol.http.port));
			printf("\ttcp_state: %d\r\n", conn->protocol.http.tcp_state);
		}
		else
		{
			printf("\tGPIP for protocol %d\r\n", conn->output_handler->handler_data.generator.handlers.gp_ip.protocol);
		}
		printf("\toutput_handler: ");
		if(conn->output_handler)
			printf("****\r\n");
		else
			printf("NULL\r\n");
		printf("\tsomething to send: %d\r\n", something_to_send(conn));
	})
}

void mbed_console_help(char *args)
{
    int i;
    for (i = 0 ; i < _console_command_count ; ++i)
	printf("\t- %s (%s) : %s\r\n", _console_commands[i].command, _console_commands[i].shortcut, _console_commands[i].help_message);
}

extern void mbed_eth_dump_tx_buffer_status();

void mbed_console_arp_state(char *args)
{
  mbed_arp_dump();
}

void mbed_console_tx_state(char *args)
{
    mbed_eth_dump_tx_buffer_status();
}

/* extern variable, needed for rx state */
extern const uint8_t * volatile current_rx_frame;
extern volatile uint32_t current_rx_frame_size;
extern volatile uint32_t current_rx_frame_idx;

void mbed_console_rx_state(char *args)
{
   printf("Rx frame ptr: %p\r\n", current_rx_frame);
   printf("Rx frame idx: %d\r\n", current_rx_frame_idx);
   printf("Rx frame size: %d\r\n", current_rx_frame_size);
   if (current_rx_frame)
   {
      MBED_DUMP_BYTES(current_rx_frame, current_rx_frame_size);
   }
   printf("Dumping rx descriptors\r\n");
   rflpc_eth_descriptor_t *d = (rflpc_eth_descriptor_t*)LPC_EMAC->RxDescriptor;
   rflpc_eth_rx_status_t *s = (rflpc_eth_rx_status_t*)LPC_EMAC->RxStatus;
   int i;
   for (i = 0 ; i <= LPC_EMAC->RxDescriptorNumber ; ++i)
   {
      printf("Descriptor %d: packet: %p, status: %0x\r\n", i, d[i].packet, s[i].status_info);
      if (d[i].packet)
         MBED_DUMP_BYTES(d[i].packet, rflpc_eth_get_packet_size(s[i].status_info));
   }
}
void mbed_console_eth_state(char *args)
{
    rflpc_eth_dump_internals();
}
void mbed_console_mem_state(char *args)
{
    printf("%d bytes free\r\n", get_free_mem());
}

void mbed_console_stack_dump(char *args)
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
         printf("Current PC: %p\r\n", mstack[i+5]);
         /* Dump from here */
         int j;
         for (j = 0 ; j < 128 ; ++j)
         {
            if (j % 16 == 0)
               printf("\n\r%p: ", ((uint8_t*)(mstack+i))  + j);
            printf("%02x ", ((uint8_t*)(mstack+i))[j]);
         }
         printf("\n\r");
         break;
      }
   }
}

void mbed_console_prompt()
{
    _console_buffer_idx = 0;
    printf("smews> ");
}

void mbed_console_add_char(char c)
{
    if (c != '\r')
	printf("%c", c);
    else
	printf("\r\n");
    if (c == '\b') /* backspace */
    {
       if (_console_buffer_idx)
       {
         --_console_buffer_idx;
         printf(" \b");
       }
       return;
    }
    if (c == '\r' || _console_buffer_idx == (CONSOLE_BUFFER_SIZE - 1))
    {
	_console_buffer[_console_buffer_idx] = '\0';
	mbed_console_parse_command();
	mbed_console_prompt();
	return;
    }
    _console_buffer[_console_buffer_idx++] = c;
}

int mbed_console_strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2)
    {
	s1++;
	s2++;
    }
    return *s1 - *s2;
}

char *mbed_console_strchr(char *s1, int c)
{
    while (*s1 && *s1++ != c);
    if (*s1)
	return s1;
    return NULL;
}

void mbed_console_parse_command()
{
    int i;
    char *args = mbed_console_strchr(_console_buffer, ' ');
    if (args != NULL)
    {
	*args = '\0';
	args++;
    }
    for (i = 0 ; i < _console_command_count ; ++i)
    {
	if (mbed_console_strcmp(_console_buffer, _console_commands[i].command) == 0 || mbed_console_strcmp(_console_buffer, _console_commands[i].shortcut) == 0)
	{
	    _console_commands[i].handler(args);
	    return;
	}
    }
    printf("Bad command: %s\r\n", _console_buffer);
}


#endif
