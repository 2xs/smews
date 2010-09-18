#include "compiler.h"
#include "config.h"
#include "cc2420.h"
#include "mcu.h" 
#include "phy.h" 
#include "mac.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <avr/io.h>
#include <avr/interrupt.h> 
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/sleep.h>



/*******************************************************************************************************
 **************************                 from mac.h                        **************************
 *******************************************************************************************************/
//-------------------------------------------------------------------------------------------------------
// The RF settings structure is declared here, since we'll always need halRfInit()
extern volatile CC2420_SETTINGS rfSettings;

// Basic RF transmission and reception structures
extern CC2420_RX_INFO rfRxInfo;
extern CC2420_TX_INFO rfTxInfo;
extern unsigned char pTxBuffer[CC2420_MAX_PAYLOAD_SIZE];
extern unsigned char pRxBuffer[CC2420_MAX_PAYLOAD_SIZE];

extern unsigned char flag_ReceiveComplete;
//-------------------------------------------------------------------------------------------------------

/*******************************************************************************************************
 **************************       Handel interrupts                           **************************
 *******************************************************************************************************/

// FIFOP Pin of MicaZ sensor interfaces with Timer2 of ATmega128L
// So we initialise timer2 counter to maximum (OxFF) in init interrupt
// We set up Timer2 to have an external clock source on  the rising edge
// and set up an over flow interrupt
// we should reinitialise timer counter to 0xFF in the ISR

ISR(TIMER2_OVF_vect)
{ 
   unsigned char tempL,tempH;
   unsigned char length;
   unsigned int frameControlField;
   unsigned char pFooter[2];
   unsigned char i,j=0;
   unsigned int result=0;
    
    //TCNT to 0xFF to enable over flow on the next rising edge
    TCNT2=0xFF; 
    CLEAR_FIFOP_INT();
   
    // Clean up and exit in case of FIFO overflow, which is indicated by FIFOP = 1 and FIFO = 0
	if((FIFOP_IS_1) && (!(FIFO_IS_1))) 
	{	      
	    WriteStrobeReg_spi(CC2420_SFLUSHRX);
	    WriteStrobeReg_spi(CC2420_SFLUSHRX);
	    return;
	}
    
    //check again if there is data in the RXFIFO buffer    	
    if (FIFO_IS_1)
    {   
    
        //wait until transmission complete    
        while(SFD_IS_1); 
   		SPI_ENABLE(); 
   		
		//read frame length 
		i = spi(0x40|CC2420_RXFIFO);
        length = WriteStrobeReg(CC2420_SNOP);
        
        length &= CC2420_LENGTH_MASK; // Ignore MSB : MSB always set to 0 

        // Ignore the packet if the length is too short less than an ack packet
        
		if (length < CC2420_ACK_PACKET_SIZE) // CC2420_ACK_PACKET_SIZE == 5
        {
            //if wrong packet erase data
            for(j=0; j<length; j++) 
            { 
                WriteStrobeReg(CC2420_SNOP);
            }	  
        } 
        else //if the length is valid, then treat the packet
        {            
            // Read the frame control field and the data sequence number
            tempL = WriteStrobeReg(CC2420_SNOP);
            tempH = WriteStrobeReg(CC2420_SNOP);            
            frameControlField = tempH;
            frameControlField = (frameControlField<<8)|tempL;
            rfSettings.pRxInfo->ackRequest = !!(frameControlField & CC2420_FCF_ACK_BM);
            rfSettings.pRxInfo->seqNumber = WriteStrobeReg(CC2420_SNOP);
        
            // test if it is an acknowledgment packet
    	    if ((length == CC2420_ACK_PACKET_SIZE) && (frameControlField == CC2420_FCF_FRAMETYPE_ACK) && (rfSettings.pRxInfo->seqNumber == rfSettings.txSeqNumber)) 
    	    {
 	       	    // Read the footer and check for CRC OK
 	       	    pFooter[0] = WriteStrobeReg(CC2420_SNOP);
                pFooter[1] = WriteStrobeReg(CC2420_SNOP);

                // Indicate the successful ack reception (this flag is polled by the transmission routine)
			    if (pFooter[1] & CC2420_CRC_OK_BM)
			        rfSettings.ackReceived = TRUE;		        
            } 
            
            else if (length < CC2420_PACKET_OVERHEAD_SIZE_DsSs) // Too small to be a valid packet?
            {
                 for(j=0; j<length-3; j++) 
                 { 
                         WriteStrobeReg(CC2420_SNOP);
                 }
		 		return;		    
             } 
		
             else // Receive the rest of the packet
             { 
              // Register the payload length
               rfSettings.pRxInfo->length = length - CC2420_PACKET_OVERHEAD_SIZE_DsSs;
               
              // Skip the destination PAN (that's taken care of by harware address recognition!)
              i = WriteStrobeReg(CC2420_SNOP);
              i = WriteStrobeReg(CC2420_SNOP);

              // Skip the dest address (that's taken care of by harware address recognition!)
                 if( ( frameControlField & CC2420_FCF_DESTADDR_BM ) == CC2420_FCF_DESTADDR_16BIT )
                 {
		         i = WriteStrobeReg(CC2420_SNOP);
			 i = WriteStrobeReg(CC2420_SNOP);
                 }
		             
              // Read source address 
                 if( ( frameControlField & CC2420_FCF_SOURCEADDR_BM ) == CC2420_FCF_SOURCEADDR_16BIT )
                 {
                         tempL = WriteStrobeReg(CC2420_SNOP);
                         tempH = WriteStrobeReg(CC2420_SNOP); 
                         rfSettings.pRxInfo->srcAddr = tempH;
			 			 rfSettings.pRxInfo->srcAddr = (rfSettings.pRxInfo->srcAddr<<8)|tempL;
                 }
                       
              // Read the packet payload
                 for(j=0; j < rfSettings.pRxInfo->length; j++) 
                 { 
		            	rfSettings.pRxInfo->pPayload[j] = WriteStrobeReg(CC2420_SNOP);
		            	if(j > CC2420_MAX)
		                	break;
                 }			    
                
             // Read the footer to get the RSSI value
				pFooter[0] = WriteStrobeReg(CC2420_SNOP);
				pFooter[1] = WriteStrobeReg(CC2420_SNOP);    
				rfSettings.pRxInfo->rssi = pFooter[0];
			    
				flag_ReceiveComplete = 1;
		    }	        	            
        }
        SPI_DISABLE();  
        i=WriteStrobeReg_spi(CC2420_SFLUSHRX);
        i=WriteStrobeReg_spi(CC2420_SFLUSHRX); 
    }
    if(flag_ReceiveComplete)
    {
        //transmission ok : display number sent using Leds
        result = rfSettings.pRxInfo->pPayload[0];
        switch (result)
        {
       
        case 0:
        {
           //000
           RLED_DISABLE();
           YLED_DISABLE();
           GLED_DISABLE();
           _delay_ms(500);
                     
           break; 
        }      
       
        case 1:
        {
           
           YLED_EN(); 
           RLED_DISABLE();
           GLED_DISABLE();
           
           break; 
        }
        
        case 2:
        {
            YLED_DISABLE();
            GLED_EN(); 
            RLED_DISABLE();
                       
            break;
        }
        case 3:
        {
            YLED_EN();
            GLED_EN(); 
            RLED_DISABLE();
            
            
            break;
        } 
        
        case 4:
        {
            YLED_DISABLE();
            GLED_DISABLE();
            RLED_EN();
            
            break;
        }
        
        case 5:
        {
            YLED_EN();
            GLED_DISABLE();
            RLED_EN();
            
            break;
        }
        
        case 6:
        {
            YLED_DISABLE();
            GLED_EN();
            RLED_EN();
            
            break;
        }
        
        case 7:
        {
            YLED_EN();
            GLED_EN();
            RLED_EN();
            
            
            break;
        }
        
        
        default:
            break;     
        }
        
        
        flag_ReceiveComplete = 0;
		//sleep_disable();
    }
	//sleep_disable();

} 

