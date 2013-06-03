#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <util/delay.h>

#include "nic.h"
#include "enc624J600.h"
#include "dev.h"

#define PACKET_HEADER_SIZE	8
#define ARP_PACKET_SIZE	42
#define MAC_ADDR_SIZE	6
#define IP_ADDR_SIZE	4


// DATA 
volatile uint8_t ENC624J600Bank;
volatile uint16_t NextPacketPtr,PacketPtr;
volatile packet_t IPPacketTab[MAX_PACKET];
volatile unsigned char read_idx = 0, write_idx = 0;




unsigned char arpResponse[ARP_RESPONSE_PACKET_SIZE-6] = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//	ENC624J600_MAC0, ENC624J600_MAC1, ENC624J600_MAC2, ENC624J600_MAC3, ENC624J600_MAC4, ENC624J600_MAC5,
	0x08, 0x06,
	0x00, 0x01,
	0x08, 0x00,
	0x06,
	0x04,
	0x00, 0x02,
	ENC624J600_MAC0, ENC624J600_MAC1, ENC624J600_MAC2, ENC624J600_MAC3, ENC624J600_MAC4, ENC624J600_MAC5,
	IP_ADDR_0, IP_ADDR_1, IP_ADDR_2, IP_ADDR_3,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};



unsigned short ip_checksum(unsigned short length, unsigned short *buff)
{
	unsigned short word16;
	unsigned long sum=0;
	int i;

	for(i=0; i<length; i=i+2)
	{
		word16 = ((buff[i]<<8&0xFF00) + (buff[i+1]&0xFF));
		sum += word16;
	}

	while(sum>>16)
		sum = (sum &0xFFFF) + (sum>>16);

	sum = ~sum;

	return ((unsigned short)sum);
}

unsigned short temp1[] = { 
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0x08, 0x06,
		0x00, 0x01,
		0x08, 0x00,
		0x06, 0x04,
		0x00, 0x01,
		IP_ADDR_0,IP_ADDR_1, IP_ADDR_2,IP_ADDR_3};


/**
 * Single Byte Instructions
 **/
void ENC624J600SBI(uint8_t instruction)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue the instruction
	SPDR = instruction;
	while(!(SPSR & (1<<SPIF)));

	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}




//Maj 23/05/12
u08 ENC624J600ReadOp(u08 op, u08 address)
{
	u08 data;
   
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);
	
	// issue read command
	SPDR = op | address;
	while(!(SPSR & (1<<SPIF)));
	// read data
	SPDR = 0x00;
	while(!(SPSR & (1<<SPIF)));
	// do dummy read if needed
	if(address & 0x80)
	{
		SPDR = 0x00;
		while(!(inb(SPSR) & (1<<SPIF)));
	}
	data = SPDR;
	
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);

	return data;
}

//MAJ 23/05/12
void ENC624J600WriteOp(u08 op, u08 address, u08 data)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue write command
	SPDR = op | address;
	while(!(SPSR & (1<<SPIF)));
	// write data
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));

	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}

//MAJ 29/05/12
void ENC624J600WriteOp16(u08 op, u16 data)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue write command
	SPDR = op;
	while(!(SPSR & (1<<SPIF)));

	SPDR = data&0x00FF;
	while(!(SPSR & (1<<SPIF)));
	
	SPDR = data>>8;
	while(!(SPSR & (1<<SPIF)));
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}

void ENC624J600WritePTR(uint8_t instruction, uint16_t address, uint8_t disable_cs)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);
	
	SPDR = instruction;
	while(!(SPSR & (1<<SPIF)));

	SPDR = address&0x00FF;
	while(!(SPSR & (1<<SPIF)));

	SPDR = address>>8;
	while(!(SPSR & (1<<SPIF)));

	// release CS
	if(disable_cs)
		ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}


void ENC624J600ReadRXBuffer(uint16_t address, uint8_t *data, uint16_t len)
{
	ENC624J600WritePTR(ENC624J600_WRITE_ERXRDPT, address, 0);

	SPDR = ENC624J600_READ_ERXDATA;
	while(!(SPSR & (1<<SPIF)));

	while(len--)
	{
		SPDR = 0x00;
		while(!(SPSR & (1<<SPIF)));
		*data++ = SPDR;
	}
	                  
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}

