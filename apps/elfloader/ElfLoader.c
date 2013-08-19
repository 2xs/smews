/*
<generator>
	<handlers init="initElfLoader" doPostOut="doPostOut" doPostIn="doPostIn"/>
	<content-types>
		<content-type type="application/x-object"/>
	</content-types>
	
</generator>
*/

#include "elf_allocator.h"

#include "elfloader-otf.h"
#include "codeprop-otf.h"
#include "RamFileSystem.h"

#include "../../core/elf_application.h"

struct file_t {
  char *filename;
  uint16_t size;
};

/* elf loader*/
const char *elf_loader_return_labels[] = {
 "ok", "Bad Elf Header", "No Symbol Table", "No String Table", "No Text section", "Symbol not found", "Segment not found", "No StartPoint"
};

/* Storage buffer */
#define STORAGE_BUFFER_SIZE 128

static uint8_t storage_buffer[STORAGE_BUFFER_SIZE];

void flash_dump() {
  int i, index, maxInfosPerLine;
  int allocatorSize = 256;
  char *flash = (char *)elf_allocator_storage;

  maxInfosPerLine = 16;
  
  printf("\r\nFlash Dump:\r\n");
  printf("---------\r\n");

  index = 0;
  while(allocatorSize > maxInfosPerLine) {

    printf("%p ", flash + index);

    for(i = 0; i < maxInfosPerLine; i++, index++)
      printf("%02X, ", flash[index]);

    printf("\r\n");
    allocatorSize -= maxInfosPerLine;
  }

  if(allocatorSize>0) {
    printf("%p ", flash + index);
    for(i = 0; i < allocatorSize; i++, index++)
      printf("%02X, ", flash[index]);
    printf("\r\n");
  }

  printf("\r\n");
}

/*-----------------------------------------------------------------------------*/
static char initElfLoader() {
  
  elf_applications_init(elf_allocator_flash_alloc, NULL);

  return 1;
}

/*-----------------------------------------------------------------------------*/
static char doPostIn(uint8_t content_type, /*uint16_t content_length,*/ uint8_t call_number, 
                     char *filename, void **post_data) {
  uint16_t i = 0;
  uint16_t j = 0;
  uint16_t k = 0;
  short value;
  printf("%s IN\r\n", __FUNCTION__);

  if(!filename)  return 1;

  // Leave when data is already available..
  if(*post_data) return 1;

  struct file_t *file = mem_alloc(sizeof(struct file_t));
  if(!file)
    return 1;

  /* Filename */
  while(filename[i++] != '\0');

  file->filename = mem_alloc(i * sizeof(char));
  if(!file->filename) {
    mem_free(file, sizeof(struct file_t));
    return 1;
  }

  i = 0;
  do {
    file->filename[i] = filename[i];
  }while(filename[i++] != '\0');

  /* Size + storage */
  i = 0;
  while((value = in()) != -1) {
    //printf("%c", value);

    if(j < STORAGE_BUFFER_SIZE) {
      storage_buffer[j] = value;
      j++;
    } else {

      /* Commit buffer */
      if(APPLICATION_WRITE(((uint8_t *)elf_allocator_storage) + k, storage_buffer, STORAGE_BUFFER_SIZE) != 0) {
        printf("An error happened while flashing elf to storage\r\n");
        return 1;
      }

      k += STORAGE_BUFFER_SIZE;
      storage_buffer[0] = value;
      j = 1;
    }
    
    
    /*if(rflpc_iap_write_to_sector(elf_allocator_storage + i, &value, 1) != 0) {
        printf("An error happened while flashing elf to storage\r\n");
        return 1;
    }*/
    i++;
  }

  if(j > 0) {
    if(rflpc_iap_write_to_sector(elf_allocator_storage + k, storage_buffer, j) != 0) {
        printf("An error happened while flashing elf to storage\r\n");
        return 1;
      }
  }

  file->size = i;

//  printf("File Size is %d\r\n", file->size);

    
  *post_data = file;
  printf("%s OUT\r\n", __FUNCTION__);
  return 1;
}

void clean_up(struct file_t *file) {
    uint16_t i = 0;
    while(file->filename[i++] != '\0');

    mem_free(file->filename, i * sizeof(char));
    mem_free(file, sizeof(struct file_t));        
}

/*-----------------------------------------------------------------------------*/
static char doPostOut(uint8_t content_type, void *data) {

  printf("%s IN\r\n", __FUNCTION__);
  if(data) {

    uint16_t i;
    struct file_t *file =  (struct file_t*)data;
    void *storage_handle;
    int loading;

    out_str("The file \"");
    out_str(file->filename);
    out_str("\" contains ");
    out_uint(file->size);
    out_str(" characters\n");

    storage_handle   = rfs_open((void *)elf_allocator_storage);

    if(storage_handle == NULL) {
      out_str("Unable to open elf storage");
      clean_up(file);
      return 1;
    }

    elfloader_init();
    printf("ElfLoader loading...\r\n");
    loading = elfloader_load(storage_handle, codeprop_output);

    printf("Loading = %d\r\n", loading);
    if(loading == ELFLOADER_OK) {
      /*flash_dump();*/

      printf("PRINTF address %p\r\n", printf);
      printf("Calling Callbacks Getter %p\r\n", callbacksGetter);
      printf("Callbacks Getter is : %p\r\n", callbacksGetter());
      printf("Callbacks Getter called\r\n");
      {
	  struct SCallbacks_t *callbacks = callbacksGetter();

	  printf("Callback 0 %p\r\n", callbacks->c0);
	  printf("Callback 1 %p\r\n", callbacks->c1);
	  printf("%d\r\n", callbacks->c0());
	  printf("%d\r\n", callbacks->c1());
      }


    } else {

      if(loading < 0) {
        out_str("An internal error happened\r\n");
      } else {
        out_str("An error happened while loading ");
        out_str(file->filename);
        out_str(" : ");
        out_str(elf_loader_return_labels[loading]);
      }

      rfs_close(storage_handle);
      clean_up(file);

    }

  } else
    out_str("Please provide a file");

  printf("%s OUT\r\n", __FUNCTION__);
  return 1;
}
