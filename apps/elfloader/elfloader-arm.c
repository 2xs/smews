#include <rflpc17xx/rflpc17xx.h>
#include "elfloader-arch-otf.h"

#include "RamFileSystem.h"

#if 1
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

#define ELF32_R_TYPE(info)      ((unsigned char)(info))


/*Elf32_ST_TYPE*/
#define STT_NOTYPE  0
#define STT_OBJECT  1
#define STT_FUNC    2
#define STT_SECTION 3
#define STT_FILE    4
#define STT_LOPROC  13
#define STT_HIPROC  15

#define ELF32_ST_TYPE(i)        ((i)&0xf)


/* Supported relocations */
#define R_ARM_ABS32	2
#define R_ARM_THM_CALL	10
#define R_ARM_THM_MOVW_ABS_NC 47
#define R_ARM_THM_MOVT_ABS    48

struct mov_relocation_data {
  int offsets[4];
  int mask;
};

const struct mov_relocation_data movw_relocation_data = {
	{12, 11, 8, 0}, 0xFFFF
};

const struct mov_relocation_data movt_relocation_data = {
	{28, 27, 24, 16}, 0xFFFF0000
};

/* Adapted from elfloader-avr.c */
int
elfloader_arch_relocate(void *input_fd,
			struct elfloader_output *output,
			unsigned int sectionoffset,
			char *sectionaddr,
                        struct elf32_rela *rela, char *addr,
			struct elf32_sym *symbol)
{
  unsigned int type;

  type = ELF32_R_TYPE(rela->r_info);
  rfs_seek(input_fd, sectionoffset + rela->r_offset);

/*   PRINTF("elfloader_arch_relocate: type %d\n", type);
   PRINTF("Addr: %p, Addend: %ld\n",   addr, rela->r_addend); */
  switch(type) {
  case R_ARM_ABS32:
    {
      int32_t addend;
      rfs_read((char*)&addend, 4, 1, input_fd);
      addr += addend;
      elfloader_output_write_segment(output,(char*) &addr, 4);
      /*printf("%p: addr: %p addend %p\r\n", sectionaddr +rela->r_offset,
	     addr, addend);*/
    }
    break;

  case R_ARM_THM_CALL: {
	uint16_t instr[2];
	int32_t offset;
	int32_t addend;

        rfs_read((char *)instr, 4, 1, input_fd);

        // Build the addend from the instructions
	addend = (instr[0] & 0x7FF) << 12 | (instr[1] & 0x7FF) << 1;

	// Sign extent, when we have a negative number, we preserve it through shifting.
	if(addend & (1<<22))
	{
		//printf("%x\r\n", addend);
		addend |= 0xFF8<<20;
		//printf("%x\r\n", addend);
	}

	//printf("R_ARM_THM_CALL addend : %x %d\r\n",addend, addend);
	// S + A
	offset = addend + (uint32_t)addr;

	//printf("R_ARM_THM_CALL S + A : %x\r\n",offset);

	if(ELF32_ST_TYPE(symbol->st_info) == STT_FUNC) {
		// (S + A) | T
		offset |= 0x1;
		//PRINTF("elfloader-arm.c: R_ARM_THM_CALL Symbol is STT_FUNC\r\n");
	}

	//printf("R_ARM_THM_CALL OFFSET (S + A) | T : %x\r\n",offset);

	// ((S+A) | T) - P
	offset = offset - ((uint32_t)sectionaddr + (rela->r_offset));

	//printf("R_ARM_THM_CALL ((S+A) | T) - P: %x\r\n",offset);

	instr[0] = ((offset >> 12) & 0x7FF) | 0xF000;
	instr[1] = ((offset >> 1) & 0x7FF) | 0xF800;

	/*printf("R_ARM_THM_CALL after relocation: %04x %04x\r\n",instr[0], instr[1]);*/
	elfloader_output_write_segment(output, (char*)instr, 4);
  }
   break;


  case R_ARM_THM_MOVW_ABS_NC : 
  case R_ARM_THM_MOVT_ABS : {
	uint16_t instr[2];
	uint32_t mask;
	int16_t addend = 0;
	uint32_t val;
	const struct mov_relocation_data *mov_relocation_data = 
		( type == R_ARM_THM_MOVW_ABS_NC ) ?
			&movw_relocation_data : &movt_relocation_data;

	rfs_read((char*)instr, 4, 1, input_fd);

	/*PRINTF("elfloader-arm.c: relocation %d\r\n", type);
	PRINTF("elfloader-arm.c: R_ARM_THM_MOV before relocation %x %x\r\n", instr[0], instr[1]);*/

	// Build the 16 bit addend from the instructions
	addend |= (instr[0] & 0xF) << 12;
	addend |= ((instr[0] >> 10) & 0x1) << 11;
	addend |= ((instr[1]  >> 12) & 0x7) << 8;
	addend |= instr[1] & 0xF;

	/*printf("A: %x %d\r\n", addend, addend);
	printf("S: %x %d\r\n", addr, addr);*/

	// S + A
      val = (uint32_t) addr + addend;
      //printf("S+A %d %x\r\n", val, val);
	
	if(type == R_ARM_THM_MOVW_ABS_NC) {
	// We are in thumb mode, we just have to check whether the symbol type is STT_FUNC
		if(ELF32_ST_TYPE(symbol->st_info) == STT_FUNC) {
			// (S + A) | T
			val = val | 0x1;
			//PRINTF("elfloader-arm.c: R_ARM_THM_MOVW_ABS_NC Symbol is STT_FUNC\r\n");
		}
	}

	// Result_Mask
        mask = val & mov_relocation_data->mask;

	instr[0] &= ~0xF;
	instr[0] |= (mask  >> mov_relocation_data->offsets[0]) & 0xF;
	instr[0] &= ~(1<<10);
	instr[0] |= ((mask >> mov_relocation_data->offsets[1]) & 0x1)<<10;
	
	instr[1] &= ~(0x7<<12);
	instr[1] |= ((mask >> mov_relocation_data->offsets[2]) & 0x7)<<12;
	instr[1] &= ~0xFF;
        instr[1] |= (mask >> mov_relocation_data->offsets[3]) & 0xFF;
	
        //PRINTF("elfloader-arm.c: R_ARM_THM_MOV after relocation %x %x\r\n", instr[0], instr[1]);
	elfloader_output_write_segment(output, (char*)instr, 4);
  }
  break;

  default:
    printf("elfloader-arm.c: unsupported relocation type %d\n", type);
    return ELFLOADER_UNHANDLED_RELOC;
  }

  return ELFLOADER_OK;
}
