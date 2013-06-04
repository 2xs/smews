#include <stdint.h>
extern uint32_t alloc_buffer[];


void init_debug_serial(void);

void init_tab8(uint8_t *ptr, uint16_t size, uint8_t magic_number);
void init_tab16(uint16_t *ptr, uint16_t size, uint16_t magic_number);
void init_tab32(uint32_t *ptr, uint16_t size, uint32_t magic_number);

uint16_t stack_free(void);    // unused stack since last call

void dump_stack(void);
