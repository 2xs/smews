#ifndef __LCD_H__
#define __LCD_H__

#ifdef MBED_USE_LCD_DISPLAY

void lcd_init_ports(void);
void lcd_clear(void);
int lcd_putchar(int c);

#endif
#endif