void ENC624J600WriteGPBuffer(uint16_t address, uint8_t *data, uint16_t len)
{
	ENC624J600WritePTR(ENC624J600_WRITE_EGPWRPT, address, 0);

	SPDR = ENC624J600_WRITE_EGPDATA;
	while(!(SPSR & (1<<SPIF)));

	while(len--)
	{
		SPDR = *data++;
		while(!(SPSR & (1<<SPIF)));
	}
	                  
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}


//MAJ 06/06/12 : test OK !
void ENC624J600SetBank(u08 address)
{
	if((address & BANK_MASK) != ENC624J600Bank)
	{
		// set the bank
		ENC624J600Bank = (address & BANK_MASK);
		switch((ENC624J600Bank)>>5){
			case 0 :
				ENC624J600SBI(ENC624J600_BANK0_SELECT);
				break;
			case 1 :
				ENC624J600SBI(ENC624J600_BANK1_SELECT);
				break;
			case 2 :
				ENC624J600SBI(ENC624J600_BANK2_SELECT);
				break;
			case 3 :
				ENC624J600SBI(ENC624J600_BANK3_SELECT);
				break;
		}
	}
}

//MAJ 23/05/12
u08 ENC624J600Read(u08 address)
{
	// set the bank
	ENC624J600SetBank(address);
	// do the read
	return ENC624J600ReadOp(ENC624J600_READ_CONTROL_REGISTER, address-ENC624J600Bank);
	
}

//MAJ 23/05/12
void ENC624J600Write(u08 address, u08 data)
{
	// set the bank
	ENC624J600SetBank(address);
	// do the write
	ENC624J600WriteOp(ENC624J600_WRITE_CONTROL_REGISTER, address-ENC624J600Bank, data);
}


/**
 * Three-Byte Instructions
 **/
/*
void ENC624J600TBI(uint8_t instruction, uint8_t *data)
{

}
*/

/**
 * Three-Byte Write Instruction
 **/
void ENC624J600_Write_TBI(uint8_t instruction, uint8_t *data)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue the instruction
	SPDR = instruction;
	while(!(SPSR & (1<<SPIF)));

	SPDR = data[0];
	while(!(SPSR & (1<<SPIF)));

	SPDR = data[1];
	while(!(SPSR & (1<<SPIF)));

	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}

//MAJ 22/05/12
// ACHTUNG Bug potentiel pour data
#if 0
u16 ENC624J600PhyRead(u08 address)
{
	u16 data;

	// Set the right address and start the register read operation
	ENC624J600Write(MIREGADRL, address);
	ENC624J600Write(MICMDL, MICMD_MIIRD);

	// wait until the PHY read completes
	while(ENC624J600Read(MISTATL) & MISTAT_BUSY);

	// quit reading
	ENC624J600Write(MICMDL, 0x00);
	
	// get data value
	data  = ENC624J600Read(MIRDL);
	data |= ENC624J600Read(MIRDH);
	// return the data
	return data;
}
#endif

//MAJ 22/05/12
#if 0
void ENC624J600PhyWrite(u08 address, u16 data)
{
	// set the PHY register address
	ENC624J600Write(MIREGADRL, address);
	
	// write the PHY data
	ENC624J600Write(MIWRL, data);	
	ENC624J600Write(MIWRH, data>>8);

	// wait until the PHY write completes
	while(ENC624J600Read(MISTATL) & MISTAT_BUSY);
}
#endif

