/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * Copyright (c) 2007, Simon Berg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: elfloader-otf.c,v 1.1 2009/07/11 14:18:50 ksb Exp $
 */

#ifdef LINUX

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#else 

#include <rflpc17xx/rflpc17xx.h>

#endif

#include "elfloader-otf.h"
#include "elfloader-arch-otf.h"

//#include "cfs/cfs.h"
#include "RamFileSystem.h"
#include "symtab.h"
#include "tinyLibC.h"

// For rodata linked list.
#include "../../core/memory.h"

/*#include <stddef.h>
#include <string.h>
#include <stdio.h>*/

#if 0
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
//#define PRINTF(...) printf(__VA_ARGS__)
#endif

#define EI_NIDENT 16


struct elf32_ehdr {
  unsigned char e_ident[EI_NIDENT];    /* ident bytes */
  elf32_half e_type;                   /* file type */
  elf32_half e_machine;                /* target machine */
  elf32_word e_version;                /* file version */
  elf32_addr e_entry;                  /* start address */
  elf32_off e_phoff;                   /* phdr file offset */
  elf32_off e_shoff;                   /* shdr file offset */
  elf32_word e_flags;                  /* file flags */
  elf32_half e_ehsize;                 /* sizeof ehdr */
  elf32_half e_phentsize;              /* sizeof phdr */
  elf32_half e_phnum;                  /* number phdrs */
  elf32_half e_shentsize;              /* sizeof shdr */
  elf32_half e_shnum;                  /* number shdrs */
  elf32_half e_shstrndx;               /* shdr string index */
};

/* Values for e_type. */
#define ET_NONE         0       /* Unknown type. */
#define ET_REL          1       /* Relocatable. */
#define ET_EXEC         2       /* Executable. */
#define ET_DYN          3       /* Shared object. */
#define ET_CORE         4       /* Core file. */

struct elf32_shdr {
  elf32_word sh_name; 		/* section name */
  elf32_word sh_type; 		/* SHT_... */
  elf32_word sh_flags; 	        /* SHF_... */
  elf32_addr sh_addr; 		/* virtual address */
  elf32_off sh_offset; 	        /* file offset */
  elf32_word sh_size; 		/* section size */
  elf32_word sh_link; 		/* misc info */
  elf32_word sh_info; 		/* misc info */
  elf32_word sh_addralign; 	/* memory alignment */
  elf32_word sh_entsize; 	/* entry size if table */
};

/* sh_type */
#define SHT_NULL        0               /* inactive */
#define SHT_PROGBITS    1               /* program defined information */
#define SHT_SYMTAB      2               /* symbol table section */
#define SHT_STRTAB      3               /* string table section */
#define SHT_RELA        4               /* relocation section with addends*/
#define SHT_HASH        5               /* symbol hash table section */
#define SHT_DYNAMIC     6               /* dynamic section */
#define SHT_NOTE        7               /* note section */
#define SHT_NOBITS      8               /* no space section */
#define SHT_REL         9               /* relation section without addends */
#define SHT_SHLIB       10              /* reserved - purpose unknown */
#define SHT_DYNSYM      11              /* dynamic symbol table section */
#define SHT_LOPROC      0x70000000      /* reserved range for processor */
#define SHT_HIPROC      0x7fffffff      /* specific section header types */
#define SHT_LOUSER      0x80000000      /* reserved range for application */
#define SHT_HIUSER      0xffffffff      /* specific indexes */

struct elf32_rel {
  elf32_addr      r_offset;       /* Location to be relocated. */
  elf32_word      r_info;         /* Relocation type and symbol index. */
};

#define ELF32_R_SYM(info)       ((info) >> 8)
#define ELF32_R_TYPE(info)      ((unsigned char)(info))


struct relevant_section {
  unsigned char number;
  unsigned int offset;
  char *address;
};

struct relevant_rodata_section {
  unsigned char number;
  unsigned int offset;
  char *address;
  struct relevant_rodata_section *next;
};

char elfloader_unknown[30];	/* Name that caused link error. */

