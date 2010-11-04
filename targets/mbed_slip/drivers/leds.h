#ifndef __LEDS_H__
#define __LEDS_H__

/* LEDS GPIO Registers */
// GPIO1 Mask register, FIOMASK page 122 
#define LEDS_GPIO_MASK *(uint32_t *)(0x2009C030)
// GPIO1 Pin register, FIOPIN page 122
#define LEDS_GPIO_PIN  *(uint32_t *)(0x2009C034)
// GPIO1 Direction, FIO1DIR page 122
#define LEDS_GPIO_DIR  *(uint32_t *)(0x2009C020)

/* LEDS GPIO Pins */
#define LED1 (1 << 18)
#define LED2 (1 << 20)
#define LED3 (1 << 21)
#define LED4 (1 << 23)
#define LEDS_MASK (LED1 | LED2 | LED3 | LED4)

#define LEDS_SET(value) LEDS_GPIO_MASK = ~LEDS_MASK; \
                        LEDS_GPIO_PIN = value;

#define LEDS_INIT       LEDS_GPIO_DIR |= LEDS_MASK;


#endif