//MAJ 31/05/12
void ENC624J600Init(void)
{
	ENC624J600Bank=3;
	//packet_transmit=0;
	
	// initialize I/O
	sbi(ENC624J600_CONTROL_DDR, ENC624J600_CONTROL_CS);
	sbi(ENC624J600_CONTROL_PORT, ENC624J600_CONTROL_CS);
	
	// setup SPI I/O pin
	sbi(ENC624J600_SPI_PORT, ENC624J600_SPI_SCK);	// set SCK hi
	sbi(ENC624J600_SPI_DDR, ENC624J600_SPI_SCK);	// set SCK as output
	cbi(ENC624J600_SPI_DDR, ENC624J600_SPI_MISO);	// set MISO as input
	sbi(ENC624J600_SPI_DDR, ENC624J600_SPI_MOSI);	// set MOSI as output
	sbi(ENC624J600_SPI_DDR, ENC624J600_SPI_SS);	// SS must be output for Master mode to work
	// initialize SPI interface
	// master mode
	sbi(SPCR, MSTR);
	// select clock phase positive-going in middle of data
	cbi(SPCR, CPOL);
	// Data order MSB first
	cbi(SPCR,DORD);
	// switch to f/4 2X = f/2 bitrate
	cbi(SPCR, SPR0);
	cbi(SPCR, SPR1);
	sbi(SPSR, SPI2X);
	// enable SPI
	sbi(SPCR, SPE);
	
	//8.1 RESET
	//STEP ONE
	ENC624J600Write(EUDASTL,0x34);
	ENC624J600Write(EUDASTH,0x12);
	
	//STEP TWO
	while(ENC624J600Read(EUDASTL)!=0x34 || ENC624J600Read(EUDASTH)!=0x12)
	{
		ENC624J600Write(EUDASTL,0x34);
		ENC624J600Write(EUDASTH,0x12);
	}

	//STEP THREE
	while(ENC624J600Read(ESTATH) & ESTAT_CLKRDY);

	//STEP FOUR
	// reset command
	ENC624J600SBI(ENC624J600_ETH_RESET);

	//STEP FIVE
	_delay_us(25);
		
	//STEP SIX
	if (ENC624J600Read(EUDASTL)==0x00 && ENC624J600Read(EUDASTH)==0x00)
	{
		_delay_us(260);		
		//8.2 CLKOUT Frequency
		// Arduino : 16MHz =>  COCON=0100 
		// We do not use the clkout
		//ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON2H,ECON2_COCON2>>8);
		//8.3 reception
		NextPacketPtr = RXSTART_INIT;
		ENC624J600Write(ERXSTL, RXSTART_INIT&0x00FF);
		ENC624J600Write(ERXSTH, RXSTART_INIT>>8);
		ENC624J600Write(ERXTAILL, RXSTOP_INIT&0x00FF);
		ENC624J600Write(ERXTAILH, RXSTOP_INIT>>8);
			
 		// USER buffer
 		ENC624J600Write(EUDASTL, USER_START_INIT&0x00FF);
 		ENC624J600Write(EUDASTH, USER_START_INIT>>8);
 		ENC624J600Write(EUDANDL, USER_STOP_INIT&0x00FF);
 		ENC624J600Write(EUDANDH, USER_STOP_INIT>>8);
			
		//8.4 RAF

		//8.5 RECEIVE FILTER TODO!!!
		// crc ERROR FILTER => disabled
		// frames shorter than 64 bits => disabled
		// CRC error rejection => enabled
		// Unicast collection filter => enabled
		// Not me unicast filter => disabled
		// Multicast collection filter => ne pas activer le multicast
		ENC624J600Write(ERXFCONL,ERXFCON_CRCEN|ERXFCON_RUNTEN|ERXFCON_UCEN);
		// brodcast collection filter => enabled
		// Hash table collection filter.. compris mais je sais pas si c'est disabled ou enabled
		// Magic packet => disabled TODO
		// PAttern
			
		//window 
		ENC624J600Write(EPMOL,0x00);
		ENC624J600Write(EPMOH,0x00);
			
		//pattern
		ENC624J600Write(EPMM1L, 0x3F);
		ENC624J600Write(EPMM1H, 0xF0);
		ENC624J600Write(EPMM2L, 0x3F);
		ENC624J600Write(EPMM2H, 0x00);
		ENC624J600Write(EPMM3L, 0xC0);
		ENC624J600Write(EPMM3H, 0x03);
		ENC624J600Write(EPMM4L, 0x00);
		ENC624J600Write(EPMM4H, 0x00);
		//CheckSum
		unsigned short check = ip_checksum(20, temp1);
		ENC624J600Write(EPMCSL,(check&0x00FF));
		ENC624J600Write(EPMCSH,(check>>8));
		//exact pattern
		ENC624J600Write(ERXFCONH,0x01);
					      
		// 8.6 MAC initialization ...
		//flow control ???
		ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, MACON2L, MACON2_TXCRCEN|MACON2_PADCFG0|MACON2_PADCFG1|MACON2_HFRMEN);
		ENC624J600Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
		ENC624J600Write(MAMXFLH, MAX_FRAMELEN>>8);
			
		ENC624J600Write(MAADR1L, ENC624J600_MAC0);
		ENC624J600Write(MAADR1H, ENC624J600_MAC1);
		ENC624J600Write(MAADR2L, ENC624J600_MAC2);
		ENC624J600Write(MAADR2H, ENC624J600_MAC3);
		ENC624J600Write(MAADR3L, ENC624J600_MAC4);
		ENC624J600Write(MAADR3H, ENC624J600_MAC5);
		//ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON2H, ECON2_TXMAC>>8);
		ENC624J600Write(ECON2H, 0xa0);
		//8.7 PHY initialization 
		// auto-negotiation ?
		//ENC624J600PhyWrite(PHANA,0x05E1);
		// 8.8 OTHER considerations
		//half-duplex mode
		//ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, MACON2H,MACON2_DEFER|MACON2_BPEN|MACON2_NOBKOFF);$
			
		// enable interuption
		ENC624J600Write(EIEL,0x40);
//		ENC624J600Write(EIEL,0x48);
		ENC624J600Write(EIEH,0x80);
		// configuration LED
	//	ENC624J600Write(EIDLEDH,0x54);
	// 	ENC624J600PhyWrite(PHCON1,PHCON1_PFULDPX);
		// enable reception
		ENC624J600SBI(ENC624J600_ENABLE_RX);
	}
	else
	{
		#ifdef DEBUG
		displayString("-------initialization failed-----\n");
		#endif
	}
	write_idx=0;
	read_idx=0;
}

