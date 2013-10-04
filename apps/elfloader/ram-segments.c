#ifndef __RAM_SEGMENTS_C__1POIF5E8U4__
#define __RAM_SEGMENTS_C__1POIF5E8U4__

#include "elfloader-otf.h"
#include "codeprop-otf.h"


#include "elf_allocator.h"

#include "tinyLibC.h"

struct ram_output
{
  struct elfloader_output output;
  char *base;
  unsigned int offset, type;
  void *text;
  void *rodata;
  void *data;
  void *bss;
  void *flashLimit;
};

static void *
allocate_segment(struct elfloader_output * const output,
		 unsigned int type, int size)
{
  struct ram_output * const ram = (struct ram_output *)output;
  
  switch(type) {
  case ELFLOADER_SEG_TEXT:
    /*if (ram->text) mem_free(ram->text, ram->textSize);*/
    ram->text = elf_allocator_flash_alloc(size);
    return ram->text;

  case ELFLOADER_SEG_RODATA:
    /*if (ram->rodata) free(ram->rodata, ram->rodataSize);*/
    ram->rodata = elf_allocator_flash_alloc(size);
    return ram->rodata;

  case ELFLOADER_SEG_DATA:
    /*if (ram->data) free(ram->data, ram->dataSize);*/
    ram->data = elf_allocator_ram_alloc(size);
    return ram->data;

  case ELFLOADER_SEG_BSS:
    /*if (ram->bss) free(ram->bss, ram->bssSize);*/
    ram->bss = elf_allocator_ram_alloc(size);
    return ram->bss;
  }
  return 0;
}

static int
start_segment(struct elfloader_output *output,
			unsigned int type, void *addr, int size)
{
  ((struct ram_output*)output)->base   = addr;
  ((struct ram_output*)output)->offset = 0;
  ((struct ram_output*)output)->type   = type;
  return ELFLOADER_OK;
}

static int
end_segment(struct elfloader_output *output)
{
  return ELFLOADER_OK;
}

static int
write_segment(struct elfloader_output *output, const char *buf,
	      unsigned int len)
{
  struct ram_output * const memory = (struct ram_output *)output;
  /*printf("ram-segments.write Base %p + Ram Offset %x = %p (%d bytes)\r\n", memory->base, memory->offset, memory->base + memory->offset,len);*/

  switch(memory->type) {
	case ELFLOADER_SEG_TEXT :
	case ELFLOADER_SEG_RODATA : {
	   int ret;

	   if(memory->flashLimit) {
	     if( ((memory->base + memory->offset) <= (char *)memory->flashLimit) &&
		 ((memory->base + memory->offset + len) >= (char *)memory->flashLimit)) {
	       printf("No more available flash memory\r\n");
	       return -2;
	     }
	   }

	   ret = rflpc_iap_write_buffer(memory->base + memory->offset, buf, len);


	   if(ret != 0) {
		printf("An error happened while writing to flash %d\r\n", ret);
		return ret;
	   }
	}
	break;
	default :
	  fake_memcpy(memory->base + memory->offset, buf, len);
  }

  memory->offset += len;
  return len;
}

static unsigned int
segment_offset(struct elfloader_output *output)
{
  return ((struct ram_output*)output)->offset;
}

static const struct elfloader_output_ops elf_output_ops =
  {
    allocate_segment,
    start_segment,
    end_segment,
    write_segment,
    segment_offset
  };


static struct ram_output seg_output = {
  {&elf_output_ops},
  0, 0,
  0,
  0, 0, 0, 0, 0
};

struct elfloader_output *codeprop_output = &seg_output.output;

void ram_segments_set_flash_limit(void *aLimit) {
  seg_output.flashLimit = aLimit;
}

#endif /* __RAM_SEGMENTS_C__1POIF5E8U4__ */
