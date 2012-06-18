#include <avr/io.h>
#include <util/delay.h>
#include <string.h>

#include "nic.h"
#include "enc624J600.h"
#include "dev.h"



#define PACKET_HEADER_SIZE	8
#define ARP_PACKET_SIZE	42
#define MAC_ADDR_SIZE	6
#define IP_ADDR_SIZE	4


// DATA 
u16 length;
#ifdef DEBUG
char chaine[256];
#endif

u08 ENC624J600Bank;
volatile u16 NextPacketPtr,PacketPtr;
volatile unsigned char head[8];

volatile u08 packet_transmit;


volatile packet_t IPPacketTab[MAX_PACKET];
volatile unsigned char read_idx = 0, write_idx = 0;
volatile const unsigned char my_mac[] = {ENC624J600_MAC0, ENC624J600_MAC1, ENC624J600_MAC2, ENC624J600_MAC3, ENC624J600_MAC4, ENC624J600_MAC5, 0x08, 0x00};


volatile unsigned int packet_size=0;


unsigned char arpResponse[ARP_RESPONSE_PACKET_SIZE] = { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	ENC624J600_MAC0, ENC624J600_MAC1, ENC624J600_MAC2, ENC624J600_MAC3, ENC624J600_MAC4, ENC624J600_MAC5,
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


// MAJ 24_05_12
#ifdef DEBUG
u08 stateSend(){
	int i=0;
	u08 L=ENC624J600Read(ETXSTATL);
	u08 H=ENC624J600Read(ETXSTATH);
	sprintf(chaine,"  ETXSTAT : %2x %2x\n",L,H);
	displayString(chaine);
	displayString("Report send !\n L : ");
	for(i=7;i>=0;i--){
		if(CHECK_BIT(L,i))
			displayString("1");
		else
			displayString("0");
	}
	displayString("\n H : ");
	for(i=7;i>=0;i--){
		if(CHECK_BIT(H,i))
			displayString("1");
		else
			displayString("0");
	}
	displayString("\n");
	displayString("Report END !\n");
	return 0;
}
#endif





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

//MAJ 29_05_12
void ENC624J600ReadBuffer(u16 len,u16 address,volatile u08* data)
{
	// assert CS
//	ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);
	                  
	ENC624J600WriteOp16(ENC624J600_WRITE_ERXRDPT,address);
//	while(!(SPSR & (1<<SPIF)));
//	*data = ENC624J600ReadOp(ENC624J600_READ_ERXDATA,0);
	//while(--len)
	while(len--)
	{
		// read data
 //		SPDR = 0x00;
//		while(!(SPSR & (1<<SPIF)));
 		*data++ = ENC624J600ReadOp(ENC624J600_READ_ERXDATA,0);
 		//SPDR = 0x00;

	}
	// release CS
//	ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
	
}
//MAJ 06_06_12
void ENC624J600WriteBuffer(u16 len, volatile u08* data,u16 address)
{
	// assert CS
	//ENC624J600_CONTROL_PORT &= ~(1<<ENC624J600_CONTROL_CS);
	
	// issue write command
	ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,address);
	while(len--)
	{
		ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,*data++);
	}	
	// release CS
	//ENC624J600_CONTROL_PORT |= (1<<ENC624J600_CONTROL_CS);
	
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
				ENC624J600WriteOp(ENC624J600_BANK0_SELECT, 0,ENC624J600_BANK0_SELECT);
				#ifdef DEBUG
				displayString("BANK 0\n");
				#endif
				break;
			case 1 :
				ENC624J600WriteOp(ENC624J600_BANK1_SELECT, 0,ENC624J600_BANK1_SELECT);
				#ifdef DEBUG
				displayString("BANK 1\n");
				#endif				
				break;
			case 2 :
				ENC624J600WriteOp(ENC624J600_BANK2_SELECT, 0,ENC624J600_BANK2_SELECT);
				
				#ifdef DEBUG
				displayString("BANK 2\n");
				#endif
				break;
			case 3 :
				ENC624J600WriteOp(ENC624J600_BANK3_SELECT, 0,ENC624J600_BANK3_SELECT);
				#ifdef DEBUG
				displayString("BANK 3\n");
				#endif				
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


//MAJ 22/05/12
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

//MAJ 22/05/12
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