//MAJ 24_05_12
void ENC624J600PacketSend(uint16_t len, unsigned char* packet)
{
	while(ENC624J600Read(ECON1L) & ECON1_TXRTS);
	// copy the packet into the transmit buffer
	ENC624J600WriteGPBuffer(TXSTART_INIT, packet, len);
	
	// Set the write pointer to start of transmit buffer area
	ENC624J600Write(ETXSTL, TXSTART_INIT&0x00FF);
	ENC624J600Write(ETXSTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	ENC624J600Write(ETXLENL, (len&0x00FF));
	ENC624J600Write(ETXLENH, (len>>8));

	// send the contents of the transmit buffer onto the network
	ENC624J600SBI(ENC624J600_SETTXRTS);
}

//maj 05_05_12
void ENC624J600WriteUser(u16 address,u08 data)
{
	ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,address);
	ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,data);
}

//MAJ 06_06_12
#if 0
void ENC624J600DMABuffer(u16 len,u16 source,u16 dest)
{
	if ( (ENC624J600Read(ECON1L) & ECON1_DMAST )==0x00)
	{
		ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_DMACPY);
		ENC624J600Write(EDMASTL,(source)&0xFF);
		ENC624J600Write(EDMASTH,(source)>>8);
		ENC624J600Write(EDMADSTL,(dest)&0xFF);
		ENC624J600Write(EDMADSTH,(dest)>>8);
		ENC624J600Write(EDMALENL,(len)&0xFF);
		ENC624J600Write(EDMALENH,(len)>>8);

		ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_DMAST);
		while((ENC624J600Read(ECON1L) & ECON1_DMAST )!=0x00);
	}
}
#endif


