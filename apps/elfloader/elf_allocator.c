#include "elf_allocator.h"

/* RAM */
#ifndef ELF_ALLOCATOR_RAM_SIZE
#define ELF_ALLOCATOR_RAM_SIZE 1024*4
#endif

/* FLASH */
#ifndef ELF_ALLOCATOR_FLASH_ADDRESS
extern char _persistent_data_end;
#define ELF_ALLOCATOR_FLASH_ADDRESS ((char *)&_persistent_data_end)
#endif

#ifndef ELF_ALLOCATOR_FLASH_STORAGE_ADDRESS
#define ELF_ALLOCATOR_FLASH_STORAGE_ADDRESS ((char *)0x0000C000)
#endif


static char elf_allocator_ram_buffer[ELF_ALLOCATOR_RAM_SIZE];
static char *elf_allocator_last_ram_used = 0;

const char *elf_allocator_flash_buffer = ELF_ALLOCATOR_FLASH_ADDRESS;
static char *elf_allocator_last_flash_used   = 0;


const void * elf_allocator_storage = ELF_ALLOCATOR_FLASH_STORAGE_ADDRESS;

void *elf_allocator_ram_alloc(unsigned int size) {
  char *result;	
  if(elf_allocator_last_ram_used == 0)
    elf_allocator_last_ram_used = elf_allocator_ram_buffer;

  result                       = elf_allocator_last_ram_used;
  elf_allocator_last_ram_used += size;

  return result;
}

void *elf_allocator_flash_alloc(unsigned int size) {
  char *result;	
  if(elf_allocator_last_flash_used == 0)
    elf_allocator_last_flash_used = (char *)elf_allocator_flash_buffer;

  result                         = elf_allocator_last_flash_used;
  elf_allocator_last_flash_used += ((size & 1) == 0) ? size : size + 1; // align on an even address

  return result;
}
