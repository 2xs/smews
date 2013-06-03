#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "dev.h"
#include "enc624J600.h"
#include "nic.h"

//#define TIME	16000


extern volatile packet_t IPPacketTab[];
extern volatile uint8_t read_idx, write_idx ;
extern volatile uint8_t my_mac[] ;
static volatile uint16_t packet_size; 
static volatile uint8_t ipProto[] = {0x08, 0x00};
//extern volatile u08 packet_transmit;



void dev_init(void) 
{
	nicInit(); 

	DDRB |= _BV(0);
	PORTB &= ~(_BV(0));

	// Interruption management
	EICRA = 0x00;	
	EIMSK = 0x01;

	sei();
}


void dev_prepare_output(uint16_t size)
{
	cli();
	while(ENC624J600Read(ECON1L)&ECON1_TXRTS);
//	_delay_us(500);
//	_delay_ms(1);
	//packet_size = size + 14;
	packet_size = size + 8;
	if (read_idx!=0)
		ENC624J600WriteGPBuffer(TXSTART_INIT+44, IPPacketTab[read_idx-1].mac_src, 6); 
	else
		ENC624J600WriteGPBuffer(TXSTART_INIT+44, IPPacketTab[MAX_PACKET-1].mac_src, 6); 

	ENC624J600WriteGPBuffer(TXSTART_INIT+6+44,ipProto, 2);
	//ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,TXSTART_INIT+6+8);
	////ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,TXSTART_INIT+8+44);
}


void dev_put(unsigned char byte)
{
	ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,byte);
}





void dev_output_done(void)
{
	ENC624J600Write(ETXSTL, ((TXSTART_INIT+44)&0x00FF));
	ENC624J600Write(ETXSTH, (TXSTART_INIT+44)>>8);
	
	ENC624J600Write(ETXLENL, (packet_size&0x00FF));
	ENC624J600Write(ETXLENH, (packet_size>>8));

	// send the contents of the transmit buffer onto the network
	//ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_TXRTS);
	ENC624J600SBI(ENC624J600_SETTXRTS);
	while(ENC624J600Read(ECON1L)&ECON1_TXRTS);
//	while(!packet_transmit);
//	_delay_ms(1);
#if 0
	unsigned int i;
	for(i=0; i<TIME&!packet_transmit; i++);
	if(i!=TIME)
		PORTB |= _BV(0);
#endif
	sei();
}


char dev_get(void)
{
	volatile uint8_t byte=0;
	static unsigned char first=1;

	if(IPPacketTab[read_idx].size == 0)
		return -1;
	if(first && IPPacketTab[read_idx].size>0)
	{
		ENC624J600WriteOp16(ENC624J600_WRITE_ERXRDPT,IPPacketTab[read_idx].packetPtr+7+15);
		first = 0;
	}
	if(IPPacketTab[read_idx].size > 0)
	{
		byte=ENC624J600ReadOp(ENC624J600_READ_ERXDATA,0);
		IPPacketTab[read_idx].size--;
	}

	if(IPPacketTab[read_idx].size == 0)
	{
		ENC624J600WriteOp16(ENC624J600_WRITE_ERXRDPT,IPPacketTab[read_idx].nextPtr);
		first=1;
		if(read_idx < (MAX_PACKET-1))
			read_idx++;
		else
			read_idx=0;

		ENC624J600Write(ERXTAILL, (IPPacketTab[read_idx].nextPtr-2)&0x00FF);
		ENC624J600Write(ERXTAILH, (IPPacketTab[read_idx].nextPtr-2)>>8);
	}
	
	return byte;
}

//check if data is available!
u08 data_available(void){
	if(IPPacketTab[read_idx].size > 0)
		return 1;
	else
		return 0;
}


ISR(INT0_vect, ISR_BLOCK)
{
//	cli();
	// desactiver les interruptions
	ENC624J600Write(EIEL,0x00);
	ENC624J600Write(EIEH,0x00);
	
	if( (ENC624J600Read(EIRL) & EIR_PKTIF) )
		ENC624J600PacketReceive();
/*	
	if (ENC624J600Read(EIRL) & EIR_TXIF) 
	{
		PORTB |= _BV(0);
//		packet_transmit = 1;
	}
*/
	// réactiver les interruptions
	ENC624J600Write(EIEL,0x40);
//	ENC624J600Write(EIEL,0x48);
	ENC624J600Write(EIEH,0x80);
//	sei();
}