static struct relevant_section bss, data, rodata, text;

const static unsigned char elf_magic_header[] =
  {0x7f, 0x45, 0x4c, 0x46,  /* 0x7f, 'E', 'L', 'F' */
   0x01,                    /* Only 32-bit objects. */
   0x01,                    /* Only LSB data. */
   0x01,                    /* Only ELF version 1. */
  };

#define COPY_SEGMENT_DATA_BUFFER_SIZE 128

static char copy_segment_data_buffer[COPY_SEGMENT_DATA_BUFFER_SIZE];
/* Copy data from the elf file to a segment */
static int
copy_segment_data(void *input_fd, unsigned int offset,
		  struct elfloader_output *output, unsigned int len)
{
  int res;

  /*printf("Copy Segment Data IN input %p output %p buffer %p, len : %d\r\n", input_fd, output, buffer, len);*/

  if (rfs_seek(input_fd, offset) != offset) return ELFLOADER_INPUT_ERROR;
  while(len > sizeof(copy_segment_data_buffer)) {
    res = rfs_read(copy_segment_data_buffer, sizeof(copy_segment_data_buffer), 1, input_fd);
    if (res != sizeof(copy_segment_data_buffer)) return ELFLOADER_INPUT_ERROR;
    res = elfloader_output_write_segment(output, copy_segment_data_buffer, sizeof(copy_segment_data_buffer));
    if (res != sizeof(copy_segment_data_buffer)) return ELFLOADER_OUTPUT_ERROR;
    len -= sizeof(copy_segment_data_buffer);
  }

  if (len)
  {
	res = rfs_read(copy_segment_data_buffer, len, 1, input_fd);
  	if (res != len) return ELFLOADER_INPUT_ERROR;
	res = elfloader_output_write_segment(output, copy_segment_data_buffer, len);
  	if (res != len) return ELFLOADER_OUTPUT_ERROR;
  }
  /*printf("Copy Segment Data OUT input %p output %p\r\n", input_fd, output);*/

  return ELFLOADER_OK;
}

static int
seek_read(void *fd, unsigned int offset, char *buf, int len)
{
  if (rfs_seek(fd, offset) != offset) return -1;
  return rfs_read(buf, len, 1, fd);
}

static void *
find_local_symbol(void *input_fd, const char *symbol,
		  unsigned int symtab, unsigned short symtabsize,
		  unsigned int strtab,
                  struct relevant_rodata_section *rodatas)
{
  struct elf32_sym s;
  unsigned int a;
  char name[30];
  struct relevant_section *sect;
  int ret;
  
  /*printf("Symbol Name : %s\r\n", symbol);*/