//MAJ 31/05/12
void ENC624J600Init(void)
{
	ENC624J600Bank=3;
	packet_transmit=0;
	
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
		ENC624J600Write(EUDASTL,0x12);
		ENC624J600Write(EUDASTH,0x34);
	// 		sprintf(chaine,"  EUDASTL=>0x1234\n");
	// 		displayString(chaine);
		//STEP TWO
		while(ENC624J600Read(EUDASTL)!=0x12 || ENC624J600Read(EUDASTH)!=0x34 ){
			ENC624J600Write(EUDASTL,0x12);
			ENC624J600Write(EUDASTH,0x34);

		}
		//STEP THREE
		while(ENC624J600Read(ESTATH) & ESTAT_CLKRDY);

		//STEP FOUR

		ENC624J600Write(ECON2L,ECON2_ETHRST);

		//STEP FIVE
		_delay_us(25);
		//STEP SIX
		if (ENC624J600Read(EUDASTL)==0x00 && ENC624J600Read(EUDASTH)==0x00){
			
			_delay_us(260);		
			//8.2 CLKOUT Frequency
			// Arduino : 16MHz =>  COCON=0100 
				ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON2H,ECON2_COCON2>>8);
			//8.3 reception
				NextPacketPtr = RXSTART_INIT;
				ENC624J600Write(ERXSTL, RXSTART_INIT&0x00FF);
				ENC624J600Write(ERXSTH, RXSTART_INIT>>8);
				#ifdef DEBUG
				sprintf(chaine," buffer RX : %2x%2x ! ====> ",ENC624J600Read(ERXSTH),ENC624J600Read(ERXSTL));
				displayString(chaine);
				#endif
				ENC624J600Write(ERXTAILL, RXSTOP_INIT&0x00FF);
				ENC624J600Write(ERXTAILH, RXSTOP_INIT>>8);
				#ifdef DEBUG
 				sprintf(chaine," %2x%2x !\n",ENC624J600Read(ERXTAILH),ENC624J600Read(ERXTAILL));
 				displayString(chaine);
				sprintf(chaine," debut du paquet(HEAD) : %2x%2x et NextPacketPtr :%04X!\n",ENC624J600Read(ERXHEADH),ENC624J600Read(ERXHEADL),NextPacketPtr);
 				displayString(chaine);
				#endif
				
	// 		// USER buffer
	// 				
	// 				ENC624J600Write(EUDASTL, USER_START_INIT&0x00FF);
	// 				ENC624J600Write(EUDASTH, USER_START_INIT>>8);
	// 				ENC624J600Write(EUDANDL, USER_STOP_INIT&0x00FF);
	// 				ENC624J600Write(EUDANDH, USER_STOP_INIT>>8);
			//8.4 RAF

			//8.5 RECEIVE FILTER TODO!!!
	// 			sprintf(chaine,"  ERXFCON : %2x %2x\n",ENC624J600Read(ECON2L),ENC624J600Read(ECON2H));
	//  			displayString(chaine);
			  // crc ERROR FILTER => disabled
			  // frames shorter than 64 bits => disabled
			  // CRC error rejection => enabled
			  // Unicast collection filter => enabled
			  // Not me unicast filter => disabled
			  // Multicast collection filter => ne pas activer le multicast
				ENC624J600Write(ERXFCONL,ERXFCON_CRCEN|ERXFCON_RUNTEN|ERXFCON_UCEN);
				#ifdef DEBUG
				sprintf(chaine,"filter low : %02X \n",ENC624J600Read(ERXFCONL));
				displayString(chaine);
				#endif
				// brodcast collection filter => enabled
			  // Hash table collection filter.. compris mais je sais pas si c'est disabled ou enabled
			  // Magic packet => desabled TODO
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
				#ifdef DEBUG
				sprintf(chaine,"pattern : %02X %02X %02X %02X %02X %02X\n",ENC624J600Read(EPMM1H),ENC624J600Read(EPMM1L),ENC624J600Read(EPMM2H),ENC624J600Read(EPMM2L),ENC624J600Read(EPMM3H),ENC624J600Read(EPMM3L));
				displayString(chaine);
				#endif			
				//CheckSum
				ENC624J600Write(EPMCSL,ip_checksum(20,temp1)&0xFF);
				ENC624J600Write(EPMCSH,ip_checksum(20,temp1)>>8);
				#ifdef DEBUG		
				sprintf(chaine,"checksum  : %02X %02X \n",ENC624J600Read(EPMCSH),ENC624J600Read(EPMCSL));
				displayString(chaine);
				#endif	
				//exact pattern
				ENC624J600Write(ERXFCONH,0x01);
				#ifdef DEBUG
				sprintf(chaine,"filter hight : %02X \n",ENC624J600Read(ERXFCONH));
				displayString(chaine);
				#endif	

			      
			// 8.6 MAC initialization ...
				//flow control ???
				ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, MACON2L, MACON2_TXCRCEN|MACON2_PADCFG0|MACON2_PADCFG1|MACON2_HFRMEN);
				// enable reception
				ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_RXEN);
				ENC624J600Write(MAMXFLL, MAX_FRAMELEN&0xFF);	
				ENC624J600Write(MAMXFLH, MAX_FRAMELEN>>8);
				
				ENC624J600Write(MAADR1L, ENC624J600_MAC0);
				ENC624J600Write(MAADR1H, ENC624J600_MAC1);
				ENC624J600Write(MAADR2L, ENC624J600_MAC2);
				ENC624J600Write(MAADR2H, ENC624J600_MAC3);
				ENC624J600Write(MAADR3L, ENC624J600_MAC4);
				ENC624J600Write(MAADR3H, ENC624J600_MAC5);
				#ifdef DEBUG
				sprintf(chaine,"address MAC effective : %02X:%02X:%02X:%02X:%02X:%02X \n",ENC624J600Read(MAADR3H),ENC624J600Read(MAADR3L),ENC624J600Read(MAADR2H),ENC624J600Read(MAADR2L),ENC624J600Read(MAADR1H),ENC624J600Read(MAADR1L));
				displayString(chaine);
				#endif
				//8.7 PHY initialization 
				// auto-negotiation ?
				//ENC624J600PhyWrite(PHANA,0x05E1);
			// 8.8 OTHER considerations
				//half-duplex mode
				//ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, MACON2H,MACON2_DEFER|MACON2_BPEN|MACON2_NOBKOFF);$
				
			// enable interuption
				ENC624J600Write(EIEH,0x80);
