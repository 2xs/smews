#include <stdint.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "enc624J600.h"
#include "dev.h"

#include "link_layer_cache.h"

#define PACKET_HEADER_SIZE	8
#define ARP_PACKET_SIZE	42
#define MAC_ADDR_SIZE	6
/* Ethernet header size with no-vlan */
/* 6 for MAC address destination     */
/* 6 for MAC address source          */
/* 2 for packet type                 */
#define ETH_HEADER_SIZE	14 

#ifndef IPV6
#define IP_ADDR_SIZE	4
/* IP_START = number of bytes before the IP source address */
#define IP_START		12
#else
#define IP_ADDR_SIZE	16
#define IP_START		8
#endif

// DATA 
volatile static uint8_t ENC624J600Bank=0;
volatile static uint16_t NextPacketPtr,PacketPtr;
volatile packet_t IPPacketTab[MAX_PACKET];
volatile unsigned char read_idx;
static volatile unsigned char write_idx;
static sauvegarde_t save;




/**
 * Single Byte Instructions
 **/
void ENC624J600SBI(uint8_t instruction)
{
	cli();
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue the instruction
	SPDR = instruction;
	while(!(SPSR & (1<<SPIF)));

	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
	sei();
}


uint8_t ENC624J600ReadOp(uint8_t op, uint8_t address)
{
	uint8_t data;
   
	cli();
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);
	
	// issue read command
	SPDR = op | address;
	while(!(SPSR & (1<<SPIF)));
	// read data
	SPDR = 0x00;
	while(!(SPSR & (1<<SPIF)));
	// do dummy read if needed
#if 0
	if(address & 0x80)
	{
		SPDR = 0x00;
		while(!(inb(SPSR) & (1<<SPIF)));
	}
#endif
	data = SPDR;
	
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
	sei();
	return data;
}


static void ENC624J600WriteOp(uint8_t op, uint8_t address, uint8_t data)
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
#if 0
void ENC624J600WriteOp16(uint8_t op, uint16_t data)
{
	cli();
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
	sei();
}
#endif
static void ENC624J600WritePTR(uint8_t instruction, uint16_t address, uint8_t disable_cs)
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


void ENC624J600ReadRXBuffer(uint16_t address, volatile uint8_t *data, uint16_t len)
{
	cli();
	if(address != 0xFFFF)
		ENC624J600WritePTR(ENC624J600_WRITE_ERXRDPT, address,CS_ENABLED);
	else
		ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

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
	sei();
}

void ENC624J600WriteGPBuffer(uint16_t address, volatile uint8_t *data, uint16_t len)
{
	cli();
	if(address != 0xFFFF)
		ENC624J600WritePTR(ENC624J600_WRITE_EGPWRPT, address,CS_ENABLED);
	else
		ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	SPDR = ENC624J600_WRITE_EGPDATA;
	while(!(SPSR & (1<<SPIF)));

	while(len--)
	{
		SPDR = *data++;
		while(!(SPSR & (1<<SPIF)));
	}
	                  
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
	sei();
}

static void ENC624J600SetBank(uint8_t address)
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
static uint8_t ENC624J600Read(uint8_t address)
{
	// set the bank
	ENC624J600SetBank(address);
	// do the read
	//return ENC624J600ReadOp(ENC624J600_READ_CONTROL_REGISTER, address-ENC624J600Bank);
	return ENC624J600ReadOp(ENC624J600_READ_CONTROL_REGISTER, address&0x1F);
	
}

void ENC624J600Write(uint8_t address, uint8_t data)
{
	// set the bank
	ENC624J600SetBank(address);
	// do the write
	ENC624J600WriteOp(ENC624J600_WRITE_CONTROL_REGISTER, address&0x1F, data);
}



/**
 * Three-Byte Write Instruction
 **/
static void ENC624J600_Write_TBI(uint8_t instruction, volatile uint8_t *data)
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

static void ENC624J600_READ_TBI(uint8_t instruction,volatile uint8_t *data)
{
	// assert CS
	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);

	// issue the instruction
	SPDR = instruction;
	while(!(SPSR & (1<<SPIF)));
	
	SPDR=0x00;
	while(!(SPSR & (1<<SPIF)));
	data[0]=SPDR;
	
	SPDR=0x00;
	while(!(SPSR & (1<<SPIF)));
	data[1]=SPDR;
	// release CS
	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
}




//MAJ 22/05/12
// ACHTUNG Bug potentiel pour data
#if 0
uint16_t ENC624J600PhyRead(uint8_t address)
{
	uint16_t data;

	// Set the right address and start the register read operation
	ENC624J600Write(MIREGADRL, address);
	ENC624J600Write(MICMDL, MICMD_MIIRD);

	// wait until the PHY read completes
	while(ENC624J600Read(MISTATL) & MISTAT_BUSY);

	// quit reading
	ENC624J600Write(MICMDL, 0x00);
	
	// get data value
	data  = ENC624J600Read(MIRDL)<<8;
	data |= ENC624J600Read(MIRDH);
	// return the data
	return data;
}
#endif

