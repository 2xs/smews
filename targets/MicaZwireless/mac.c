#include <stdlib.h> 
#include <stdio.h> 
#include <avr/io.h>
#include <avr/interrupt.h> 
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <string.h> 
#include "cc2420.h"
#include "mac.h" 
#include "phy.h" 
#include "compiler.h"
#include "mcu.h"
#include "config.h"




// The RF settings structure is declared here // could be referenced in other .c files by "extern"
volatile CC2420_SETTINGS rfSettings;

// Basic RF transmission and reception structures
CC2420_RX_INFO rfRxInfo;

CC2420_TX_INFO rfTxInfo;

unsigned char pTxBuffer[CC2420_MAX_PAYLOAD_SIZE];

unsigned char pRxBuffer[CC2420_MAX_PAYLOAD_SIZE];

unsigned char flag_ReceiveComplete;




/***********************************************************************************************
* set channel by controlling freq register : datasheet formula
************************************************************************************************/
void CC2420_SetChannel(unsigned char channel_temp)
{
    WriteConfigReg_spi(CC2420_FSCTRL,(0x4165 | (357+5*(unsigned int)(channel_temp-11))));
}


/***********************************************************************************************
* Pan id in RAM
************************************************************************************************/
void CC2420_SetPanId(unsigned int PanId_temp)
{ 
    unsigned char PanIdtemp[2];
    
    PanIdtemp[1] = (PanId_temp >> 8);
    PanIdtemp[0] = (PanId_temp & 0x00FF);
    
    WriteRAM_spi(CC2420_RAM_PANID,PanIdtemp,2); 
}


/***********************************************************************************************
* Set ShortAddress in Ram
************************************************************************************************/
void CC2420_SetShortAddress(unsigned int ShortAddress)
{
    unsigned char ShortAddresstemp[2];
    
    ShortAddresstemp[1] = (ShortAddress >> 8);
    ShortAddresstemp[0] = (ShortAddress & 0x00FF);
    
    WriteRAM_spi(CC2420_RAM_SHORTADDR,ShortAddresstemp,2); 
}

/***********************************************************************************************
* enable receive mode (command address) // Rxfifo should be flushed to avoid conflit
************************************************************************************************/
void CC2420_ReceiveOn(void)
{   
    unsigned char i;
    
    rfSettings.receiveOn = TRUE;
    
    i=WriteStrobeReg_spi(CC2420_SRXON);
    i=WriteStrobeReg_spi(CC2420_SFLUSHRX); 
    i=WriteStrobeReg_spi(CC2420_SFLUSHRX); 
    FIFOP_INT_INIT();
    ENABLE_FIFOP_INT();
}
 
/***********************************************************************************************
* disable receive mode 
************************************************************************************************/
void CC2420_ReceiveOff(void)
{
    unsigned char i; 
    
    rfSettings.receiveOn = FALSE; 
    i=WriteStrobeReg_spi(CC2420_SRFOFF);
    DISABLE_FIFOP_INT();
}



/***********************************************************************************************
* CC2420 init
************************************************************************************************/
void CC2420_Init() 
{
    unsigned char i;

    //voltage regulator is on & reset pin is inactive
    SET_VREG_ACTIVE();
    _delay_ms(10);
    SET_RESET_ACTIVE();
    _delay_ms(50);
    SET_RESET_INACTIVE();
    _delay_ms(10);
    
    //Turn off all interrupts during CC2420 registers accessing
    DISABLE_GLOBAL_INT();

    // Register modifications
    i = WriteStrobeReg_spi(CC2420_SXOSCON);
    i = WriteConfigReg_spi(CC2420_MDMCTRL0, 0x0AE2);    // Turn off automatic packet acknowledgment
    i = WriteConfigReg_spi(CC2420_MDMCTRL1, 0x0500);    // Set the correlation threshold = 20
    i = WriteConfigReg_spi(CC2420_IOCFG0, 0x007F);      // Set the FIFOP threshold to maximum
    i = WriteConfigReg_spi(CC2420_SECCTRL0, 0x01C4);    // Turn off "Security enable"

    //Wait for the crystal oscillator to become stable
    do
    {
        i = WriteStrobeReg_spi(CC2420_SXOSCON);    //sixth bit to one
        _delay_ms(100);   
    }
    while((i&0x40)==0);
}
 