//				ENC624J600Write(EIEL,0x40);
				ENC624J600Write(EIEL,0x48);
				// configuration LED
	// 				ENC624J600Write(EIDLEDH,0x54);
	// 				ENC624J600PhyWrite(PHCON1,PHCON1_PFULDPX);
		}
		else{
			#ifdef DEBUG
			  displayString("-------initialization failed-----\n");
			#endif
		}
		#ifdef DEBUG
 		displayString("------initialization DONE ----\n");
		#endif
		write_idx=0;
		read_idx=0;
}

//MAJ 24_05_12
void ENC624J600PacketSend(unsigned int len, unsigned char* packet)
{
	// copy the packet into the transmit buffer
	ENC624J600WriteBuffer(len, packet,TXSTART_INIT);
	
	// Set the write pointer to start of transmit buffer area
	ENC624J600Write(ETXSTL, TXSTART_INIT&0x00FF);
	ENC624J600Write(ETXSTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	ENC624J600Write(ETXLENL, (TXSTART_INIT+len)&0x00FF);
	ENC624J600Write(ETXLENH, (TXSTART_INIT+len)>>8);

	// send the contents of the transmit buffer onto the network
	ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_TXRTS);
}

#ifdef  DEBUG
void ENC624J600Print(u16 len,volatile u08 *packet){
	displayString("\t");
	u16 temp=0;
	while((len)--){
		sprintf(chaine," %02X",packet[temp++]);
		displayString(chaine);
		if (len%16==1)
			displayString("\n\t");
		}
	displayString("\n\n");
}
#endif

//maj 05_05_12
void ENC624J600WriteUser(u16 address,u08 data)
{
	ENC624J600WriteOp16(ENC624J600_WRITE_EGPWRPT,address);
	ENC624J600WriteOp(ENC624J600_WRITE_EGPDATA,0,data);
}