//MAJ 22/05/12
#if 0
void ENC624J600PhyWrite(uint8_t address, uint16_t data)
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
	// initialize I/O
	sbi(ENC624J600_CONTROL_DDR, ENC624J600_CONTROL_CS);
	sbi(ENC624J600_CONTROL_PORT, ENC624J600_CONTROL_CS);
	
	// setup SPI I/O pin
	sbi(ENC624J600_SPI_PORT, ENC624J600_SPI_SCK);	// set SCK hi
	cbi(ENC624J600_SPI_DDR, ENC624J600_SPI_MISO);	// set MISO as input
	sbi(ENC624J600_SPI_DDR, ENC624J600_SPI_SCK);	// set SCK as output
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
			
 		// USER buffer : EUDAST Pointer at a higher memory address relative to the end address.
 		ENC624J600Write(EUDASTL, USER_STOP_INIT&0x00FF);
 		ENC624J600Write(EUDASTH, USER_STOP_INIT>>8);
 		ENC624J600Write(EUDANDL, USER_START_INIT&0x00FF);
 		ENC624J600Write(EUDANDH, USER_START_INIT>>8);
			
#ifndef IPV6
 		// fill user-defined area with arpResponse
		// only for IPv4
 		unsigned char arpResponse[ARP_RESPONSE_PACKET_SIZE-MAC_ADDR_SIZE] = { 
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
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

 		ENC624J600WriteGPBuffer(USER_START_INIT,arpResponse,ARP_PACKET_SIZE-MAC_ADDR_SIZE);
#endif

		//8.4 RAF

		//8.5 RECEIVE FILTER TODO!!!
#ifdef IPV6
		// We need multicast for neighbor sollicitation
		ENC624J600Write(ERXFCONL,ERXFCON_CRCEN|ERXFCON_RUNTEN|ERXFCON_UCEN|ERXFCON_MCEN);
#else
		// crc ERROR FILTER => disabled
		// frames shorter than 64 bits => disabled
		// CRC error rejection => enabled
		// Unicast collection filter => enabled
		// Not me unicast filter => disabled
		// Multicast collection filter 
		ENC624J600Write(ERXFCONL,ERXFCON_CRCEN|ERXFCON_RUNTEN|ERXFCON_UCEN);
		// brodcast collection filter => enabled
		// Hash table collection filter.. c
		// Magic packet => disabled TODO
		// PAttern
#endif
			
#ifdef CHECKSUM_PATTERN
		// Checksum only for IPv4
		//window 
		ENC624J600Write(EPMOL,0x00);
		ENC624J600Write(EPMOH,0x00);
			
		//pattern
		ENC624J600Write(EPMM1L, WINDOW_PATTERN_0);
		ENC624J600Write(EPMM1H, WINDOW_PATTERN_1);
		ENC624J600Write(EPMM2L, WINDOW_PATTERN_2);
		ENC624J600Write(EPMM2H, WINDOW_PATTERN_3);
		ENC624J600Write(EPMM3L, WINDOW_PATTERN_4);
		ENC624J600Write(EPMM3H, WINDOW_PATTERN_5);
		ENC624J600Write(EPMM4L, WINDOW_PATTERN_6);
		ENC624J600Write(EPMM4H, WINDOW_PATTERN_7);
		//CheckSum
		ENC624J600Write(EPMCSL,CHECKSUM_PATTERN&0xFF);
		ENC624J600Write(EPMCSH,CHECKSUM_PATTERN>>8);
#endif
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
		ENC624J600Write(EIEH,0x80);
		// configuration LED
			//	ENC624J600Write(EIDLEDH,0x54);
			// 	ENC624J600PhyWrite(PHCON1,PHCON1_PFULDPX);
		// enable reception
		ENC624J600SBI(ENC624J600_ENABLE_RX);
	}
	else
	{
		// Oups something went wrong
	}
	write_idx=0;
	read_idx=0;
}


void ENC624J600Send(uint16_t address, uint16_t len)
{
	cli();
	// Set the write pointer to start of the packet
	ENC624J600Write(ETXSTL,(address)&0x00FF);
	ENC624J600Write(ETXSTH,(address)>>8);
	// Set the TXND pointer to correspond to the packet size given
	ENC624J600Write(ETXLENL, (len)&0x00FF);
	ENC624J600Write(ETXLENH, (len)>>8);
	// send the contents of the transmit buffer onto the network
	ENC624J600SBI(ENC624J600_SETTXRTS);
	sei();
}

#ifndef IPV6
static void ENC624J600WriteUser(uint16_t address,uint8_t data)
{
	ENC624J600WritePTR(ENC624J600_WRITE_EGPWRPT,address, 1);
	ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,data);
}
#endif

//MAJ 06_06_12
#if 0
void ENC624J600DMABuffer(uint16_t len,uint16_t source,uint16_t dest)
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


