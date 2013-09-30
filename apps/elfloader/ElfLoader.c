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

#include "application.h"

struct file_t {
  char *filename;
  uint16_t size;
};

/* elf loader*/
const char *elf_loader_return_labels[] = {
 "ok", "Bad Elf Header", "No Symbol Table", "No String Table", "No Text section", "Symbol not found", "Segment not found", "No StartPoint",
 "Unhandled Relocation", "Out Of Range", "Relocation Not Sorted", "Input Error", "Output Error"
};


/* Storage buffer */
#define STORAGE_BUFFER_SIZE 512

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
extern char _text_end;
/*-----------------------------------------------------------------------------*/
static char initElfLoader() {
  
  elf_applications_init(elf_allocator_flash_alloc, NULL);
  printf("text end %p\r\n", &_text_end);
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
    /*printf("%d\r\n", i);*/

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
      printf("%d\r\n", k);
      storage_buffer[0] = value;
      j = 1;
    }
    
    i++;
  }

  if(j > 0) {
    if(APPLICATION_WRITE(((uint8_t *)elf_allocator_storage) + k, storage_buffer, j) != 0) {
        printf("An error happened while flashing elf to storage\r\n");
        return 1;
      }
  }
  file->size = i;

  //printf("File Size is %d\r\n", file->size);

    
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
      printf("Elf Application Environment is %p\r\n", elf_application_environment);

      printf("Install function is %p\r\n", elf_application_environment->install);
      printf("Remove function is %p\r\n", elf_application_environment->remove);
      printf("URLS Tree is %p\r\n", elf_application_environment->urls_tree);
      printf("Resources index is %p\r\n", elf_application_environment->resources_index);

      if(!application_add(file->filename, file->size, elf_application_environment)) {
	out_str("Failed to add application ");
	out_str(file->filename);
        out_str(".");
        printf("Failed to add application %s.\r\n", file->filename);
	clean_up(file);
        return 1;
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