//MAJ 06_06_12
void ENC624J600DMABuffer(u16 len,u16 source,u16 dest){
	#ifdef  DEBUG
	sprintf(chaine,"DMA COPY ERXST : %02X%02X, EUDAND : %02X%02X, EUDAST : %02X%02X\n",ENC624J600Read(ERXSTH),ENC624J600Read(ERXSTL),ENC624J600Read(EUDANDH),ENC624J600Read(EUDANDL),ENC624J600Read(EUDASTH),ENC624J600Read(EUDASTL));
	displayString(chaine);
	#endif
	if ( (ENC624J600Read(ECON1L) & ECON1_DMAST )==0x00){
		ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_DMACPY);
		#ifdef  DEBUG
		sprintf(chaine,"ECON1L %02X\n",ENC624J600Read(ECON1L));
		displayString(chaine);
		#endif
		ENC624J600Write(EDMASTL,(source)&0xFF);
		ENC624J600Write(EDMASTH,(source)>>8);
		#ifdef  DEBUG
		sprintf(chaine,"EDMAST %02X%02X\n",ENC624J600Read(EDMASTH),ENC624J600Read(EDMASTL));
		displayString(chaine);
		#endif
		ENC624J600Write(EDMADSTL,(dest)&0xFF);
		ENC624J600Write(EDMADSTH,(dest)>>8);
		#ifdef  DEBUG
		sprintf(chaine,"EDMADST %02X%02X\n",ENC624J600Read(EDMADSTH),ENC624J600Read(EDMADSTL));
		displayString(chaine);
		#endif

		ENC624J600Write(EDMALENL,(len)&0xFF);
		ENC624J600Write(EDMALENH,(len)>>8);

		#ifdef  DEBUG
		sprintf(chaine,"LEN %02X%02X\n",ENC624J600Read(EDMALENH),ENC624J600Read(EDMALENL));
		displayString(chaine);
		#endif

		ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_DMAST);
	   /*   displayString("travail...");*/
		while((ENC624J600Read(ECON1L) & ECON1_DMAST )!=0x00);
	}
}