//MAJ 05_05_12
uint16_t ENC624J600PacketReceive(void)
{
	uint8_t i;
	volatile uint16_t length;
	volatile unsigned char type[2];
	volatile unsigned char head[8];
	volatile uint8_t byte[4];

	// check if a packet has been received and buffered
	if( !(ENC624J600Read(EIRL) & EIR_PKTIF) )
		return 0;

	PacketPtr=NextPacketPtr;
	ENC624J600ReadRXBuffer(PacketPtr,head,PACKET_HEADER_SIZE);
	NextPacketPtr=head[1]<<8;
	NextPacketPtr|=head[0];
	length=head[3]<<8;
	length|=head[2];
	ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+12,type,2);
	/*-----------------------test si c'est un ARP-------------------------*/
	if (type[0]==0x08 && type[1]==0x06)
	{
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+6,arpResponse,MAC_ADDR_SIZE);
		strncpy(arpResponse+26, arpResponse, MAC_ADDR_SIZE);
	
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+28,arpResponse + 32, IP_ADDR_SIZE);
		ENC624J600PacketSend(ARP_PACKET_SIZE-6, arpResponse); 

		// freed up memory by updating ERXTAIL if needed
		uint8_t indice;
		if (write_idx==0)
			indice=MAX_PACKET-1;
		else
			indice=write_idx-1;

		if (IPPacketTab[indice].size!=0){
			IPPacketTab[indice].nextPtr=NextPacketPtr;
		}
		else{
			ENC624J600Write(ERXTAILL, (NextPacketPtr-2)&0x00FF);
			ENC624J600Write(ERXTAILH, (NextPacketPtr-2)>>8);
		}
	}
	else 
	{
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+23,byte,1);
		/*---------------------test si c'est un ping----------------------*/
		if (type[0]==0x08 && type[1]==0x00 && byte[0] == 0x01)
		{
			/*------------------------traitement-------------------------*/
			ENC624J600WriteUser(PacketPtr+7+35,0x00);
	
			ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+26,byte,4);
			ENC624J600WriteGPBuffer(PacketPtr+8+30, byte, 4);
			
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+26,IP_ADDR_0);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+27,IP_ADDR_1);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+28,IP_ADDR_2);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+29,IP_ADDR_3);
			/*------------------------ENVOIE-------------------------*/
			
			while(ENC624J600Read(ECON1L) & ECON1_TXRTS);
			// Set the write pointer to start of the packet
			ENC624J600Write(ETXSTL,(PacketPtr+PACKET_HEADER_SIZE+6)&0x00FF);
			ENC624J600Write(ETXSTH,(PacketPtr+PACKET_HEADER_SIZE+6)>>8);
			// Set the TXND pointer to correspond to the packet size given
			ENC624J600Write(ETXLENL, (length-10)&0x00FF);
			ENC624J600Write(ETXLENH, (length-10)>>8);
			// send the contents of the transmit buffer onto the network
			ENC624J600SBI(ENC624J600_SETTXRTS);

			// freed up memory by updating ERXTAIL if needed !
			u08 indice;
			if (write_idx==0)
				indice=MAX_PACKET-1;
			else
				indice=write_idx-1;

			if (IPPacketTab[indice].size!=0){
				IPPacketTab[indice].nextPtr=NextPacketPtr;
			}
			else{
				ENC624J600Write(ERXTAILL, (NextPacketPtr-2)&0x00FF);
				ENC624J600Write(ERXTAILH, (NextPacketPtr-2)>>8);
			}

		}
		else{
			#ifdef DEBUG
			displayString("TCP ou UDP\n");
			#endif
			u08 port[2];
			ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+36,port,2);
			if (port[0]==0x00 && port[1]==0x50)
			{
				IPPacketTab[write_idx].packetPtr = PacketPtr;
				IPPacketTab[write_idx].nextPtr = NextPacketPtr;
				IPPacketTab[write_idx].size = length-18; // 6 for Mac Src, 6 for Mac Dst, 2 for ethernet type, 4 for ethernet CRC
				ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+6,IPPacketTab[write_idx].mac_src,6);
				
				if(write_idx < (MAX_PACKET-1))
					write_idx++;
				else
					write_idx=0;
			}else{
				length=0;
			}
		}
	}
	 /*	
	// freed up memory by updating ERXTAIL
	ENC624J600Write(ERXTAILL, (NextPacketPtr-2)&0x00FF);
	ENC624J600Write(ERXTAILH, (NextPacketPtr-2)>>8);
	*/
	// decrement the packet counter indicate we are done with this packet
	ENC624J600SBI(ENC624J600_SETPKTDEC);
	
	return length;
}

