#include <avr/io.h>
#include <avr/interrupt.h>


#include "dev.h"
#include "enc624J600.h"
#include "ethernet.h"
#include "connections.h"
#include "link_layer_cache.h"

#include "serial.h"
#include <stdio.h>
#include "debugMemory.h"

extern volatile packet_t IPPacketTab[];
extern volatile uint8_t read_idx;

static uint16_t packet_size; 
#ifdef IPV6
static uint8_t ipProto[] = {0x86, 0xdd};
#else
static uint8_t ipProto[] = {0x08, 0x00};
#endif

#define MAX_TAMPON 64
static unsigned char tampon[MAX_TAMPON];
static unsigned char indice_write = 0;

static unsigned char tampon_read[MAX_TAMPON];
static unsigned char indice_read = 0;


static uint16_t choix_TX;


void dev_init(void) 
{
	choix_TX=TXSTART_INIT;
	ENC624J600Init(); 

	// Interruption management
	EICRA = 0x00;	
	//EIMSK = 0x01; // INT0
	EIMSK = 0x03; // INT0 and INT1

	sei();
}


void dev_prepare_output(uint16_t size)
{
	packet_size = size + 8; // 6 for MAC address destination + 2 for protocol

	ENC624J600WriteGPBuffer(choix_TX+6, ipProto, 2); // +6 because we add the MAC destination in dev_output_done
	indice_write=0;
}


void dev_put(unsigned char byte)
{
	tampon[indice_write++]=byte;
	
	if(indice_write == MAX_TAMPON)
	{
		ENC624J600WriteGPBuffer(0xFFFF,tampon,MAX_TAMPON);
		indice_write=0;
	}
}


void dev_output_done(void)
{
	// test if the buffer is empty
	if (indice_write!=0){
		ENC624J600WriteGPBuffer(0xFFFF,tampon,indice_write);
	}

	// Add the mac dst
#ifdef IPV6
	unsigned char dst_ip[16];
#else
	unsigned char dst_ip[4];
#endif
	ethAddr_t dst_addr;
	get_current_remote_ip(dst_ip);
	if(!get_link_layer_address(dst_ip, dst_addr.addr))
	{
		// Oups, the mac address is not available 
		sprintf(stringBuffer, "oups unknown mac :%x:%x:%x:%x\n",dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
		displayString(stringBuffer);
		uint8_t i;
		for(i=0; i<4; i++)
			dst_addr.addr[i] = dst_ip[i];

		dst_addr.addr[4] = dst_ip[0];
		dst_addr.addr[5] = dst_ip[1];
	}
	ENC624J600WriteGPBuffer(choix_TX, dst_addr.addr, 6); 

	ENC624J600Send(choix_TX, packet_size);
	if(choix_TX==TXSTART_INIT)
		choix_TX=TXSTART_2;
	else
		choix_TX=TXSTART_INIT;
}


int16_t dev_get(void)
{
	uint8_t byte=0;
	static unsigned char first=1;
	
	if(IPPacketTab[read_idx].size == 0)
		return -1;
	
	if(first)
	{
	 	ENC624J600ReadRXBuffer(IPPacketTab[read_idx].packetPtr+8+14,tampon_read,MIN(MAX_TAMPON,IPPacketTab[read_idx].size)); // 6 for mac src + 6 for mac dst + 2 for protocol + 8 for header

		indice_read=0;
		first = 0;
	}
	
	if(IPPacketTab[read_idx].size > 0)
	{	
		if(indice_read==MAX_TAMPON)
		{
			ENC624J600ReadRXBuffer(0xFFFF,tampon_read,MIN(MAX_TAMPON,IPPacketTab[read_idx].size));
			indice_read=0;
		}
		byte=tampon_read[indice_read++];
		IPPacketTab[read_idx].size--;
	}

	if(IPPacketTab[read_idx].size == 0)
	{
		first=1;
		ENC624J600Write(ERXTAILL, (IPPacketTab[read_idx].nextPtr-2)&0x00FF);
		ENC624J600Write(ERXTAILH, (IPPacketTab[read_idx].nextPtr-2)>>8);
		if(read_idx < (MAX_PACKET-1))
			read_idx++;
		else
			read_idx=0;
	}
	return byte;
}

//check if data is available!
uint8_t data_available(void)
{
	return (IPPacketTab[read_idx].size > 0);
}


ISR(INT0_vect, ISR_BLOCK)
{
	cli();
	// desactived 
	ENC624J600SBI(ENC624J600_DISABLE_INTERRUPTS);

	// treatment of the received packet
	ENC624J600PacketReceive();

	// réactiver les interruptions
	ENC624J600SBI(ENC624J600_ENABLE_INTERRUPTS);
	sei();
}

// External interruption for debug
// pushing the button will dump the memory (stack + bss) 
// via the serial port
ISR(INT1_vect, ISR_BLOCK)
{
	cli();
	dump_stack();
	sei();
}