//MAJ 05_05_12
unsigned int ENC624J600PacketReceive(unsigned int maxlen)
{
	volatile unsigned char type[2];

	// check if a packet has been received and buffered
	if( !(ENC624J600Read(EIRL) & EIR_PKTIF) )
		return 0;

	u08 i;
	u08 byte[1];
	PacketPtr=NextPacketPtr;
	//ENC624J600ReadBuffer(10,PacketPtr-1,head);
	ENC624J600ReadBuffer(PACKET_HEADER_SIZE,PacketPtr,head);
	NextPacketPtr=head[1]<<8;
	NextPacketPtr|=head[0];
	length=head[3]<<8;
	length|=head[2];
	#ifdef  DEBUG
  	sprintf(chaine,"------------incoming---------------\n PacketPtr :%04X \n Next : %04X\n RSV : Lenght : %04X  \n RSV : Autre  : %02X%02X\n RSV : Autre2 : %02X%02X \n\n",PacketPtr,NextPacketPtr,length,head[4],head[5],head[6],head[7]);
  	displayString(chaine);
	#endif
	//ENC624J600ReadBuffer(3,PacketPtr+7+12,type);
	ENC624J600ReadBuffer(2,PacketPtr+PACKET_HEADER_SIZE+12,type);
	/*------------------------test si c'est un ARP-------------------------*/
	if (type[0]==0x08 && type[1]==0x06)
	{
		ENC624J600ReadBuffer(MAC_ADDR_SIZE,PacketPtr+PACKET_HEADER_SIZE+6,arpResponse);
		strncpy(arpResponse+32, arpResponse, MAC_ADDR_SIZE);
	
		ENC624J600ReadBuffer(IP_ADDR_SIZE,PacketPtr+PACKET_HEADER_SIZE+28,arpResponse + 38);
		nicSend(ARP_PACKET_SIZE, arpResponse); 
	}
	else {
		//ENC624J600ReadBuffer(2,PacketPtr+7+23,byte);
		ENC624J600ReadBuffer(1,PacketPtr+PACKET_HEADER_SIZE+23,byte);
		/*---------------------test si c'est un ping----------------------*/
		if (type[0]==0x08 && type[1]==0x00 && byte[0] == 0x01){
			#ifdef DEBUG
			displayString("PING!\n");
			#endif
			/*------------------------traitement-------------------------*/
			ENC624J600WriteUser(PacketPtr+7+35,0x00);
			for(i=0; i<6; i++){
				//ENC624J600ReadBuffer(2,PacketPtr+7+i+6,byte);
				ENC624J600ReadBuffer(1,PacketPtr+8+i+6,byte);
				ENC624J600WriteUser(PacketPtr+7+i+1,byte[0]);
			}
			ENC624J600WriteUser(PacketPtr+7+7,ENC624J600_MAC0);
			ENC624J600WriteUser(PacketPtr+7+8,ENC624J600_MAC1);
			ENC624J600WriteUser(PacketPtr+7+9,ENC624J600_MAC2);
			ENC624J600WriteUser(PacketPtr+7+10,ENC624J600_MAC3);
			ENC624J600WriteUser(PacketPtr+7+11,ENC624J600_MAC4);
			ENC624J600WriteUser(PacketPtr+7+12,ENC624J600_MAC5);
			for(i=0;i<4;i++){
				//ENC624J600ReadBuffer(2,PacketPtr+7+i+26,byte);
				ENC624J600ReadBuffer(1,PacketPtr+8+i+26,byte);
				ENC624J600WriteUser(PacketPtr+7+i+30+1,byte[0]);
			}
			ENC624J600WriteUser(PacketPtr+7+27,IP_ADDR_0);
			ENC624J600WriteUser(PacketPtr+7+28,IP_ADDR_1);
			ENC624J600WriteUser(PacketPtr+7+29,IP_ADDR_2);
			ENC624J600WriteUser(PacketPtr+7+30,IP_ADDR_3);

			/*------------------------ENVOIE-------------------------*/
			ENC624J600DMABuffer(length-3,PacketPtr+7,TXSTART_INIT);
			
			// Set the write pointer to start of transmit buffer area
			ENC624J600Write(ETXSTL, (TXSTART_INIT+1)&0x00FF);
			ENC624J600Write(ETXSTH, TXSTART_INIT>>8);
			// Set the TXND pointer to correspond to the packet size given
			ENC624J600Write(ETXLENL, (TXSTART_INIT+length-4)&0x00FF);
			ENC624J600Write(ETXLENH, (TXSTART_INIT+length-4)>>8);
			// send the contents of the transmit buffer onto the network
			ENC624J600WriteOp(ENC624J600_BIT_FIELD_SET, ECON1L, ECON1_TXRTS);
		}
		else{
			#ifdef DEBUG
			displayString("TCP ou UDP\n");
			#endif
			u08 port[2];
			//ENC624J600ReadBuffer(3,PacketPtr+7+36,port);;
			ENC624J600ReadBuffer(2,PacketPtr+PACKET_HEADER_SIZE+36,port);;
			if (port[0]==0x00 && port[1]==0x50){
				 #ifdef DEBUG
					displayString("c'est bon !\n");
				 #endif
				IPPacketTab[write_idx].packetPtr = PacketPtr;
				IPPacketTab[write_idx].nextPtr = NextPacketPtr;
				IPPacketTab[write_idx].size = length-18; // 6 for Mac Src, 6 for Mac Dst, 2 for ethernet type, 4 for ethernet CRC
				ENC624J600ReadBuffer(6,PacketPtr+PACKET_HEADER_SIZE+6,IPPacketTab[write_idx].mac_src);
				#ifdef DEBUG
				sprintf(chaine,"mac_src : %02X:%02X:%02X:%02X:%02X:%02X\n",IPPacketTab[write_idx].mac_src[0],IPPacketTab[write_idx].mac_src[1],IPPacketTab[write_idx].mac_src[2],IPPacketTab[write_idx].mac_src[3],IPPacketTab[write_idx].mac_src[4],IPPacketTab[write_idx].mac_src[5]);
				displayString(chaine);
				#endif
				if(write_idx < (MAX_PACKET-1))
					write_idx++;
				else
					write_idx=0;
			}else{
				length=0;
			}
		}
	}
	 	
	// freed up memory by updating ERXTAIL
	ENC624J600Write(ERXTAILL, (NextPacketPtr-2)&0x00FF);
	ENC624J600Write(ERXTAILH, (NextPacketPtr-2)>>8);
	
	// decrement the packet counter indicate we are done with this packet
	ENC624J600Write(ECON1H,ECON1_PKTDEC>>8);
	return length;
}

