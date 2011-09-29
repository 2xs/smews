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
  Time-stamp: <2011-09-29 12:54:17 (hauspie)>
*/


#ifdef MBED_USE_CONSOLE


#include <rflpc17xx/drivers/ethernet.h>
#include "target.h"
#include "out_buffers.h"
#define CONSOLE_BUFFER_SIZE 64


int get_free_mem();


typedef struct
{
    const char *command;
    void (*handler)(char *args);
    const char *help_message;
} console_command_t;


void mbed_console_help(char *args);
void mbed_console_tx_state(char *args);
void mbed_console_eth_state(char *args);
void mbed_console_mem_state(char *args);
void mbed_console_parse_command();

#define CONSOLE_COMMAND(command, help) {#command, mbed_console_##command, help}

console_command_t _console_commands[] = {
    CONSOLE_COMMAND(help, "Display this help message"),
    CONSOLE_COMMAND(tx_state, "Dump the state of TX buffers"),
    CONSOLE_COMMAND(eth_state, "Dump the state of ethernet device"),
    CONSOLE_COMMAND(mem_state, "Dump the state of memory"),
};

static int _console_command_count = sizeof(_console_commands) / sizeof(_console_commands[0]);

static char _console_buffer[CONSOLE_BUFFER_SIZE];
static int _console_buffer_idx;


void mbed_console_help(char *args)
{
    int i;
    for (i = 0 ; i < _console_command_count ; ++i)
	printf("\t- %s : %s\r\n", _console_commands[i].command, _console_commands[i].help_message);
}
void mbed_console_tx_state(char *args)
{
    mbed_eth_dump_tx_buffer_status();
}
void mbed_console_eth_state(char *args)
{
    rflpc_eth_dump_internals();
}
void mbed_console_mem_state(char *args)
{
    printf("%d bytes free\r\n", get_free_mem());
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
	if (mbed_console_strcmp(_console_buffer, _console_commands[i].command) == 0)
	{
	    _console_commands[i].handler(args);
	    return;
	}
    }
    printf("Bad command: %s\r\n", _console_buffer);
}


#endif