  for(a = symtab; a < symtab + symtabsize; a += sizeof(s)) {
    ret = seek_read(input_fd, a, (char *)&s, sizeof(s));
    if (ret < 0) return NULL;

    if(s.st_name != 0) {
      ret = seek_read(input_fd, strtab + s.st_name, name, sizeof(name));
      if (ret < 0) return NULL;
      /*printf("====>Name : %s\r\n", name);*/
      if(strcmp(name, symbol) == 0) {
	if(s.st_shndx == bss.number) {
	  sect = &bss;
	} else if(s.st_shndx == data.number) {
	  sect = &data;
	} else if(s.st_shndx == text.number) {
	  sect = &text;
	} else {
          struct relevant_rodata_section *rodata = rodatas;	  
	  while(rodata) {
            if(rodata->number == s.st_shndx) {
              return &(rodata->address[s.st_value]);
	    }

            rodata = rodata->next;
	  }
          return NULL;
	}

	return &(sect->address[s.st_value]);
      }
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
static int
relocate_section(void *input_fd,
		 struct elfloader_output *output,
		 unsigned int section, unsigned short size,
		 unsigned int sectionaddr,
		 char *sectionbase,
		 unsigned int strs,
		 unsigned int strtab,
		 unsigned int symtab, unsigned short symtabsize,
		 unsigned char using_relas,
                 struct relevant_rodata_section *rodatas)
{
  /* sectionbase added; runtime start address of current section */
  struct elf32_rela rela; /* Now used both for rel and rela data! */
  int rel_size = 0;
  struct elf32_sym s;
  unsigned int a;
  char name[30];
  char *addr;
  struct relevant_section *sect;
  int ret;

//  printf("\r\n------------>%s IN fd = %p %p\r\n", __FUNCTION__, input_fd,output);
  /* determine correct relocation entry sizes */
  if(using_relas) {
    rel_size = sizeof(struct elf32_rela);
  } else {
    rel_size = sizeof(struct elf32_rel);
  }

//  printf("elfloader : Section %x size :%d relsize %d\r\n", section, size, rel_size);
  
  for(a = section; a < section + size; a += rel_size) {
    ret = seek_read(input_fd, a, (char *)&rela, rel_size);
    if (ret < 0) return ELFLOADER_INPUT_ERROR;


    ret = seek_read(input_fd,
		    (symtab +
		     sizeof(struct elf32_sym) * ELF32_R_SYM(rela.r_info)),
		    (char *)&s, sizeof(s));

    if (ret < 0) return ELFLOADER_INPUT_ERROR;


    /*printf("\r\ns.st_name %p\r\n", s.st_name);
    printf("s.st_shndx %d\r\n", s.st_shndx);*/

    if(s.st_name != 0) {
      ret = seek_read(input_fd, strtab + s.st_name, name, sizeof(name));
      if (ret < 0) return ELFLOADER_INPUT_ERROR;

      addr = (char *)symtab_lookup(name);
      /* ADDED */
      if(addr == NULL) {
	addr = find_local_symbol(input_fd, name, symtab, symtabsize, strtab, rodatas);
      }
      if(addr == NULL) {
	if(s.st_shndx == bss.number) {
	  sect = &bss;
	} else if(s.st_shndx == data.number) {
	  sect = &data;
	} else if(s.st_shndx == rodata.number) {
	  sect = &rodata;
	} else if(s.st_shndx == text.number ) {
	  sect = &text;
	} else {
	  printf("elfloader unknown name: '%30s'\n", name);
	  memcpy(elfloader_unknown, name, sizeof(elfloader_unknown));
	  elfloader_unknown[sizeof(elfloader_unknown) - 1] = 0;
	  return ELFLOADER_SYMBOL_NOT_FOUND;
	}
	addr = sect->address;
      }
    } else {
      if(s.st_shndx == bss.number) {
	sect = &bss;
      } else if(s.st_shndx == data.number) {
	sect = &data;
      } else if(s.st_shndx == rodata.number) {
	sect = &rodata;
      } else if(s.st_shndx == text.number) {
	sect = &text;
      } else {
	return ELFLOADER_SEGMENT_NOT_FOUND;
      }
      
      addr = sect->address;
    }
    
#if 0 /* We don't know how big the relocation is or even if we need to read it.
	 Let the architecture dependant code decide */
    if (!using_relas) {
      /* copy addend to rela structure */
      ret = seek_read(fd, sectionaddr + rela.r_offset, &rela.r_addend, 4);
      if (ret < 0) return ELFLOADER_INPUT_ERROR;
    }
#endif
    {
      /* Copy data up to the next relocation */
      unsigned int offset = elfloader_output_segment_offset(output);
      if (rela.r_offset < offset) {
	PRINTF("elfloader relocation out of offset order\r\n");
	
      }
      if (rela.r_offset > offset) {
	ret = copy_segment_data(input_fd, offset+sectionaddr, output,
				rela.r_offset - offset);

	if (ret != ELFLOADER_OK) return ret;
      }
    }

    ret = elfloader_arch_relocate(input_fd, output, sectionaddr, sectionbase,
				  &rela, addr, &s);

    if (ret != ELFLOADER_OK) return ret;
  }

  return ELFLOADER_OK;
}

/*---------------------------------------------------------------------------*/
void
elfloader_init(void)
{
}
/*---------------------------------------------------------------------------*/
#if 0
static void
print_chars(unsigned char *ptr, int num)
{
  int i;
  for(i = 0; i < num; ++i) {
    PRINTF("%d", ptr[i]);
    if(i == num - 1) {
      PRINTF("\n");
    } else {
      PRINTF(", ");
    }
  }
}
#endif /* 0 */

static int
copy_segment(void *input_fd,
	     struct elfloader_output *output,
	     unsigned int section, unsigned short size,
	     unsigned int sectionaddr,
	     char *sectionbase,
	     unsigned int strs,
	     unsigned int strtab,
	     unsigned int symtab, unsigned short symtabsize,
	     unsigned char using_relas,
	     unsigned int seg_size, unsigned int seg_type,
             struct relevant_rodata_section *rodatas)
{
  unsigned int offset;
  int ret;

  ret = elfloader_output_start_segment(output, seg_type,sectionbase, seg_size);
  if (ret != ELFLOADER_OK) return ret;

  ret = relocate_section(input_fd, output,
			 section, size,
			 sectionaddr,
			 sectionbase,
			 strs,
			 strtab,
			 symtab, symtabsize, using_relas, rodatas);

  if (ret != ELFLOADER_OK) return ret;

  offset = elfloader_output_segment_offset(output);
  ret = copy_segment_data(input_fd, offset+sectionaddr, output,seg_size - offset);

  if (ret != ELFLOADER_OK) return ret;

  return elfloader_output_end_segment(output);
}

static void cleanup_rodatas(struct relevant_rodata_section *rodatas) {
  struct relevant_rodata_section *next_rodata;
  struct relevant_rodata_section *temp_rodata;
  temp_rodata = rodatas;
  while(temp_rodata) {
    next_rodata = temp_rodata->next;
    mem_free(temp_rodata, sizeof(struct relevant_rodata_section));
    temp_rodata = next_rodata;
  }
}

/*---------------------------------------------------------------------------*/
struct elf_application_environment_t *elf_application_environment;
int
elfloader_load(void *input_fd, struct elfloader_output *output)
{
  struct elf32_ehdr ehdr;
  struct elf32_shdr shdr;
  struct elf32_shdr strtable;
  unsigned int strs;
  unsigned int shdrptr;
  unsigned int nameptr;
  char name[12];
  
  int i;
  unsigned short shdrnum, shdrsize;

  unsigned char using_relas = -1;
  unsigned short textoff = 0, textsize, textrelaoff = 0, textrelasize;
  unsigned short dataoff = 0, datasize, datarelaoff = 0, datarelasize;
  unsigned short rodataoff = 0, rodatasize, rodatarelaoff = 0, rodatarelasize;
  unsigned short symtaboff = 0, symtabsize;
  unsigned short strtaboff = 0, strtabsize;
  unsigned short bsssize = 0;

  struct relevant_rodata_section *rodatas = NULL;
  struct relevant_rodata_section *temp_rodata = NULL;

  void *localSymbol;
  int ret;

  elfloader_unknown[0] = 0;

  /* The ELF header is located at the start of the buffer. */
  ret = seek_read(input_fd, 0, (char *)&ehdr, sizeof(ehdr));
  if (ret != sizeof(ehdr)) return ELFLOADER_INPUT_ERROR;

  /*  print_chars(ehdr.e_ident, sizeof(elf_magic_header));
      print_chars(elf_magic_header, sizeof(elf_magic_header));*/
  /* Make sure that we have a correct and compatible ELF header. */
  if(memcmp(ehdr.e_ident, elf_magic_header, sizeof(elf_magic_header)) != 0) {
    PRINTF("ELF header problems\n");
    return ELFLOADER_BAD_ELF_HEADER;
  }

  /* Grab the section header. */
  shdrptr = ehdr.e_shoff;
  ret = seek_read(input_fd, shdrptr, (char *)&shdr, sizeof(shdr));
  if (ret != sizeof(shdr)) return ELFLOADER_INPUT_ERROR;
  
  /* Get the size and number of entries of the section header. */
  shdrsize = ehdr.e_shentsize;
  shdrnum = ehdr.e_shnum;

  /* The string table section: holds the names of the sections. */
  ret = seek_read(input_fd, ehdr.e_shoff + shdrsize * ehdr.e_shstrndx,
			     (char *)&strtable, sizeof(strtable));
  if (ret != sizeof(strtable)) return ELFLOADER_INPUT_ERROR;
  /* Get a pointer to the actual table of strings. This table holds
     the names of the sections, not the names of other symbols in the
     file (these are in the sybtam section). */
  strs = strtable.sh_offset;

  /* Go through all sections and pick out the relevant ones. The
     ".text" segment holds the actual code from the ELF file, the
     ".data" segment contains initialized data, the ".rodata" segment
     contains read-only data, the ".bss" segment holds the size of the
     unitialized data segment. The ".rel[a].text" and ".rel[a].data"
     segments contains relocation information for the contents of the
     ".text" and ".data" segments, respectively. The ".symtab" segment
     contains the symbol table for this file. The ".strtab" segment
     points to the actual string names used by the symbol table.

     In addition to grabbing pointers to the relevant sections, we
     also save the section number for resolving addresses in the
     relocator code.
  */


  /* Initialize the segment sizes to zero so that we can check if
     their sections was found in the file or not. */
  textsize = textrelasize = datasize = datarelasize =
    rodatasize = rodatarelasize = symtabsize = strtabsize = 0;

  bss.number = data.number = rodata.number = text.number = -1;

  shdrptr = ehdr.e_shoff;

  // first, find all .rodata.* sections
  for(i = 0; i < shdrnum; ++i) {

    ret = seek_read(input_fd, shdrptr, (char *)&shdr, sizeof(shdr));
    if (ret != sizeof(shdr)) return ELFLOADER_INPUT_ERROR;
    
    /* The name of the section is contained in the strings table. */
    nameptr = strs + shdr.sh_name;
    ret = seek_read(input_fd, nameptr, name, sizeof(name));
    if (ret != sizeof(name)) return ELFLOADER_INPUT_ERROR;
    
    if(strncmp(name, ".rodata", 7) == 0) {

      temp_rodata = mem_alloc(sizeof(struct relevant_rodata_section));
      if(!temp_rodata) {
        PRINTF("Unable to allocate relevant rodata section.\r\n");
	return ELFLOADER_OUTPUT_ERROR;
      }

      temp_rodata->number = i;
      temp_rodata->offset = shdr.sh_offset;
      
      temp_rodata->address = (char *)
        elfloader_output_alloc_segment(output, ELFLOADER_SEG_RODATA, shdr.sh_size);
      if (!temp_rodata->address) {
	PRINTF("Unable to allocate relevant rodata section space.\r\n");
        return ELFLOADER_OUTPUT_ERROR;
      }

      ret = elfloader_output_start_segment(output, ELFLOADER_SEG_RODATA,
                                           temp_rodata->address, shdr.sh_size);
      if(ret != ELFLOADER_OK) return ret;

      copy_segment_data(input_fd, shdr.sh_offset, output, shdr.sh_size);
      
      // Head insert
      temp_rodata->next = rodatas;
      rodatas = temp_rodata;
    }

    /* Move on to the next section header. */
    shdrptr += shdrsize;
  }

  /*temp_rodata = rodatas;
  while(temp_rodata) {
    printf("Loaded RODATA %d\r\n", temp_rodata->number);

    temp_rodata = temp_rodata->next;
  }*/

	
  // NOW the bss, data, text sections.	
  shdrptr = ehdr.e_shoff;

  for(i = 0; i < shdrnum; ++i) {

    ret = seek_read(input_fd, shdrptr, (char *)&shdr, sizeof(shdr));
    if (ret != sizeof(shdr)) return ELFLOADER_INPUT_ERROR;
    
    /* The name of the section is contained in the strings table. */
    nameptr = strs + shdr.sh_name;
    ret = seek_read(input_fd, nameptr, name, sizeof(name));
    if (ret != sizeof(name)) return ELFLOADER_INPUT_ERROR;
    
    /* Match the name of the section with a predefined set of names
       (.text, .data, .bss, .rela.text, .rela.data, .symtab, and
       .strtab). */
    /* added support for .rodata, .rel.text and .rel.data). */


    if(strcmp(name, ".text") == 0) {
      textoff = shdr.sh_offset;
      textsize = shdr.sh_size;
      text.number = i;
      text.offset = textoff;
    } else if(strcmp(name, ".rel.text") == 0) {
      using_relas = 0;
      textrelaoff = shdr.sh_offset;
      textrelasize = shdr.sh_size;
    } else if(strcmp(name, ".rela.text") == 0) {
      using_relas = 1;
      textrelaoff = shdr.sh_offset;
      textrelasize = shdr.sh_size;
    } else if(strcmp(name, ".data") == 0) {
      dataoff = shdr.sh_offset;
      datasize = shdr.sh_size;
      data.number = i;
      data.offset = dataoff;
    } else if(strcmp(name, ".rodata") == 0) {
      /* read-only data handled the same way as regular text section */
      rodataoff = shdr.sh_offset;
      rodatasize = shdr.sh_size;
      rodata.number = i;
      rodata.offset = rodataoff;
    } else if(strcmp(name, ".rel.rodata") == 0) {
      /* using elf32_rel instead of rela */
      using_relas = 0;
      rodatarelaoff = shdr.sh_offset;
      rodatarelasize = shdr.sh_size;
    } else if(strcmp(name, ".rela.rodata") == 0) {
      using_relas = 1;
      rodatarelaoff = shdr.sh_offset;
      rodatarelasize = shdr.sh_size;
    } else if(strcmp(name, ".rel.data") == 0) {
      /* using elf32_rel instead of rela */
      using_relas = 0;
      datarelaoff = shdr.sh_offset;
      datarelasize = shdr.sh_size;
    } else if(strcmp(name, ".rela.data") == 0) {
      using_relas = 1;
      datarelaoff = shdr.sh_offset;
      datarelasize = shdr.sh_size;
    } else if(strcmp(name, ".symtab") == 0) {
      symtaboff = shdr.sh_offset;
      symtabsize = shdr.sh_size;
    } else if(strcmp(name, ".strtab") == 0) {
      strtaboff = shdr.sh_offset;
      strtabsize = shdr.sh_size;
    } else if(strcmp(name, ".bss") == 0) {
      bsssize = shdr.sh_size;
      bss.number = i;
      bss.offset = 0;
    }

    /* Move on to the next section header. */
    shdrptr += shdrsize;
  }

  if(symtabsize == 0) {
    return ELFLOADER_NO_SYMTAB;
  }
  if(strtabsize == 0) {
    return ELFLOADER_NO_STRTAB;
  }
  if(textsize == 0) {
    return ELFLOADER_NO_TEXT;
  }


  if (bsssize) {
    bss.address = (char *)
      elfloader_output_alloc_segment(output, ELFLOADER_SEG_BSS, bsssize);
    if (!bss.address) return ELFLOADER_OUTPUT_ERROR;
  }
  if (datasize) {
    data.address = (char *)
      elfloader_output_alloc_segment(output,ELFLOADER_SEG_DATA,datasize);
    if (!data.address) return ELFLOADER_OUTPUT_ERROR;
  }
  if (textsize) {
    text.address = (char *)
      elfloader_output_alloc_segment(output,ELFLOADER_SEG_TEXT,textsize);
    if (!text.address) return ELFLOADER_OUTPUT_ERROR;
  }
  if (rodatasize) {
    rodata.address =  (char *)
      elfloader_output_alloc_segment(output,ELFLOADER_SEG_RODATA,rodatasize);
    if (!rodata.address) return ELFLOADER_OUTPUT_ERROR;
  }


/* If we have text segment relocations, we process them. */
  if(textrelasize > 0) {
    PRINTF("elfloader: relocate text\r\n");
    ret = copy_segment(input_fd, output,
		       textrelaoff, textrelasize,
		       textoff,
		       text.address,
		       strs,
		       strtaboff,
		       symtaboff, symtabsize, using_relas,
		       textsize, ELFLOADER_SEG_TEXT, rodatas);
    if(ret != ELFLOADER_OK) {
      PRINTF("elfloader: text failed\r\n");
      return ret;
    }
  }

  /* If we have any rodata segment relocations, we process them too. */
  
  if(rodatarelasize > 0) {
    PRINTF("elfloader: relocate rodata %d\r\n", rodatarelaoff, rodatarelasize, rodataoff);
    ret = copy_segment(input_fd, output,
		       rodatarelaoff, rodatarelasize,
		       rodataoff,
		       rodata.address,
		       strs,
		       strtaboff,
		       symtaboff, symtabsize, using_relas,
		       rodatasize, ELFLOADER_SEG_RODATA, rodatas);
    if(ret != ELFLOADER_OK) {
      PRINTF("elfloader: data failed\r\n");
      return ret;
    }
  } else {
	// When no relocation is performed, we still have to copy the rodata values.
     if(rodatasize) {
	     ret = elfloader_output_start_segment(output, ELFLOADER_SEG_RODATA,
						 rodata.address, rodatasize);
	     if(ret != ELFLOADER_OK) return ret;

	     copy_segment_data(input_fd, rodataoff, output, rodatasize);
     }
  }

  /* If we have any data segment relocations, we process them too. */
  
  if(datarelasize > 0) {
    PRINTF("elfloader: relocate data\r\n");
    ret = copy_segment(input_fd, output,
		       datarelaoff, datarelasize,
		       dataoff,
		       data.address,
		       strs,
		       strtaboff,
		       symtaboff, symtabsize, using_relas,
		       datasize, ELFLOADER_SEG_DATA, rodatas);
    if(ret != ELFLOADER_OK) {
      PRINTF("elfloader: data failed\n");
      return ret;
    }
    ret = elfloader_output_end_segment(output);
    if (ret != ELFLOADER_OK) return ret;
  } else {
     if(datasize) {
	     ret = elfloader_output_start_segment(output, ELFLOADER_SEG_DATA,
						 data.address, datasize);
	     if(ret != ELFLOADER_OK) return ret;

	     copy_segment_data(input_fd, dataoff, output, datasize);
     }
  }

  /* Write text and rodata segment into flash and data segment into RAM. */
/*   elfloader_arch_write_rom(fd, textoff, textsize, text.address); */
/*   elfloader_arch_write_rom(fd, rodataoff, rodatasize, rodata.address); */

  {
    /* Write zeros to bss segment */
    unsigned int len = bsssize;
    static const char zeros[16] = {0};
    ret = elfloader_output_start_segment(output, ELFLOADER_SEG_BSS,
					 bss.address,bsssize);
    if (ret != ELFLOADER_OK) return ret;
    while(len > sizeof(zeros)) {
      ret = elfloader_output_write_segment(output, zeros, sizeof(zeros));
      if (ret != sizeof(zeros)) return ELFLOADER_OUTPUT_ERROR;
      len -= sizeof(zeros);
    }
    ret = elfloader_output_write_segment(output, zeros, len);
    if (ret != len) return ELFLOADER_OUTPUT_ERROR;
  }

  PRINTF("elfloader: elf application environment search\r\n");

  localSymbol = find_local_symbol(input_fd, ELF_APPLICATION_ENVIRONMENT_NAME, symtaboff, symtabsize, strtaboff, rodatas);

  cleanup_rodatas(rodatas);

  if(localSymbol != NULL) {
    PRINTF("elfloader: elf application environment %p\r\n", localSymbol);
    elf_application_environment = (struct elf_application_environment_t *)localSymbol;

    return ELFLOADER_OK;
  }
/*
    PRINTF("elfloader: no autostart\n");
    process = (struct process **) find_program_processes(fd, symtaboff, symtabsize, strtaboff);
    if(process != NULL) {
      PRINTF("elfloader: FOUND PRG\n");
    }*/
    PRINTF("elfloader: elf application environment not found\r\n");
    return ELFLOADER_NO_STARTPOINT;
}
/*---------------------------------------------------------------------------*/
