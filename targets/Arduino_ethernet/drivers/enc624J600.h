/*! \file ENC624J600.h \brief Microchip ENC624J600 Ethernet Interface Driver. */
//*****************************************************************************
//
// File Name		: 'ENC624J600.h'
// Title		: Microchip ENC624J600 Ethernet Interface Driver
// Author		: Pascal Stang (c)2005
// Created		: 9/22/2005
// Revised		: 9/22/2005
// Version		: 0.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
///	\ingroup network
///	\defgroup ENC624J600 Microchip ENC624J600 Ethernet Interface Driver (ENC624J600.c)
///	\code #include "net/ENC624J600.h" \endcode
///	\par Overview
///		This driver provides initialization and transmit/receive
///	functions for the Microchip ENC624J600 10Mb Ethernet Controller and PHY.
/// This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
/// chip, using an SPI interface to the host processor.
///
//
//*****************************************************************************
//@{

#ifndef ENC624J600_H
#define ENC624J600_H

#include "avrlibdefs.h"



/* a verifier -------------*/
// ENC624J600 Control Registers
// Control register definitions are a combination of address,
// bank number, and Ethernet/MAC/PHY indicator bits.
// - Register address	(bits 0-4)
// - Bank number	(bits 5-6)
// - MAC/PHY indicator	(bit 7)
#define ADDR_MASK	0x1F
#define BANK_MASK	0x60
#define SPRD_MASK	0x80

/* ------------- a verifier */
// MAJ 21/05/12
// Bank 0 registers
#define ETXSTL 		(0x00|0x00)
#define ETXSTH 		(0x01|0x00)
#define ETXLENL 	(0x02|0x00)
#define ETXLENH 	(0x03|0x00)
#define ERXSTL 		(0x04|0x00)
#define ERXSTH 		(0x05|0x00)
#define ERXTAILL	(0x06|0x00)
#define ERXTAILH 	(0x07|0x00)
#define ERXHEADL 	(0x08|0x00)
#define ERXHEADH	(0x09|0x00)
#define EDMASTL  	(0x0A|0x00)
#define EDMASTH		(0x0B|0x00)
#define EDMALENL 	(0x0C|0x00)
#define EDMALENH	(0x0D|0x00)
#define EDMADSTL 	(0x0E|0x00)
#define EDMADSTH 	(0x0F|0x00)
#define EDMACSL 	(0x10|0x00)
#define EDMACSH 	(0x11|0x00)
#define ETXSTATL 	(0x12|0x00)
#define ETXSTATH	(0x13|0x00)
#define ETXWIREL 	(0x14|0x00)
#define ETXWIREH	(0x15|0x00)
#define EUDASTL 	(0x16|0x00)
#define EUDASTH		(0x17|0x00)
#define EUDANDL 	(0x18|0x00)
#define EUDANDH		(0x19|0x00)
#define ESTATL 		(0x1A|0x00)
#define ESTATH		(0x1B|0x00)
#define EIRL 		(0x1C|0x00)
#define EIRH 		(0x1D|0x00)
#define ECON1L 		(0x1E|0x00)
#define ECON1H		(0x1F|0x00)

//MAJ 21/05/12
// Bank 1 registers
#define EHT1L  		(0x00|0x20)
#define EHT1H 		(0x01|0x20)
#define EHT2L 	 	(0x02|0x20)
#define EHT2H	 	(0x03|0x20)
#define EHT3L 		(0x04|0x20)
#define EHT3H 		(0x05|0x20)
#define EHT4L		(0x06|0x20)
#define EHT4H	 	(0x07|0x20)
#define EPMM1L  	(0x08|0x20)
#define EPMM1H		(0x09|0x20)
#define EPMM2L  	(0x0A|0x20)
#define EPMM2H		(0x0B|0x20)
#define EPMM3L	 	(0x0C|0x20)
#define EPMM3H		(0x0D|0x20)
#define EPMM4L	 	(0x0E|0x20)
#define EPMM4H	 	(0x0F|0x20)
#define EPMCSL  	(0x10|0x20)
#define EPMCSH   	(0x11|0x20)
#define EPMOL  		(0x12|0x20)
#define EPMOH		(0x13|0x20)
#define ERXFCONL  	(0x14|0x20)
#define ERXFCONH	(0x15|0x20)



