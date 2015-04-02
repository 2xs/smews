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
  Time-stamp: <2015-04-02 17:38:39 (hauspie)>
*/

/*
  <generator>
  <handlers init="kernel_console_init" doGet="kernel_console_get"/>
  </generator>
*/
#include "handlers.h"
#include "connections.h"
#include "kernel_console.h"

static char kernel_console_get(struct args_t *args)
{
   return 0;
}


#define CONSOLE_BUFFER_SIZE 64

/* Can be redefined in a target */
#ifndef KERNEL_CONSOLE_COMMAND_SIZE
#define KERNEL_CONSOLE_COMMAND_SIZE 10
#endif


typedef struct
{
   const char *command;
   const char *shortcut;
   void (*handler)(const char *args);
   const char *help_message;
} console_command_t;


void kernel_console_help(const char *args);
void kernel_console_mem_state(const char *args);
void kernel_console_ll_state(const char *args);
void kernel_console_connections_state(const char *args);
void kernel_console_ressource_index(const char *args);
void kernel_console_ip(const char *args);

void kernel_console_parse_command();


#define CONSOLE_COMMAND(command, shortcut, help) {#command, shortcut, kernel_console_##command, help}

static console_command_t _console_commands[KERNEL_CONSOLE_COMMAND_SIZE] = {
   CONSOLE_COMMAND(help,"h", "Display this help message"),
   CONSOLE_COMMAND(mem_state, "ms", "Dump the state of memory"),
   CONSOLE_COMMAND(ll_state, "ls", "Dump the state of link layer resolve table"),
   CONSOLE_COMMAND(connections_state, "cs", "Show the connections state"),
   CONSOLE_COMMAND(ressource_index, "ri", "Show the available ressources"),
   CONSOLE_COMMAND(ip, "ip", "ip [addr]: Show/change ip address. Warning NO check at all is done on the given string. Use with caution."),
   {NULL,NULL,NULL,NULL},
};

static char _console_buffer[CONSOLE_BUFFER_SIZE];
static int _console_buffer_idx;

static char kernel_console_init()
{
   kernel_console_prompt();
   return 0;
}

/* Performs a base10 convertion of string.
   Stops convertion at the end of the string or at the first non digit.
   i.e. "100" return 100, "100." also return 100
   returns the pointer to the first non digit character or NULL if no convertion has been performed
*/

const char *kernel_console_atoi(const char *arg, int *value)
{
   int len = 0, mul= 1;
   const char *p = arg;
   if (!arg)
      return NULL;
   while (*p >= '0' && *p <= '9') p++;
   if (p == arg)
      return NULL;
   len = p - arg;
   for (--p, *value = 0;  p >= arg; --p)
   {
      *value += (*p - '0') * mul;
      mul *= 10;
   }
   return arg + len;
}

void kernel_console_ip(const char *args)
{
#ifdef IPV6
   KERNEL_CONSOLE_PRINT("Ipv6 change is not implemented yet\r\n");
   return;
#endif                           
                           
   if (args)
   {
      int byte;
      for (byte = 3 ; byte >= 0 ; --byte)
      {
         int val;
         args = kernel_console_atoi(args, &val);
         local_ip_addr[byte] = val;
         args++;
      }
   }
#ifdef IPV6
   KERNEL_CONSOLE_PRINT("Ipv6 is not implemented yet\r\n");
#else
   KERNEL_CONSOLE_PRINT("Current ip address: %d.%d.%d.%d\r\n",
                        local_ip_addr[3], local_ip_addr[2],
                        local_ip_addr[1], local_ip_addr[0]);
#endif
}

extern CONST_VAR(const struct output_handler_t *, resources_index[]);
extern CONST_VAR(unsigned char, urls_tree[]);
void kernel_console_ressource_index(const char *args)
{
   uint8_t i,j;
   for (i = 0 ; resources_index[i] != NULL ; ++i)
   {
      struct output_handler_t *handler = (struct output_handler_t*) CONST_ADDR(resources_index[i]);
      KERNEL_CONSOLE_PRINT("%d %p ", i, handler);
      switch (handler->handler_type)
      {
         case type_file:
            KERNEL_CONSOLE_PRINT("File: ");
            break;
         case type_control:
            KERNEL_CONSOLE_PRINT("Control: ");
            break;
         case type_generator:
            KERNEL_CONSOLE_PRINT("Generator: ");
            break;
         default:
            break;
      }
      KERNEL_CONSOLE_PRINT("\r\n");
   }
   KERNEL_CONSOLE_PRINT("Url tree: %p\r\n", CONST_ADDR(urls_tree));
}

