#ifndef CC2420_H_
#define CC2420_H_



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                from CC2420 datasheet              **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
//address commands
#define CC2420_SNOP         0x00  //No Operation (has no other effect than reading out status-bits)
#define CC2420_SXOSCON      0x01  //Turn on the crystal oscillator (set XOSC16M_PD = 0 and BIAS_PD = 0)  
#define CC2420_STXCAL       0x02  //Enable and calibrate frequency synthesizer for TX; Go from RX
#define CC2420_SRXON        0x03  //Enable RX
#define CC2420_STXON        0x04  //Enable TX after calibration (if not already performed)
#define CC2420_STXONCCA     0x05  //If CCA indicates a clear channel:Enable calibration, then TX. 
#define CC2420_SRFOFF       0x06  //Disable RX/TX and frequency synthesizer 
#define CC2420_SXOSCOFF     0x07  //Turn off the crystal oscillator and RF 
#define CC2420_SFLUSHRX     0x08  //Flush the RX FIFO buffer and reset the demodulator. 
#define CC2420_SFLUSHTX     0x09  //Flush the TX FIFO buffer 
#define CC2420_SACK         0x0A  //Send acknowledge frame, with pending field cleared. 
#define CC2420_SACKPEND     0x0B  //Send acknowledge frame, with pending field set. 
#define CC2420_SRXDEC       0x0C  //Start RXFIFO in-line decryption / authentication (as set by SPI_SEC_MODE) 
#define CC2420_STXENC       0x0D  //Start TXFIFO in-line encryption / authentication (as set by SPI_SEC_MODE), without starting TX. 
#define CC2420_SAES         0x0E  //AES Stand alone encryption strobe

//CC2420 registers address·
#define CC2420_MAIN         0x10  // Main Control Register
#define CC2420_MDMCTRL0     0x11  // Modem Control Register 0
#define CC2420_MDMCTRL1     0x12  // Modem Control Register 1
#define CC2420_RSSI         0x13  // RSSI and CCA Status and Control register
#define CC2420_SYNCWORD     0x14  // Synchronisation word control register
#define CC2420_TXCTRL       0x15  // Transmit Control Register
#define CC2420_RXCTRL0      0x16  // Receive Control Register 0
#define CC2420_RXCTRL1      0x17  // Receive Control Register 1
#define CC2420_FSCTRL       0x18  // Frequency Synthesizer Control and Status Register//FSCTRL.FREQ=357+5*(K-11)
#define CC2420_SECCTRL0     0x19  // Security Control Register 0 
#define CC2420_SECCTRL1     0x1A  // Security Control Register 1
#define CC2420_BATTMON      0x1B  // Battery Monitor Control and Status Register
#define CC2420_IOCFG0       0x1C  // Input / Output Control Register 0
#define CC2420_IOCFG1       0x1D  // Input / Output Control Register 1
#define CC2420_MANFIDL      0x1E  // Manufacturer ID, Low 16 bits
#define CC2420_MANFIDH      0x1F  // Manufacturer ID, High 16 bits
#define CC2420_FSMTC        0x20  // Finite State Machine Time Constants
#define CC2420_MANAND       0x21  // Manual signal AND override register
#define CC2420_MANOR        0x22  // Manual signal OR override register
#define CC2420_AGCCTRL      0x23  // AGC Control Register
#define CC2420_AGCTST0      0x24  // AGC Test Register 0
#define CC2420_AGCTST1      0x25  // AGC Test Register 1
#define CC2420_AGCTST2      0x26  // AGC Test Register 2
#define CC2420_FSTST0       0x27  // Frequency Synthesizer Test Register 0
#define CC2420_FSTST1       0x28  // Frequency Synthesizer Test Register 1
#define CC2420_FSTST2       0x29  // Frequency Synthesizer Test Register 2
#define CC2420_FSTST3       0x2A  // Frequency Synthesizer Test Register 3
#define CC2420_RXBPFTST     0x2B  // Receiver Bandpass Filter Test Register
#define CC2420_FSMSTATE     0x2C  // Finite State Machine State Status Register 
#define CC2420_ADCTST       0x2D  // ADC Test Register
#define CC2420_DACTST       0x2E  // DAC Test Register
#define CC2420_TOPTST       0x2F  // Top Level Test Register
#define CC2420_RESERVED     0x30  // Reserved for future use control / status register

#define CC2420_TXFIFO       0x3E  // Transmit FIFO Byte Register 
#define CC2420_RXFIFO       0x3F  // Receiver FIFO Byte Register
//-------------------------------------------------------------------------------------------------------



/*******************************************************************************************************
 *******************************************************************************************************
 **************************                cc2420 Memory                 **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//------------------------------------------------------------------------------------------------------
// Sizes
#define CC2420_RAM_SIZE			368
#define CC2420_FIFO_SIZE		128

//RAM Memory Space·
#define CC2420_RAM_TXFIFO		0x000
#define CC2420_RAM_RXFIFO		0x080
#define CC2420_RAM_KEY0			0x100
#define CC2420_RAM_RXNONCE		0x110
#define CC2420_RAM_SABUF		0x120
#define CC2420_RAM_KEY1			0x130
#define CC2420_RAM_TXNONCE		0x140
#define CC2420_RAM_CBCSTATE		0x150
#define CC2420_RAM_IEEEADDR		0x160
#define CC2420_RAM_PANID		0x168   
#define CC2420_RAM_SHORTADDR	        0x16A  
//------------------------------------------------------------------------------------------------------------




/*******************************************************************************************************
 *******************************************************************************************************
 ************************                     cc2420 Status byte              **************************
 *******************************************************************************************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
// Status byte
#define CC2420_XOSC16M_STABLE		6
#define CC2420_TX_UNDERFLOW		5
#define CC2420_ENC_BUSY			4
#define CC2420_TX_ACTIVE		3
#define CC2420_LOCK			2
#define CC2420_RSSI_VALID		1
//-------------------------------------------------------------------------------------------------------
// SECCTRL0
#define CC2420_SECCTRL0_NO_SECURITY         0x0000
#define CC2420_SECCTRL0_CBC_MAC             0x0001
#define CC2420_SECCTRL0_CTR                 0x0002
#define CC2420_SECCTRL0_CCM                 0x0003

#define CC2420_SECCTRL0_SEC_M_IDX           2

#define CC2420_SECCTRL0_RXKEYSEL0           0x0000
#define CC2420_SECCTRL0_RXKEYSEL1           0x0020

#define CC2420_SECCTRL0_TXKEYSEL0           0x0000
#define CC2420_SECCTRL0_TXKEYSEL1           0x0040

#define CC2420_SECCTRL0_SEC_CBC_HEAD        0x0100
#define CC2420_SECCTRL0_RXFIFO_PROTECTION   0x0200
//-------------------------------------------------------------------------------------------------------
// RSSI to Energy Detection conversion
#define RSSI_OFFSET         -38
#define RSSI_2_ED(rssi)     ((rssi) < RSSI_OFFSET ? 0 : ((rssi) - (RSSI_OFFSET)))
#define ED_2_LQI(ed)        (((ed) > 63 ? 255 : ((ed) << 2)))
//-------------------------------------------------------------------------------------------------------



#endif


