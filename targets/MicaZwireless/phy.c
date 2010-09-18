#include <stdlib.h> 
#include <stdio.h> 
#include <avr/io.h>
#include <avr/interrupt.h> 
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "phy.h" 
#include "cc2420.h"
#include "mcu.h"
#include "compiler.h"
#include "config.h"


/***********************************************************************************************
* 
************************************************************************************************/
unsigned char WriteStrobeReg_spi(unsigned char cmd)
{
    unsigned char Value;
    SPI_ENABLE();   //CSn low                    
    Value = spi(0x00|cmd);
    SPI_DISABLE();  //CSn high 
    //_delay_ms(1);
    return Value;
}

unsigned char WriteStrobeReg(unsigned char cmd)
{
    unsigned char Value;
    Value = spi(0x00|cmd);
    //_delay_ms(1);
    return Value;
}


/***********************************************************************************************
* 
************************************************************************************************/
unsigned char WriteConfigReg_spi(unsigned char cmd, unsigned int data)
{
    unsigned char TempH,TempL,Value;
    TempH = (data >> 8);
    TempL = (data & 0x00FF);

    SPI_ENABLE();   //CSn low                    
    Value = spi(0x00|cmd);
    spi(TempH);
    spi(TempL);   
    SPI_DISABLE();  //CSn high 

    //_delay_ms(1);
    return Value;
} 



/***********************************************************************************************
* 
************************************************************************************************/
unsigned int ReadConfigReg_spi(unsigned char cmd)
{
    unsigned char Status,ValueH,ValueL;
    unsigned int  Value;
    
    SPI_ENABLE();   //CSn low       
    Status = spi(0x40|cmd); 
    ValueH = spi(0x00); 
    ValueL = spi(0x00);
    SPI_DISABLE();  //CSn high 
    
    Value = ValueH ;
    Value = Value<<8 ;
    Value |= ValueL ;
    
    return Value;
}




/***********************************************************************************************
*
************************************************************************************************/
void Write_TXFIFO(unsigned char *data,unsigned char n)
{
    unsigned char i;
    
    SPI_ENABLE();
    WriteStrobeReg(CC2420_TXFIFO);
    for (i = 0; i < (n); i++) 
    {
        spi(*data++);
    }
    SPI_DISABLE();
}



/***********************************************************************************************
* 
************************************************************************************************/
void WriteRAM_spi(unsigned int cmd,unsigned char data[],unsigned char n)
{
    unsigned char cmdTempH,cmdTempL,i;
    cmdTempH = (0x007F & cmd);
    cmdTempL = ((cmd>>1) & 0x00C0);

    SPI_ENABLE();   //CSn low                    
    spi(0x80|cmdTempH);
    spi(0x00|cmdTempL);
    for(i=0; i<n; i++)
    {   
        spi(data[i]);
    }  
    SPI_DISABLE();  //CSn high 

    _delay_ms(1);
}



/***********************************************************************************************
* 
************************************************************************************************/
unsigned char ReadRAM(unsigned int cmd)
{
    unsigned char cmdTempH,cmdTempL;
    unsigned char Value;
    cmdTempH = (0x007F & cmd);
    cmdTempL = ((cmd>>1) & 0x00C0); 

    spi(0x80|cmdTempH);
    Value = spi(0x20|cmdTempL);

    _delay_ms(1);
    return Value;
}


/***********************************************************************************************
* 
************************************************************************************************/
void PHY_Init(void)
{
    PORT_Init();
    SPI_Init();
}
    


    
