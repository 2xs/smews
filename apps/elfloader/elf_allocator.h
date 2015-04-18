#ifndef __ELF_ALLOCATOR_H__
#define __ELF_ALLOCATOR_H__

extern const void * elf_allocator_storage;

extern void *elf_allocator_ram_alloc(unsigned int size);

extern void *elf_allocator_flash_alloc(unsigned int size);

#endif