//MAJ 21/05/12
// Bank 2 registers
#define MACON1L 	(0x00|0x40)
#define MACON1H		(0x01|0x40)
#define MACON2L	 	(0x02|0x40)
#define MACON2H	 	(0x03|0x40)
#define MABBIPGL 	(0x04|0x40)
#define MABBIPGH	(0x05|0x40)
#define MAIPGL 		(0x06|0x40)
#define MAIPGH	 	(0x07|0x40)
#define MACLCONL   	(0x08|0x40)
#define MACLCONH	(0x09|0x40)
#define MAMXFLL   	(0x0A|0x40)
#define MAMXFLH		(0x0B|0x40)





#define MICMDL 		(0x12|0x40)
#define MICMDH		(0x13|0x40)
#define MIREGADRL  	(0x14|0x40)
#define MIREGADRH	(0x15|0x40)



//MAJ 21/05/12
// Bank 3 registers
#define MAADR3L 	(0x00|0x60)
#define MAADR3H		(0x01|0x60)
#define MAADR2L  	(0x02|0x60)
#define MAADR2H	 	(0x03|0x60)
#define MAADR1L 	(0x04|0x60)
#define MAADR1H 	(0x05|0x60)
#define MIWRL 		(0x06|0x60)
#define MIWRH	 	(0x07|0x60)
#define MIRDL   	(0x08|0x60)
#define MIRDH		(0x09|0x60)
#define MISTATL   	(0x0A|0x60)
#define MISTATH		(0x0B|0x60)
#define EPAUSL 	 	(0x0C|0x60)
#define EPAUSH		(0x0D|0x60)
#define ECON2L 	 	(0x0E|0x60)
#define ECON2H	 	(0x0F|0x60)
#define ERXWML   	(0x10|0x60)
#define ERXWMH   	(0x11|0x60)
#define EIEL   		(0x12|0x60)
#define EIEH		(0x13|0x60)
#define EIDLEDL  	(0x14|0x60)
#define EIDLEDH		(0x15|0x60)



//MAJ 22/05/12
// PHY registers
#define PHCON1		0x00
#define PHSTAT1		0x01
#define PHANA		0x04
#define PHANLPA		0x05
#define PHANE		0x06
#define PHCON2		0x11
#define PHSTAT2		0x1B
#define PHSTAT3		0x1F



//MAJ 22/05/12
// ENC624J600 ERXFCON Register Bit Definitions
#define ERXFCON_HTEN	0x8000	//bit16	32768
#define ERXFCON_MPEN	0x4000	//bit15	16384
#define ERXFCON_NOTPM	0x1000	//bit13	4096
#define ERXFCON_PMEN3	0x800	//bit12	2048
#define ERXFCON_PMEN2	0x400	//bit11	1024
#define ERXFCON_PMEN1	0x200	//bit10	512
#define ERXFCON_PMEN0	0x100	//bit9	256
#define ERXFCON_CRCEEN	0x80	//bit8	128
#define ERXFCON_CRCEN	0x40	//bit7	64
#define ERXFCON_RUNTEEN	0x20	//bit6	32
#define ERXFCON_RUNTEN	0x10	//bit5	16
#define ERXFCON_UCEN	0x08	//bit4	8
#define ERXFCON_NOTMEEN	0x04	//bit3	4
#define ERXFCON_MCEN	0x02	//bit2	2
#define ERXFCON_BCEN	0x01	//bit1	1

//MAJ 22/05/12
// ENC624J600 EIE Register Bit Definitions
#define EIE_INTIE		0x8000
#define EIE_MODEXIE		0x4000
#define EIE_HASHIE		0x2000
#define EIE_AESIE		0x1000
#define EIE_LINKIE		0x800
#define EIE_PKTIE		0x40
#define EIE_DMAIE		0x20
#define EIE_TXIE		0x08
#define EIE_TXABTIE		0x04
#define EIE_RXABTIE		0x02
#define EIE_PCFULIE		0x01