/***********************************************************************************************
* CC2420 send data  : address format : src : 16 bits, dest : 16 bits, intra panid
* We use only short addresses for sending&receiving data, other @ format combinations
* can be added as a case switch and new parameter  
************************************************************************************************/
unsigned char CC2420_SendPacket(CC2420_TX_INFO *pRTI)
{
    unsigned int frameControlField;
    unsigned char packetLength;
    unsigned char success;
    unsigned char SendDataTemp[CC2420_MAX]={' '};
    unsigned char i;

    // Wait until the transceiver is idle
    while (FIFOP_IS_1 || SFD_IS_1);

    // Turn off global interrupts to avoid interference on the SPI interface
    DISABLE_GLOBAL_INT();

    // Flush the TX FIFO 
    WriteStrobeReg_spi(CC2420_SFLUSHTX);   
   
    // Write the packet to the TX FIFO (the FCS is appended automatically when AUTOCRC is enabled)    
    
    packetLength = pRTI->length + CC2420_PACKET_OVERHEAD_SIZE_DsSs;
    frameControlField = CC2420_FCF_FRAMETYPE_DATA;
    frameControlField |= pRTI->ackRequest ? CC2420_FCF_ACK_REQUEST : CC2420_FCF_NO_ACK_REQUEST;
    frameControlField |= CC2420_FCF_INTRAPAN;
    frameControlField |= CC2420_FCF_DESTADDR_16BIT;
    frameControlField |= CC2420_FCF_SOURCEADDR_16BIT;
            
    SendDataTemp[0] = packetLength;                 // Packet length
    SendDataTemp[1] = frameControlField & 0x00FF;   // Frame control field
    SendDataTemp[2] = frameControlField >> 8;
    SendDataTemp[3] = rfSettings.txSeqNumber;       // Sequence number
    SendDataTemp[4] = rfSettings.panId & 0x00FF; 
    SendDataTemp[5] = rfSettings.panId >> 8;        // Dest. PAN ID
    SendDataTemp[6] = pRTI->destAddr & 0x00FF;      // Dest. address 
    SendDataTemp[7] = pRTI->destAddr >> 8;           
    SendDataTemp[8] = rfSettings.myAddr & 0x00FF;   // Source address
    SendDataTemp[9] = rfSettings.myAddr >> 8; 
    
    for(i=0;i<pRTI->length;i++)                     // Payload
    {
        SendDataTemp[i+10] = *pRTI->pPayload++; 
    }                       
    
    Write_TXFIFO( SendDataTemp, packetLength+1 );
            
           
    SPI_ENABLE();   
    
    WriteStrobeReg(CC2420_STXON);

    while (!SFD_IS_1);// wait SFD to be fired
    while (SFD_IS_1); // wait transmission to be accopmlished
	
    SPI_DISABLE();   
	    
    // Turn interrupts back on
    ENABLE_GLOBAL_INT();
    success = TRUE;
	
	// if an acknowledgment has been asked : treatment should be done here

    // Increment the sequence number, and return the result
    rfSettings.txSeqNumber++;
    
    // Turn on RX mode
    CC2420_ReceiveOn();
    
    pRTI->pPayload = pTxBuffer;

    memset(pRTI->pPayload,' ',CC2420_MAX_PAYLOAD_SIZE);
  
    return success;
} 


                          
void MAC_Init(void)
{
    
    PHY_Init(); //init ports & init spi
    
    _delay_ms(200);
    
    CC2420_Init(); // CC2420 : configure registers : security ack etc
    
    rfTxInfo.pPayload = pTxBuffer;
    rfRxInfo.pPayload = pRxBuffer;
    
    
    // Set the RF channel
    CC2420_SetChannel(CHANNEL);

    // Set the protocol configuration
	// from config.h file
    rfSettings.pRxInfo = &rfRxInfo;
    rfSettings.panId = PANID;
    rfSettings.myAddr = MY_ADDR;
    
          
    rfSettings.txSeqNumber = 0;
    rfSettings.receiveOn = FALSE;
     
    // Write the short address and the PAN ID to the CC2420 RAM (requires that the XOSC is on and stable)
    CC2420_SetPanId(PANID);                 
    CC2420_SetShortAddress(MY_ADDR);
    
       
    // Initialize the FIFOP external interrupt
    FIFOP_INT_INIT();
    ENABLE_FIFOP_INT();
    
    
    ENABLE_GLOBAL_INT();
  	
    // Turn on RX mode
    CC2420_ReceiveOn();
    
    //RLED_EN();  
} 

 

/***********************************************************************
*
************************************************************************/
void mac_pTxBuffer_Clear(void)
{
    memset(pTxBuffer,' ',CC2420_MAX_PAYLOAD_SIZE);
}

/***********************************************************************
* 
************************************************************************/
void mac_pRxBuffer_Clear(void)
{
    memset(pRxBuffer,' ',CC2420_MAX_PAYLOAD_SIZE);
}


