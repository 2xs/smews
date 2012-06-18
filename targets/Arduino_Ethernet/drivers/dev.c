#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "dev.h"
#include "enc624J600.h"
#include "nic.h"

#define TIME	16000


extern volatile packet_t IPPacketTab[MAX_PACKET];
extern volatile u08 read_idx, write_idx ;
extern volatile u08 my_mac[] ;
extern volatile unsigned int packet_size; 
extern volatile u08 packet_transmit;



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


void dev_prepare_output(int size){
		//PORTB |= _BV(0);
	packet_transmit = 0;
	//_delay_us(1);
	packet_size = size + 14;
	if (read_idx!=0)
		ENC624J600WriteBuffer(6, IPPacketTab[read_idx-1].mac_src,TXSTART_INIT); 
	else
		ENC624J600WriteBuffer(6, IPPacketTab[MAX_PACKET-1].mac_src,TXSTART_INIT); 

	ENC624J600WriteBuffer(8, my_mac,TXSTART_INIT+6);
	ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,TXSTART_INIT+6+8);
}


void dev_put(unsigned char byte)
{
	ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,byte);
}





void dev_output_done(void)
{
	ENC624J600Write(ETXSTL, (TXSTART_INIT)&0x00FF);
	ENC624J600Write(ETXSTH, TXSTART_INIT>>8);
	
	ENC624J600Write(ETXLENL, (TXSTART_INIT+packet_size)&0x00FF);
	ENC624J600Write(ETXLENH, (TXSTART_INIT+packet_size)>>8);

	// send the contents of the transmit buffer onto the network
	ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_TXRTS);
//	while(!packet_transmit);
//	_delay_ms(1);
#if 0
	unsigned int i;
	for(i=0; i<TIME&!packet_transmit; i++);
	if(i!=TIME)
		PORTB |= _BV(0);
#endif
}


char dev_get(void)
{
	volatile u08 byte=0;
	static unsigned char first=1;
	// sprintf(chaine,"read_idx=%02X size : %04X \n",read_idx,IPPacketTab[read_idx].size);
	// displayString(chaine);
	if(IPPacketTab[read_idx].size == 0)
		return -1;
	if(first && IPPacketTab[read_idx].size>0)
	{
		// displayString("je traite le new paquet\n");
		// displayString("recoit :  \t\t");
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
	ENC624J600Write(EIEH,0x00);
	ENC624J600Write(EIEL,0x00);
	
	if( (ENC624J600Read(EIRL) & EIR_PKTIF) )
	// retrieve the packet
		nicPoll(1500);
	else if (ENC624J600Read(EIRL) & EIR_TXIF) 
	{
		PORTB |= _BV(0);
		packet_transmit = 1;
	}

	// réactiver les interruptions
//	ENC624J600Write(EIEH,0x80);ENC624J600Write(EIEL,0x40);
	ENC624J600Write(EIEH,0x80);ENC624J600Write(EIEL,0x48);
//	sei();
}