//MAJ 22/05/12
// ENC624J600 EIR Register Bit Definitions
#define EIR_CRYPTEN		0x8000
#define EIR_MODEXIF		0x4000
#define EIR_HASHIF		0x2000
#define EIR_AESIF		0x1000
#define EIR_LINKIF		0x800
#define EIR_PKTIF		0x40
#define EIR_DMAIF		0x20
#define EIR_TXIF		0x08
#define EIR_TXABTIF		0x04
#define EIR_RXABTIF		0x02
#define EIR_PCFULIF		0x01

//MAJ 22/05/12
// ENC624J600 ESTAT Register Bit Definitions
#define ESTAT_INT		0x8000	//bit16	32768
#define ESTAT_FCIDLE	0x4000	//bit15	16384
#define ESTAT_RXBUSY	0X2000	//bit13	8192
#define ESTAT_CLKRDY	0x1000	//bit12	4096
#define ESTAT_PHYDPX	0x400	//bit11	1024
#define ESTAT_PHYLNK	0x100	//bit9	256
#define ESTAT_PKTCNT7	0x80	//bit8	128
#define ESTAT_PKTCNT6	0x40	//bit7	64
#define ESTAT_PKTCNT5	0x20	//bit6	32
#define ESTAT_PKTCNT4	0x10	//bit5	16
#define ESTAT_PKTCNT3	0x08	//bit4	8
#define ESTAT_PKTCNT2	0x04	//bit3	4
#define ESTAT_PKTCNT1	0x02	//bit2	2
#define ESTAT_PKTCNT0	0x01	//bit1	1

//MAJ 22/05/12
// ENC624J600 ECON2 Register Bit Definitions
#define ECON2_ETHEN		0x8000	//bit16	32768
#define ECON2_STRCH		0x4000	//bit15	16384
#define ECON2_TXMAC		0x2000	//bit14	8192
#define ECON2_SHA1MD5	0x1000	//bit13	4096
#define ECON2_COCON3	0x0800	//bit12	2048
#define ECON2_COCON2	0x0400	//bit11	1024
#define ECON2_COCON1	0x0200	//bit10	512
#define ECON2_COCON0	0x0100	//bit9	256
#define ECON2_AUTOFC	0x0080	//bit8	128
#define ECON2_TXRST		0x0040	//bit7	64
#define ECON2_RXRST		0x0020	//bit6	32
#define ECON2_ETHRST	0x0010	//bit5	16
#define ECON2_MODLEN1	0x0008	//bit4	8
#define ECON2_MODLEN0	0x0004	//bit3	4
#define ECON2_AESLEN1	0x0002	//bit2	2
#define ECON2_AESLEN0	0x0001	//bit1	1

//MAJ 22/05/12
// ENC624J600 ECON1 Register Bit Definitions
#define ECON1_MODEXST	0x8000	//bit16	32768
#define ECON1_HASHEN	0x4000	//bit15	16384
#define ECON1_HASOP		0x2000	//bit14 8192
#define ECON1_HASHLST	0x1000	//bit13	4096
#define ECON1_AESST		0x800	//bit12	2048
#define ECON1_AESOP1	0x400	//bit11	1024
#define ECON1_AESOP0	0x200	//bit10	512
#define ECON1_PKTDEC	0x100	//bit9	256
#define ECON1_FCOP1		0x80	//bit8	128
#define ECON1_FCOP0		0x40	//bit7	64
#define ECON1_DMAST		0x20	//bit6	32
#define ECON1_DMACPY	0x10	//bit5	16
#define ECON1_DMACSSD	0x08	//bit4	8
#define ECON1_DMANOCS	0x04	//bit3	4
#define ECON1_TXRTS		0x02	//bit2	2
#define ECON1_RXEN		0x01	//bit1	1

