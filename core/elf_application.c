#ifndef DISABLE_ELF

#include "elf_application.h"
#include "memory.h"

unsigned int elf_applications_count __attribute__ ((section(".persistent_data"))) = 0;

struct elf_application_t *all_applications __attribute__ ((section(".persistent_data"))) = 0;

/* Post Url termination character */
#define URL_POST_END 255

/* "404 Not found" handler */
#define http_404_handler apps_httpCodes_404_html_handler
extern CONST_VAR(struct output_handler_t, apps_httpCodes_404_html_handler);

static elf_application_allocate_t elf_application_allocate;
static elf_application_free_t     elf_application_free;

void elf_applications_init(elf_application_allocate_t allocate, elf_application_free_t free) {
  elf_application_allocate = allocate;
  elf_application_free     = free;
}

int elf_application_get_count() { return elf_applications_count; }


char elf_application_add(struct elf_application_t *application) {
  struct elf_application_parsing_t *parsing;
  /* Head insert */
  struct elf_application_t *buf[1];
  unsigned int i;
  const struct output_handler_t * /*CONST_VAR*/ output_handler;

  /*application->previous = NULL; */
  buf[0] = NULL;
  APPLICATION_WRITE(&application->previous, buf, sizeof(struct elf_application_t *));

  if(all_applications == NULL) {
    /* application->next = NULL; */
    APPLICATION_WRITE(&application->next, buf, sizeof(struct elf_application_t *));
  } else {
    /* application->next          = all_applications; */
    APPLICATION_WRITE(&application->next, &all_applications, sizeof(struct elf_application_t *));
    /* all_applications->previous = application; */
    APPLICATION_WRITE(&all_applications->previous, application, sizeof(struct elf_application_t *));
  }

  /* all_applications             = application; */
  APPLICATION_WRITE(&all_applications, &application, sizeof(struct elf_application_t *));

  {
    unsigned int elfApplicationsCount = elf_applications_count + 1;
    APPLICATION_WRITE((void *)&elf_applications_count, &elfApplicationsCount, sizeof(unsigned int));
  }

  i = 0;
  while((output_handler = CONST_ADDR(application->environment->resources_index[i])) != NULL) {

    if(CONST_UI8(output_handler->handler_type) == type_generator 
  #ifndef DISABLE_GP_IP_HANDLER
       || CONST_UI8(output_handler->handler_type) == type_general_ip_handler
  #endif
                                                               ) {
      generator_init_func_t * init_handler = CONST_ADDR(GET_GENERATOR(output_handler).init);
      if(init_handler)
        init_handler();
    }

    i++;
  }

  /* Add all existing connections to the application's parsing */
  FOR_EACH_CONN(conn, {
	      if(!IS_HTTP(conn)) {
		      NEXT_CONN(conn);
		      continue;
	      }

        parsing = (struct elf_application_parsing_t *)mem_alloc(sizeof(struct elf_application_parsing_t));
        parsing->connection = conn;
        parsing->blob       = application->environment->urls_tree;
        parsing->previous   = NULL;
        parsing->next       = application->parsing;

        if(application->parsing != NULL) {
          application->parsing->previous = parsing;
        }
  
        APPLICATION_WRITE(&application->parsing, &parsing, sizeof(struct elf_application_parsing_t *));
  })

  return 1;
}

void elf_application_remove(const struct elf_application_t *application) {
    if(elf_application_get_count() > 0 ) {
      struct elf_application_t *appli = all_applications;

      while(appli) {
        if(application == appli) {

          /* remove the parsing data */
          {
            struct elf_application_parsing_t *parsing = appli->parsing;
            struct elf_application_parsing_t *parsingNext;
            while(parsing) {
              parsingNext = parsing->next;
              mem_free(parsing, sizeof(struct elf_application_parsing_t));
              parsing = parsingNext;
            }
          }

          /* Now remove the application by itself */
          if(application == all_applications) {

            /* all_applications = application->next; */
            APPLICATION_WRITE(all_applications, application->next, sizeof(struct elf_application_t *));

            if(all_applications != NULL) {
              /* all_applications->previous = NULL; */
              struct elf_application_t *buf[1];
              buf[0] = NULL;
              APPLICATION_WRITE(all_applications->previous, buf, sizeof(struct elf_application_t *));
            }

          } else {
            /* application->previous->next = application->next; */
            APPLICATION_WRITE(application->previous->next, application->next, sizeof(struct elf_application_t *));
            
            /* application->next->previous = application->previous; */
            APPLICATION_WRITE(application->next->previous, application->previous, sizeof(struct elf_application_t *));
          }

          {
            unsigned int elfApplicationsCount = elf_applications_count - 1;
            APPLICATION_WRITE((void *)&elf_applications_count, &elfApplicationsCount, sizeof(unsigned int));
          }
          return;
      }
      appli = appli->next;
    }
  }
}

/* When the device resets, init all dynamic applications*/
void elf_application_init() {

    if(elf_application_get_count() > 0) {
      struct elf_application_t *appli = all_applications;
      const struct output_handler_t * /*CONST_VAR*/ output_handler;
      int i;

      while(appli) {


        if(appli->data_size>0) {
          memcpy(appli->data_destination, appli->data_source, appli->data_size);
        }


        i = 0;
        while((output_handler = CONST_ADDR(appli->environment->resources_index[i])) != NULL) {

          if(CONST_UI8(output_handler->handler_type) == type_generator 
#ifndef DISABLE_GP_IP_HANDLER
             || CONST_UI8(output_handler->handler_type) == type_general_ip_handler
#endif
                                                                     ) {
            generator_init_func_t * init_handler = CONST_ADDR(GET_GENERATOR(output_handler).init);
            if(init_handler)
	      init_handler();
          }

          i++;
        }

        appli = appli->next;
      }

    }
}