void kernel_console_connections_state(const char *args)
{
   int cpt = 0;
   FOR_EACH_CONN(conn, {
         KERNEL_CONSOLE_PRINT("Connection: %d\r\n", cpt++);
#ifndef IPV6
         KERNEL_CONSOLE_PRINT("\tIP: %d.%d.%d.%d\r\n",
                              conn->ip_addr[3], conn->ip_addr[2],
                              conn->ip_addr[1], conn->ip_addr[0]);
#endif
         if (IS_HTTP(conn))
         {
            KERNEL_CONSOLE_PRINT("\tport: %d\r\n", UI16(conn->protocol.http.port));
            KERNEL_CONSOLE_PRINT("\ttcp_state: %d\r\n", conn->protocol.http.tcp_state);
         }
#ifndef DISABLE_GP_IP_HANDLER
         else
         {
            KERNEL_CONSOLE_PRINT("\tGPIP for protocol %d\r\n", conn->output_handler->handler_data.generator.handlers.gp_ip.protocol);
         }
#endif
         KERNEL_CONSOLE_PRINT("\toutput_handler: ");
         if(conn->output_handler)
            KERNEL_CONSOLE_PRINT("****\r\n");
         else
            KERNEL_CONSOLE_PRINT("NULL\r\n");
         KERNEL_CONSOLE_PRINT("\tsomething to send: %d\r\n", something_to_send(conn));
      })
      }

void kernel_console_help(const char *args)
{
   uint8_t i;
   for (i = 0 ; i < KERNEL_CONSOLE_COMMAND_SIZE && _console_commands[i].command != NULL ; ++i)
      KERNEL_CONSOLE_PRINT("\t- %s (%s) : %s\r\n", _console_commands[i].command, _console_commands[i].shortcut, _console_commands[i].help_message);
}

void kernel_console_ll_state(const char *args)
{
   /* TODO */
}

extern int get_max_free_mem();
extern int get_free_mem();
extern void print_mem_state();

void kernel_console_mem_state(const char *args)
{
   KERNEL_CONSOLE_PRINT("%d bytes free\r\n", get_free_mem());
   KERNEL_CONSOLE_PRINT("Biggest free chunk: %d bytes\r\n", get_max_free_mem());
   /*print_mem_state();*/
}

void kernel_console_prompt()
{
   _console_buffer_idx = 0;
   KERNEL_CONSOLE_PRINT("smews> ");
}

void kernel_console_add_char(char c)
{
   if (c != '\r')
      KERNEL_CONSOLE_PRINT("%c", c);
   else
      KERNEL_CONSOLE_PRINT("\r\n");
   if (c == '\b') /* backspace */
   {
      if (_console_buffer_idx)
      {
         --_console_buffer_idx;
         KERNEL_CONSOLE_PRINT(" \b");
      }
      return;
   }
   if (c == '\r' || _console_buffer_idx == (CONSOLE_BUFFER_SIZE - 1))
   {
      _console_buffer[_console_buffer_idx] = '\0';
      kernel_console_parse_command();
      kernel_console_prompt();
      return;
   }
   _console_buffer[_console_buffer_idx++] = c;
}

int kernel_console_strcmp(const char *s1, const char *s2)
{
   while (*s1 && *s1 == *s2)
   {
      s1++;
      s2++;
   }
   return *s1 - *s2;
}

char *kernel_console_strchr(char *s1, int c)
{
   while (*s1 && *s1 != c) {
      s1++;
   }
   if (*s1)
      return s1;
   return NULL;
}

void kernel_console_parse_command()
{
   uint8_t i;
   char *args = kernel_console_strchr(_console_buffer, ' ');
   if (args != NULL)
   {
      *args = '\0';
      args++;
   }
   for (i = 0 ; i < KERNEL_CONSOLE_COMMAND_SIZE && _console_commands[i].command != NULL ; ++i)
   {
      if (kernel_console_strcmp(_console_buffer, _console_commands[i].command) == 0 || kernel_console_strcmp(_console_buffer, _console_commands[i].shortcut) == 0)
      {
         _console_commands[i].handler(args);
         return;
      }
   }
   KERNEL_CONSOLE_PRINT("Bad command: %s\r\n", _console_buffer);
}

extern char kernel_console_add_handler(const char *command, const char *shortcut, const char *help_message, void (*handler)(const char *))
{
   if (command == NULL || shortcut == NULL || help_message == NULL || handler == NULL)
      return 0;
   uint8_t idx = 0;
   for (idx = 0 ; idx < KERNEL_CONSOLE_COMMAND_SIZE && _console_commands[idx].command != NULL ; ++idx);
   if (idx == KERNEL_CONSOLE_COMMAND_SIZE)
      return 0;
   _console_commands[idx].command = command;
   _console_commands[idx].shortcut = shortcut;
   _console_commands[idx].help_message = help_message;
   _console_commands[idx].handler = handler;
   if (idx < KERNEL_CONSOLE_COMMAND_SIZE - 1)
      _console_commands[idx+1].command = NULL;
   return 1;
}