int main(void)
{
    
    int i;
    MAC_Init();
    
	if(NODE_TYPE == RECEIVER_NODE)
	{
		while (1)
    	{
        	//set_sleep_mode(SLEEP_MODE_IDLE);
        	//sleep_mode();     
    	};	
	}
	
	else
	{
		if(NODE_TYPE == SENDER_NODE)
		{
			rfTxInfo.destAddr = 0x1235;
     		rfTxInfo.length = 1;
     		rfTxInfo.ackRequest = FALSE; 
      		i=0;
			
			while (1)
     		{
        		// Led display : 000 <--> RGY
        		if(i==8)   
        		{
         			i= 0;       
        		}
                 
        
        		switch (i)
        		{
       		   		case 0:
        			{
						pTxBuffer[0] = 0x00;
						//000
           				RLED_DISABLE();
           				YLED_DISABLE();
           				GLED_DISABLE();
           				_delay_ms(500);
          				break; 
        			}      
       
        			case 1:
        			{
           				pTxBuffer[0] = 0x01;
           				YLED_EN(); 
           				RLED_DISABLE();
           				GLED_DISABLE();
           			    break;
					}
        
        			case 2:
        			{
            			pTxBuffer[0] = 0x02;                          
                        YLED_DISABLE();
            			GLED_EN(); 
            			RLED_DISABLE();
                        break;
        			}
        			
					case 3:
        			{
            			pTxBuffer[0] = 0x03; 
           				YLED_EN();
            			GLED_EN(); 
            			RLED_DISABLE();
            			break;
					} 
        
        			case 4:
        			{
            			pTxBuffer[0] = 0x04;
            			YLED_DISABLE();
            			GLED_DISABLE();
            			RLED_EN();
                        break;
					}
        
        			case 5:
					{
            			pTxBuffer[0] = 0x05;
           				YLED_EN();
            			GLED_DISABLE();
            			RLED_EN();
            			break;
					}
        
        			case 6:
        			{
            			pTxBuffer[0] = 0x06;
           				YLED_DISABLE();
            			GLED_EN();
            			RLED_EN();
            			break;
					}
        
        			case 7:
        			{
            			pTxBuffer[0] = 0x07;
           				YLED_EN();
            			GLED_EN();
            			RLED_EN();
            			break;
        			}
        
        			default:
            			break;     
        		 }
        
        i++;
        
        CC2420_SendPacket(&rfTxInfo); 
        _delay_ms(5000);
             
     };

				
		}

	}

	
}




