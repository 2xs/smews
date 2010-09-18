#ifndef MAC_H_
#define MAC_H_



/*******************************************************************************************************
 *******************************************************************************************************
 ******************     Constants concerned with the Basic RF packet format    *************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
// Packet overhead ((frame control field, sequence number, PAN ID, destination and source) + (footer))
// Note that the length byte itself is not included included in the packet length
#define CC2420_PACKET_OVERHEAD_SIZE_DsSs     ((2 + 1 + 2 + 2 + 2) + (2))
#define CC2420_PACKET_OVERHEAD_SIZE_DsSl     ((2 + 1 + 2 + 2 + 8) + (2))
#define CC2420_PACKET_OVERHEAD_SIZE_DlSs     ((2 + 1 + 2 + 8 + 2) + (2))
#define CC2420_MAX_PAYLOAD_SIZE		    (CC2420_MAX - CC2420_PACKET_OVERHEAD_SIZE_DsSs)
#define CC2420_ACK_PACKET_SIZE		    5
#define CC2420_MAX                          127


// The time it takes for the acknowledgment packet to be received after the data packet has been transmitted
#define CC2420_ACK_DURATION			(0.5 * 32 * 2 * ((4 + 1) + (1) + (2 + 1) + (2)))
#define CC2420_SYMBOL_DURATION	    (32 * 0.5)

// The length byte
#define CC2420_LENGTH_MASK            0x7F

// Frame control field
#define CC2420_FCF_FRAMETYPE_DATA       0x0001
#define CC2420_FCF_FRAMETYPE_ACK        0x0002

#define CC2420_FCF_INTRAPAN             0x0040

#define CC2420_FCF_ACK_REQUEST          0x0020
#define CC2420_FCF_NO_ACK_REQUEST       0x0000
#define CC2420_FCF_DESTADDR_16BIT       0x0800
#define CC2420_FCF_DESTADDR_64BIT       0x0C00
#define CC2420_FCF_DESTADDR_BM          0x0C00
#define CC2420_FCF_SOURCEADDR_16BIT     0x8000
#define CC2420_FCF_SOURCEADDR_64BIT     0xC000
#define CC2420_FCF_SOURCEADDR_BM        0xC000

#define CC2420_FCF_ACK_BM               0x0020



// Footer
#define CC2420_CRC_OK_BM              0x80
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// The data structure which is used to transmit packets
typedef struct {
    //unsigned int destPanId;
	unsigned int destAddr;
	//unsigned char destIEEE[8];
	unsigned char length;
    unsigned char *pPayload;
	unsigned char ackRequest;
} CC2420_TX_INFO;
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// The receive struct:
typedef struct {
    unsigned char seqNumber;
	unsigned int srcAddr;
	//unsigned char srcIEEE[8];
	unsigned int srcPanId;
	unsigned char length;
        unsigned char *pPayload;
	unsigned char ackRequest;
	unsigned char rssi;
} CC2420_RX_INFO;
//-------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------
// The RF settings structure:
typedef struct {
    CC2420_RX_INFO *pRxInfo;
    unsigned char txSeqNumber;
    volatile unsigned char ackReceived;
    unsigned int panId;
    unsigned int myAddr;
    //unsigned char myIEEE[8];
    unsigned char receiveOn;
    //unsigned char JoinNetworkSuccess; 
} CC2420_SETTINGS;
//-------------------------------------------------------------------------------------------------------




 /******************************************************************************************************
 **************************                     Definition                      **************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
void CC2420_Init();
unsigned char CC2420_SendPacket(CC2420_TX_INFO *pRTI);

void CC2420_SetChannel(unsigned char channel_temp); 
void CC2420_SetPanId(unsigned int PanId);
void CC2420_SetShortAddress(unsigned int ShortAddress);
//unsigned char* CC2420_ReadIEEEAddr(void);
//void CC2420_SetIEEEAddr(void);

void CC2420_ReceiveOn(void);
void CC2420_ReceiveOff(void); 


void MAC_Init(void);

void mac_pRxBuffer_Clear(void);
void mac_pTxBuffer_Clear(void);
//-------------------------------------------------------------------------------------------------------




#endif