//MAJ 22/05/12
// ENC624J600 MACON1 Register Bit Definitions
#define MACON1_LOOPBK	0x10
#define MACON1_RXPAUS	0x04
#define MACON1_PASSALL	0x02

//MAJ 22/05/12
// ENC624J600 MACON2 Register Bit Definitions
#define MACON2_DEFER	0x4000	//bit15	16384
#define MACON2_BPEN		0x2000	//bi14 	8192
#define MACON2_NOBKOFF	0x1000	//bit13	4096
#define MACON2_PADCFG2	0x80	//bit8	128
#define MACON2_PADCFG1	0x40	//bit7	64
#define MACON2_PADCFG0	0x20	//bit6	32
#define MACON2_TXCRCEN	0x10	//bit5	16
#define MACON2_PHDREN	0x08	//bit4	8
#define MACON2_HFRMEN	0x04	//bit3	4
#define MACON2_RULDPX	0x01	//bit1	1

//MAJ 22/05/12
// ENC624J600 MICMD Register Bit Definitions
#define MICMD_MIISCAN	0x02
#define MICMD_MIIRD		0x01

//MAJ 22/05/12
// ENC624J600 MISTAT Register Bit Definitions
#define MISTAT_NVALID	0x04
#define MISTAT_SCAN		0x02
#define MISTAT_BUSY		0x01


//MAJ 22/05/12
// ENC624J600 PHCON1 Register Bit Definitions
#define PHCON1_PRST		0x8000	//bit16	32768
#define PHCON1_PLOOPBK	0x4000	//bit15	16384
#define PHCON1_SPD100	0x2000	//bit14	8192
#define PHCON1_ANEN		0x1000	//bit13	4096
#define PHCON1_PSLEEP	0x800	//bit12	2048
#define PHCON1_RENEG	0x200	//bit10	512
#define PHCON1_PFULDPX	0x100	//bit9	256


//MAJ 22/05/12
// ENC624J600 PHSTAT1 Register Bit Definitions
#define PHSTAT1_FULL100	0x4000	//bit15	16384
#define PHSTAT1_HALF100	0x2000	//bit14	8192
#define PHSTAT1_FULL10	0x1000	//bit13	4096
#define PHSTAT1_HALF10	0x800	//bit12	2048
#define PHSTAT1_ANDONE	0x20	//bit6	32
#define PHSTAT1_LRFAULT	0x10	//bit5	16
#define PHSTAT1_ANABLE	0x08	//bit4	8
#define PHSTAT1_LLSTAT	0x04	//bit3	4
#define PHSTAT1_EXTREGS	0x01	//bit1	1

//MAJ 22/05/12
// ENC624J600 PHSTAT2 Register Bit Definitions
#define PHSTAT2_PLRITY	0x10	//bit5	16


//MAJ 22/05/12
// ENC624J600 PHSTAT3 Register Bit Definitions
#define PHSTAT3_SPDDPX2	0x20	//bit6	32
#define PHSTAT3_SPDDPX1	0x10	//bit5	16
#define PHSTAT3_SPDDPX0	0x08	//bit4	8




//MAJ 22/05/12
// ENC624J600 PHANA Register Bit Definitions
#define PHANA_ADNP		0x8000	//bit16	32768
#define PHANA_ADFAULT	0x2000	//bit14 8192
#define PHANA_ADPAUS1	0x800	//bit12	2048
#define PHANA_ADPAUS0	0x400	//bit11	1024
#define PHANA_AD100FD	0x100	//bit9	256
#define PHANA_AD100		0x80	//bit8	128
#define PHANA_AD10FD	0x40	//bit7	64
#define PHANA_AD10		0x20	//bit6	32
#define PHANA_ADIEEE4	0x10	//bit5	16
#define PHANA_ADIEEE3	0x08	//bit4	8
#define PHANA_ADIEEE2	0x04	//bit3	4
#define PHANA_ADIEEE1	0x02	//bit2	2
#define PHANA_ADIEEE0	0x01	//bit1	1