static void ENC624J600FreedUpMemory(void)
{
	uint8_t indice;
	if (write_idx==0)
		indice=MAX_PACKET-1;
	else
		indice=write_idx-1;

	if (IPPacketTab[indice].size!=0)
	{
		IPPacketTab[indice].nextPtr=NextPacketPtr;
	}
	else
	{
		ENC624J600Write(ERXTAILL, (NextPacketPtr-2)&0x00FF);
		ENC624J600Write(ERXTAILH, (NextPacketPtr-2)>>8);
	}
}


uint16_t ENC624J600PacketReceive(void)
{
	uint16_t length;
	uint8_t type[2];
	uint8_t head[PACKET_HEADER_SIZE];
	uint8_t mac[MAC_ADDR_SIZE];
	uint8_t ip_src[IP_ADDR_SIZE];
	
	uint8_t temp_i;
	uint8_t reverse_ip[IP_ADDR_SIZE];
	
	// check if a packet has been received and buffered
	if( !(ENC624J600Read(EIRL) & EIR_PKTIF) )
		return 0;
	
	// Save the context, i.e., read and write pointers
	ENC624J600_READ_TBI(ENC624J600_READ_ERXRDPT,save.ERXRDPT);
	ENC624J600_READ_TBI(ENC624J600_READ_EGPWRPT,save.EGPWRPT);

	PacketPtr=NextPacketPtr;
	ENC624J600ReadRXBuffer(PacketPtr,head,PACKET_HEADER_SIZE);
	NextPacketPtr=head[1]<<8 | head[0];
	length=head[3]<<8 | head[2];

#ifndef IPV6
	ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+12,type,2);
	/*-----------------------it's an ARP packet-------------------------*/
	if (type[0]==0x08 && type[1]==0x06)
	{
		// MAC Address
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+6,mac,MAC_ADDR_SIZE);
		ENC624J600WriteGPBuffer(USER_START_INIT, mac, MAC_ADDR_SIZE);
		ENC624J600WriteGPBuffer(USER_START_INIT+26, mac, MAC_ADDR_SIZE);

		// IP Address
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+28,ip_src, IP_ADDR_SIZE);
		ENC624J600WriteGPBuffer(USER_START_INIT+32, ip_src, IP_ADDR_SIZE);
		
		// send the contents onto the network
		// sender MAC address is automatically set by the chip
		ENC624J600Send(USER_START_INIT,ARP_PACKET_SIZE-MAC_ADDR_SIZE);

		// freed up memory by updating ERXTAIL if needed
		ENC624J600FreedUpMemory();
	}
	else if (type[0]==0x08 && type[1]==0x00)
	{
		ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+23,head,1);
		/*---------------------it's a ping----------------------*/
		if (head[0] == 0x01)
		{
			/*----------------create the response packet------------*/
			ENC624J600WriteUser(PacketPtr+7+35,0x00);
	
			ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+26,head,IP_ADDR_SIZE);
			ENC624J600WriteGPBuffer(PacketPtr+8+30, head, 4);
			
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+26,IP_ADDR_0);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+27,IP_ADDR_1);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+28,IP_ADDR_2);
			ENC624J600WriteUser(PacketPtr+PACKET_HEADER_SIZE+29,IP_ADDR_3);
			
			/*------------------------Send-------------------------*/
			ENC624J600Send(PacketPtr+PACKET_HEADER_SIZE+6,length-10);

			// freed up memory by updating ERXTAIL if needed !
			ENC624J600FreedUpMemory();
		}
#else
	// IPv6
	ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE,type,2);
	/* If the packet is broadcast, we reject it */
	if(type[0] == 0xff)
	{
		// freed up memory by updating ERXTAIL if needed !
		ENC624J600FreedUpMemory();
	}
#endif
		else
		{
			IPPacketTab[write_idx].packetPtr = PacketPtr;
			IPPacketTab[write_idx].nextPtr = NextPacketPtr;
			IPPacketTab[write_idx].size = length-18; // 6 for Mac Src, 6 for Mac Dst, 2 for ethernet type, 4 for ethernet CRC
	
			ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+MAC_ADDR_SIZE,mac,MAC_ADDR_SIZE);
			ENC624J600ReadRXBuffer(PacketPtr+PACKET_HEADER_SIZE+ETH_HEADER_SIZE+IP_START,ip_src,IP_ADDR_SIZE);
	
			// reverse to little endian
			for(temp_i=0; temp_i<IP_ADDR_SIZE; temp_i++)
				reverse_ip[IP_ADDR_SIZE-1-temp_i] = ip_src[temp_i];
				
			add_link_layer_address(reverse_ip, mac);

			// increment the packet index
			if(write_idx < (MAX_PACKET-1))
				write_idx++;
			else
				write_idx=0;
		}
#ifndef IPV6
	}
#endif

	// decrement the packet counter indicate we are done with this packet
	ENC624J600SBI(ENC624J600_SETPKTDEC);
	// Restore context, i.e., Read and Write Pointers
	ENC624J600_Write_TBI(ENC624J600_WRITE_ERXRDPT,save.ERXRDPT);
	ENC624J600_Write_TBI(ENC624J600_WRITE_EGPWRPT,save.EGPWRPT);
	
	return length;
}

