#include <rflpc17xx/rflpc17xx.h>

#ifdef MBED_USE_LCD_DISPLAY

#include "lcd.h"

#define RS  MBED_DIP26
#define RW  MBED_DIP25
#define E   MBED_DIP24

#define DB4 MBED_DIP23
#define DB5 MBED_DIP22
#define DB6 MBED_DIP20
#define DB7 MBED_DIP19

#define NOP asm("nop")

static uint8_t row,col;

static void delay(uint32_t min_ns)
{
    uint32_t loops = min_ns / 10; /* 1 cycle is almost 10 ns at 96Mhz */
    while (--loops)
	NOP;
}

static void clock(void)
{
    delay(6000);
    rflpc_gpio_clr_pin(E);
    delay(6000);
    rflpc_gpio_set_pin(E);
}

static void write_nibble(uint8_t nibble)
{
    rflpc_gpio_set_pin_val(DB4, nibble & 1);
    rflpc_gpio_set_pin_val(DB5, (nibble >> 1) & 1);
    rflpc_gpio_set_pin_val(DB6, (nibble >> 2) & 1);
    rflpc_gpio_set_pin_val(DB7, (nibble >> 3) & 1);
    clock();
}

static void write_byte(uint8_t byte)
{
    write_nibble(byte >> 4);
    write_nibble(byte & 0xf);
}

static void write_command(uint8_t command)
{
    rflpc_gpio_clr_pin(RS);
    write_byte(command);
}

static void write_data(uint8_t data)
{
    rflpc_gpio_set_pin(RS);
    write_byte(data);
}


void lcd_init_ports(void)
{
    rflpc_gpio_set_pin_mode_output(RS,0);
    rflpc_gpio_set_pin_mode_output(RW,0);
    rflpc_gpio_set_pin_mode_output(E, 0);

    rflpc_gpio_set_pin_mode_output(DB4, 0);
    rflpc_gpio_set_pin_mode_output(DB5, 0);
    rflpc_gpio_set_pin_mode_output(DB6, 0);
    rflpc_gpio_set_pin_mode_output(DB7, 0);

    /* Configure LCD to use 4bits mode */
    rflpc_gpio_clr_pin(RS);
    rflpc_gpio_set_pin(E);
    rflpc_gpio_clr_pin(RW);
    delay(15000000);
    int i;
    for (i = 0 ; i < 3 ; ++i)
    {
	write_nibble(0x3);
	delay(1640000);
    }
    write_nibble(0x2); /* Set 4 bits mode */


    write_command(0x2c);
/*    write_command(0x0f);*/
    write_command(0x0c);
    lcd_clear();
}


void lcd_clear(void)
{
    write_command(1);
    delay(1600000); /* Command completion time */
    row = 0;
    col = 0;
}

static void move_cursor(int _row, int _col)
{
    row = _row;
    col = _col;
    if (row <= 0)
	row = 0;
    if (col <= 0)
	col = 0;
    if (row >= 4)
	row = 4;

    if (col >= 20)
    {
	col = 0;
	row++;
    }

    switch (row)
    {
	case 4: row = 0;
	case 0:
	    write_command(0x80 + col);
	    break;
	case 1:
	    write_command(0x80 + 0x40 + col);
	    break;
	case 2:
	    write_command(0x80 + 0x14 + col);
	    break;
	case 3:
	    write_command(0x80 + 0x54 + col);
	    break;
    }
}

int lcd_putchar(int c)
{
    switch (c)
    {
	case '\r': move_cursor(row,0); return c;
	case '\n': move_cursor(row+1,col); return c;
	case '\b': move_cursor(row,col-1); return c;
	default: 
	    if (row == 0 && col == 0)
		lcd_clear();
	    write_data(c);
	    move_cursor(row, col+1); break;
    }
    return c;
}

#endif