//MAJ 22/05/12
// ENC624J600 PHANLPA Register Bit Definitions
#define PHANLPA_LPNP	0x8000	//bit16	32768
#define PHANLPA_LPACK	0x4000	//bit15	16384
#define PHANLPA_LPFAULT	0x2000	//bit14 8192

#define PHANLPA_LPPAUS1	0x800	//bit12	2048
#define PHANLPA_LPPAUS0	0x400	//bit11	1024
#define PHANLPA_LP100T4	0x200	//bit10	512
#define PHANLPA_LP100FD	0x100	//bit9	256
#define PHANLPA_LP100	0x80	//bit8	128
#define PHANLPA_LP10FD	0x40	//bit7	64
#define PHANLPA_LDP10	0x20	//bit6	32
#define PHANLPA_LPIEEE4	0x10	//bit5	16
#define PHANLPA_LPIEEE3	0x08	//bit4	8
#define PHANLPA_LPIEEE2	0x04	//bit3	4
#define PHANLPA_LPIEEE1	0x02	//bit2	2
#define PHANLPA_LPIEEE0	0x01	//bit1	1




//MAJ 22/05/12
// ENC624J600 PHANE Register Bit Definitions
#define PHANE_PDFLT		0x10	//bit5	16
#define PHANE_LPARCD	0x02	//bit2	2
#define PHANE_LPANABL	0x01	//bit1	1



//MAJ 22/05/12
// ENC624J600 PHCON2 Register Bit Definitions

#define PHCON_EDPWRDN	0x2000	//bit14 8192
#define PHCON_EDTHRES	0x800	//bit12	2048
#define PHCON_FRCLNK	0x04	//bit3	4
#define PHCON_EDSTAT	0x02	//bit2	2





//MAJ 22/05/12
// SPI operation codes
// Single Byte Instructions
#define ENC624J600_BANK0_SELECT				0xC0 // B0SEL
#define ENC624J600_BANK1_SELECT				0xC2 // B1SEL
#define ENC624J600_BANK2_SELECT				0xC4 // B2SEL
#define ENC624J600_BANK3_SELECT				0xC6 // B3SEL
#define ENC624J600_ETH_RESET				0xCA // SETETHRST
#define ENC624J600_FLOW_CONTROL_DISABLE		0xE0 // FCDISABLE
#define ENC624J600_FLOW_SINGLE				0xE2 // FCSINGLE
#define ENC624J600_FLOW_MULTIPLE			0xE4 // FCMULTIPLE
#define ENC624J600_FLOW_CLEAR				0xE6 // FCCLEAR
#define ENC624J600_SETPKTDEC				0xCC // SETPKTDEC
#define ENC624J600_DMA_STOP					0xD2 // DMASTOP
#define ENC624J600_DMA_START_CHEKSUM		0xD8 // DMACKSUM
#define ENC624J600_DMA_START_CHEKSUM_SEED	0xDA // DMACKSUMS
#define ENC624J600_DMA_START_COPY			0xDC // DMACOPY
#define ENC624J600_DMA_START_COPY_SEED		0xDE // DMACOPYS
#define ENC624J600_SETTXRTS					0xD4 // SETTXRTS
#define ENC624J600_ENABLE_RX				0xE8 // ENABLERX
#define ENC624J600_DISABLE_RX				0xEA // DISABLERX
#define ENC624J600_ENABLE_INTERRUPTS		0xEC // SETEIE
#define ENC624J600_DISABLE_INTERRUPTS		0xEE // CLREIE
// Two-Byte Instructions
#define ENC624J600_READ_BANK_SELECT			0xC8 // RBSEL
// Three-Byte Instructions
#define ENC624J600_WRITE_EGPRDPT			0x60
#define ENC624J600_READ_EGPRDPT				0x62
#define ENC624J600_WRITE_ERXRDPT			0x64
#define ENC624J600_READ_ERXRDPT				0x66
#define ENC624J600_WRITE_EUDARDPT 			0xC8
#define ENC624J600_READ_EUDARDPT 			0x6A
#define ENC624J600_WRITE_EGPWRPT  			0x6C
#define ENC624J600_READ_EGPWRPT 	 		0x6E
#define ENC624J600_WRITE_ERXWRPT   			0x70 // WRXWRPT
#define ENC624J600_READ_ERXWRPT   			0x72
#define ENC624J600_WRITE_EUDAWRPT    		0x74
#define ENC624J600_READ_EUDAWRPT    		0x76
#define ENC624J600_READ_CONTROL_REGISTER	0x00
#define ENC624J600_WRITE_CONTROL_REGISTER	0x40	
#define ENC624J600_READ_CONTROL_REGISTER_UNBANK		0x20
#define ENC624J600_WRITE_CONTROL_REGISTER_UNBANK	0x22
#define ENC624J600_BIT_FIELD_SET			0x80
#define ENC624J600_BIT_FIELD_CLEAR			0xA0
#define ENC624J600_BIT_FIELD_SET_UNBANK		0x24 // BFSU
#define ENC624J600_BIT_FIELD_CLEAR_UNBAN	0x26 // BFCU
#define ENC624J600_READ_EGPDATA				0x28 // RGPDATA
#define ENC624J600_WRITE_EGPDATA			0x2A // WGPDATA
#define ENC624J600_READ_ERXDATA 			0x2C // RRXDATA
#define ENC624J600_WRITE_ERXDATA 			0x2E // WRXDATA
#define ENC624J600_READ_EUDADATA  			0x30 // RUDADATA
#define ENC624J600_WRITE_EUDADATA  			0x32 // WUDADATA
	