/* When a connection has been created */
void elf_application_add_connection(struct connection *connection) {
  if(connection) {

    if(!IS_HTTP(connection))
	return;

    if(elf_application_get_count() > 0 ) {
      struct elf_application_parsing_t *parsing;
      struct elf_application_t *        appli = all_applications;


      while(appli) {

        parsing = (struct elf_application_parsing_t *)mem_alloc(sizeof(struct elf_application_parsing_t));
        /* Head insert */
        parsing->connection = connection;
        parsing->blob       = appli->environment->urls_tree;
        parsing->previous   = NULL;
        parsing->next       = appli->parsing;

        if(appli->parsing != NULL)
          appli->parsing->previous = parsing;

        APPLICATION_WRITE(&appli->parsing, &parsing, sizeof(struct elf_application_parsing_t *));

        appli = appli->next;
      }
    }
  }
}
/* When a connection has been terminated */
void elf_application_remove_connection(const struct connection *connection) {
  if(connection) {
    if(elf_application_get_count() > 0 ) {
      struct elf_application_parsing_t *parsing;
      struct elf_application_t *        appli = all_applications;

      while(appli) {
        parsing = appli->parsing;

        while(parsing) {

          if(parsing->connection == connection) {
            if(parsing->previous)
              parsing->previous->next = parsing->next;

            if(parsing->next)
              parsing->next->previous = parsing->previous;

            if(parsing == appli->parsing) {
              APPLICATION_WRITE(&appli->parsing, &parsing->next, sizeof(struct elf_application_parsing_t *));
            }

            mem_free(parsing, sizeof(struct elf_application_parsing_t));
            break;
          }

          parsing = parsing->next;
        }

        appli = appli->next;
      }
    }
  }
}

struct output_handler_t *url_parse_step(uint8_t byte, unsigned char **url_blob, const struct output_handler_t **resources_index) {
  unsigned char *blob;
  unsigned char blob_curr;

  blob = *url_blob;
  blob_curr = CONST_READ_UI8(blob);

  if(blob_curr == URL_POST_END) {
    blob++;    
    blob_curr = CONST_READ_UI8(blob);
  }
  
  if(blob_curr >= 128) {
    if (byte == ' ') {
      printf("======> OUTPUT HANDLER FOUND %p %d<=====\r\n", CONST_ADDR(resources_index[blob_curr - 128]), blob_curr - 128); 
      return (struct output_handler_t*)CONST_ADDR(resources_index[blob_curr - 128]);
    }
    
    blob++;    
    blob_curr = CONST_READ_UI8(blob);
  }

  do {
    unsigned char offsetInf = 0;
    unsigned char offsetEq = 0;
    unsigned char blob_next;

    blob_curr = CONST_READ_UI8(blob);
    blob_next = CONST_READ_UI8(++blob);
    if (byte != blob_curr && blob_next >= 128) {
      blob_next = CONST_READ_UI8(++blob);
    }

    if (blob_next < 32) {
      offsetInf += 
        ((blob_next>>2) & 1) + ((blob_next>>1) & 1) + (blob_next & 1);

      offsetEq = offsetInf + ((blob_next & 2)?CONST_READ_UI8(blob+1):0);
    }
    if (byte == blob_curr) {
      if (blob_next < 32) {
        if (blob_next & 2)
          blob += offsetEq;
        else {
          *url_blob = blob;
          return &http_404_handler;
        }
      }

      *url_blob = blob;
      return NULL;
    } 
    else if (byte < blob_curr) {
      if (blob_next < 32 && blob_next & 1) {
        blob += offsetInf;
      }
      else {
        *url_blob = blob;
        return &http_404_handler;
      }
    }
    else {
      if (blob_next < 32 && blob_next & 4) {
        unsigned char offsetSup = offsetEq + ((blob_next & 3) ? CONST_READ_UI8(blob + (offsetInf - 1)) : 0);

        blob += offsetSup;
      }
      else {
        *url_blob = blob;
        return &http_404_handler;
      }
    }
  } while(1);

  return NULL;
}

void elf_application_parsing_start(const struct connection *connection) {
  if(elf_application_get_count() > 0) {
    struct elf_application_parsing_t *parsing;
    struct elf_application_t *appli = all_applications;
    while(appli) {
      
      parsing = appli->parsing;

      while(parsing) {
        
        if(parsing->connection == connection) {
          parsing->blob = appli->environment->urls_tree;
          break;
        }

        parsing = parsing->next;
      }

      appli = appli->next;
    }
  }
}

struct output_handler_t *elf_application_parse_step(const struct connection *connection, uint8_t byte) {
  if(elf_application_get_count() > 0) {
    struct elf_application_parsing_t *parsing;
    struct output_handler_t *output_handler = NULL;
    struct elf_application_t *appli = all_applications;
    char still_parsing = 0;

    while(appli) {
      
      parsing = appli->parsing;

      while(parsing) {
        
        if(parsing->connection == connection) {
          output_handler = url_parse_step(byte, &parsing->blob, appli->environment->resources_index);

          if(output_handler == NULL)
            still_parsing = 1;
          else {
            if(output_handler != &http_404_handler)
              return output_handler;
          }

          break;
        }

        parsing = parsing->next;
      }

      appli = appli->next;
    }

    if(still_parsing == 1) 
      return NULL;
  }
  return &http_404_handler;
}

#endif
