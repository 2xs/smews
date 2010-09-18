#ifndef PHY_H_
#define PHY_H_




 /******************************************************************************************************
 **************************                     Definitions                      **************************
 *******************************************************************************************************/

//-------------------------------------------------------------------------------------------------------
unsigned char WriteStrobeReg_spi(unsigned char cmd);
unsigned char WriteStrobeReg(unsigned char cmd);
unsigned char WriteConfigReg_spi(unsigned char cmd, unsigned int data);
unsigned int ReadConfigReg_spi(unsigned char cmd);

void Write_TXFIFO(unsigned char *data,unsigned char n);

void WriteRAM_spi(unsigned int cmd,unsigned char data[],unsigned char n);

unsigned char ReadRAM(unsigned int cmd);

void PHY_Init(void);
//-------------------------------------------------------------------------------------------------------




#endif