#define USER_START_INIT		0x0000
#define USER_STOP_INIT		0x0029
#define TXSTART_INIT		0x0030
#define TXSTART_2			0x0A00
#define TXSTOP_INIT			0x1339
#define RXSTART_INIT   		0x1340	
#define RXSTOP_INIT			0x5FFE

/*		------------------------------	0000
			ARP (exclusion zone)		
		------------------------------ 	0026
					TX1
		------------------------------	0A00
					TX2
		------------------------------	1340
					RX
		------------------------------	5FFFF */


// #define PACKET_START		TXSTART_INIT

#define	MAX_FRAMELEN	1518	// maximum ethernet frame length

// Ethernet constants
//#define ETHERNET_MIN_PACKET_LENGTH	0x3CZ
//#define ETHERNET_HEADER_LENGTH		0x0E
#define ARP_RESPONSE_PACKET_SIZE	42

#define CS_DISABLED 	1
#define CS_ENABLED 		0

// functions
#include "enc624J600conf.h"


typedef struct sauvegarde_s {
	uint8_t ERXRDPT[2];
	uint8_t EGPWRPT[2];
} sauvegarde_t;


//! do a ENC624J600 read operation
uint8_t ENC624J600ReadOp(uint8_t op, uint8_t address);
//! do a ENC624J600 write operation
//void ENC624J600WriteOp(uint8_t op, uint8_t address, uint8_t data);
//! read the packet buffer memory
//void ENC624J600ReadBuffer(uint16_t len, uint16_t address,volatile uint8_t* data);
//! write the packet buffer memory
void ENC624J600WriteGPBuffer(uint16_t address, volatile uint8_t *data, uint16_t len);
//! set the register bank for register at address
//void ENC624J600SetBank(uint8_t address);
//! read ax88796 register
//uint8_t ENC624J600Read(uint8_t address);
//! write ax88796 register
void ENC624J600Write(uint8_t address, uint8_t data);
//! read a PHY register
//uint16_t ENC624J600PhyRead(uint8_t address);
//! write a PHY register
//void ENC624J600PhyWrite(uint8_t address, uint16_t data);

//! initialize the ethernet interface for transmit/receive
void ENC624J600Init(void);

void ENC624J600Send(uint16_t address, uint16_t len);

uint16_t ENC624J600PacketReceive(void);

void ENC624J600SBI(uint8_t instruction);

void ENC624J600ReadRXBuffer(uint16_t address, volatile uint8_t *data, uint16_t len);

#endif

