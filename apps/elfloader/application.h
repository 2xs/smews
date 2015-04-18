#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include "../../core/elf_application.h"

extern char application_add(const char *filename, uint16_t size,char *data_address, unsigned int data_size, struct elf_application_environment_t *environment);
extern void application_remove(const char *filename);

#endif
