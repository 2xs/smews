#include <stdlib.h> 
#include <stdio.h> 
#include <avr/io.h>
#include <avr/interrupt.h> 
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <string.h>
#include "mcu.h" 
#include "compiler.h"

/***********************************************************************************************
* ATMega128L port init (Led directions as output)
************************************************************************************************/
void PORT_Init(void)
{
    //init led direction
    DDRA |= (1<<RLED)|(1<<GLED)|(1<<YLED); 
    RLED_DISABLE();
    GLED_DISABLE();
    YLED_DISABLE();
    //init fifo as input
    DDRB &= ~BM(FIFO);  
    
    //init SFD CCA and FIFOP as inputs
    DDRD &= ~(BM(SFD)|BM(CCA)|BM(FIFOP));
    
    //init rest and vreg as output
    DDRA |= (BM(RESET_N)|BM(VREG_EN));
    
    //init value of reset and vreg
    PORTA &= ~(BM(RESET_N)|BM(VREG_EN));
}


/***********************************************************************************************
* SPI port init
************************************************************************************************/
void SPI_Init(void)
{
    unsigned char temp;
    /* Set MOSI (DDB2) and SCK (DDB1)(output, all others input) */
    DDRB |= (1<<CSN)|(1<<SCK)|(1<<MOSI);
    PORTB |= (1<<CSN);    
    /* Enable SPI, Master, set clock rate fck/16 : 1Mhz */  
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
        
    SPSR = 0x00;
    temp = SPSR;
    temp = SPDR; //free interrupts 
}   

/***********************************************************************************************
* SPI read/write
************************************************************************************************/

unsigned char spi(unsigned char data)
{
	SPDR=data;
	while ((SPSR & (1<<SPIF))==0);
	return SPDR;
}



