#ifndef DISABLE_ELF

#include "elf_application.h"

static unsigned int elf_applications_count = 0;

struct elf_application_t *all_applications;

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
  // Head insert
  struct elf_application_t *buf[1];
  /*application->previous = NULL; */
  buf[0] = NULL;
  APPLICATION_WRITE(application->previous, buf, sizeof(struct elf_application_t *));

  if(all_applications == NULL) {
    /* application->next = NULL; */
    APPLICATION_WRITE(application->next, buf, sizeof(struct elf_application_t *));
  } else {
    /* application->next          = all_applications; */
    APPLICATION_WRITE(application->next, all_applications, sizeof(struct elf_application_t *));
    /* all_applications->previous = application; */
    APPLICATION_WRITE(all_applications->previous, application, sizeof(struct elf_application_t *));
  }

  /* all_applications             = application; */
  APPLICATION_WRITE(all_applications, application, sizeof(struct elf_application_t *));
  elf_applications_count++;

  return 1;
}

void elf_application_remove(const struct elf_application_t *application) {
    if(elf_applications_count > 0 ) {
      struct elf_application_t *appli = all_applications;

      while(appli) {
        if(application == appli) {

          /* remove the parsing data */
          if(elf_application_free) {
            struct elf_application_parsing_t *parsing = appli->parsing;
            struct elf_application_parsing_t *parsingNext;
            while(parsing) {
              parsingNext = parsing->next;
              elf_application_free(parsing);
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

          elf_applications_count--;
          return;
      }
      appli = appli->next;
    }
  }
}

static struct elf_application_parsing_t elf_application_parsing_buffer;

/* When a connection has been created */
void elf_application_add_connection(struct connection *connection) {
  if(connection && elf_application_allocate) {
    if(elf_applications_count > 0 ) {
      struct elf_application_parsing_t *parsing;
      struct elf_application_t *        appli = all_applications;

  		while(appli) {
        /* Head insert */
        elf_application_parsing_buffer.connection = connection;
        elf_application_parsing_buffer.blob       = appli->environment->urls_tree;
        elf_application_parsing_buffer.previous   = NULL;
        elf_application_parsing_buffer.next       = appli->parsing;
        
        parsing = (struct elf_application_parsing_t *)elf_application_allocate(sizeof(struct elf_application_parsing_t));

        APPLICATION_WRITE(parsing, &elf_application_parsing_buffer, sizeof(struct elf_application_parsing_t));

        if(appli->parsing != NULL) {
          APPLICATION_WRITE(appli->parsing->previous, parsing, sizeof(struct elf_application_parsing_t *));
        }
  
        APPLICATION_WRITE(appli->parsing, parsing, sizeof(struct elf_application_parsing_t *));

        appli = appli->next;
      }
    }
  }
}
/* When a connection has been terminated */
void elf_application_remove_connection(const struct connection *connection) {
  if(connection && elf_application_free) {
    if(elf_applications_count > 0 ) {
      struct elf_application_parsing_t *buf[1];
      struct elf_application_parsing_t *parsing;
      struct elf_application_t *        appli = all_applications;

      buf[0] = NULL;

      while(appli) {
        
        parsing = appli->parsing;

        while(parsing) {
          if(parsing->connection == connection) {
              
            if(parsing->previous) {
              APPLICATION_WRITE(parsing->previous->next, parsing->next, sizeof(struct elf_application_parsing_t *));
            }

            if(parsing->next) {
              APPLICATION_WRITE(parsing->next->previous, parsing->previous, sizeof(struct elf_application_parsing_t *));
            }

            elf_application_free(parsing);
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
          APPLICATION_WRITE(url_blob, &blob, sizeof(unsigned char *));
          return &http_404_handler;
        }
      }

      APPLICATION_WRITE(url_blob, &blob, sizeof(unsigned char *));
      return NULL;
    } 
    else if (byte < blob_curr) {
      if (blob_next < 32 && blob_next & 1) {
        blob += offsetInf;
      }
      else {
        APPLICATION_WRITE(url_blob, &blob, sizeof(unsigned char *));
        return &http_404_handler;
      }
    }
    else {
      if (blob_next < 32 && blob_next & 4) {
        unsigned char offsetSup = offsetEq + ((blob_next & 3) ? CONST_READ_UI8(blob + (offsetInf - 1)) : 0);

        blob += offsetSup;
      }
      else {
        APPLICATION_WRITE(url_blob, &blob, sizeof(unsigned char *));
        return &http_404_handler;
      }
    }
  } while(1);

  return NULL;
}

void elf_application_parsing_start(const struct connection *connection) {
  if(elf_applications_count > 0) {
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
  if(elf_applications_count > 0) {
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
