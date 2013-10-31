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

#ifndef DISABLE_ELF

#ifndef __ELF_APPLICATION_H__
#define __ELF_APPLICATION_H__

#include "handlers.h"
#include "connections.h"

typedef void *(*elf_application_allocate_t)(unsigned int size);
typedef void  (*elf_application_free_t)(void *memory);

/* Callbacks */
typedef char (*elf_application_install_t)();
typedef void (*elf_application_remove_t)();

struct elf_application_environment_t {
  elf_application_install_t  *install;
  elf_application_remove_t   *remove;
  unsigned char *            urls_tree;
  struct output_handler_t ** resources_index;
};

/* Stores parsing information per connection*/
struct elf_application_parsing_t {
  struct connection *connection;
  unsigned char *    blob;

  struct elf_application_parsing_t *previous;
  struct elf_application_parsing_t *next;
};

struct elf_application_t {
  char *                                filename;
  uint16_t                              size;

  char *                                data_source;
  char *                                data_destination;
  unsigned int                          data_size;
  
  struct elf_application_parsing_t *    parsing;

  struct elf_application_environment_t *environment;

  struct elf_application_t *            previous;
  struct elf_application_t *            next;
};


#define FOR_EACH_APPLICATION(item, code) \
	if(elf_application_get_count() > 0) { \
		struct elf_application_t *(item) = all_applications; \
		do { \
			{code} \
			(item) = (item)->next; \
		} while(item); \
	}

extern struct elf_application_t * all_applications;

extern void elf_applications_init(elf_application_allocate_t allocate, elf_application_free_t free);

extern int  elf_application_get_count();
extern char elf_application_add(struct elf_application_t *application);
extern void elf_application_remove(const struct elf_application_t *application);

/* When the device resets, init all dynamic applications*/
extern void elf_application_init();

/* When a connection has been created */
extern void elf_application_add_connection(struct connection *connection);
/* When a connection has been terminated */
extern void elf_application_remove_connection(const struct connection *connection);

/* Resets the parsing internals*/
extern void elf_application_parsing_start(const struct connection *connection);
extern struct output_handler_t *elf_application_parse_step(const struct connection *connection, uint8_t byte);

#endif

#endif
